#include <sserialize/mt/ThreadPool.h>
#include <sserialize/utility/assert.h>

namespace sserialize {

void ThreadPool::taskWorkerFunc(uint32_t /*myThreadNumber*/) {
	m_runningThreads.syncedWithNotifyAll([](uint32_t & v) { v += 1;});
	while(m_online.load() == true) {
		QueuedTaskFunction t;
		{
			auto qlck(m_qi.uniqueLock());
			if (m_online.load() && m_qi.unsyncedValue().q.empty()) {
				m_qi.wait(qlck);
			}
			//this is needed in case of spurious wake-ups or if the queue was shutdown while we were waiting
			if (!m_online.load() || m_qi.unsyncedValue().q.empty()) {
				continue;
			}
			m_qi.unsyncedValue().runningTasks += 1;
			t = std::move(m_qi.unsyncedValue().q.front());
			m_qi.unsyncedValue().q.pop();
		}
		t();//do the task
		m_qi.syncedWithNotifyAll([](QueueInfo & v) { v.runningTasks -= 1;});
	}
	m_runningThreads.syncedWithNotifyAll([](uint32_t & v) { v -= 1;});
}

void ThreadPool::start(uint32_t count) {
	SSERIALIZE_CHEAP_ASSERT(!m_online.load());
	m_online.store(true);
	m_threads.reserve(count);
	while (m_threads.size() < count) {
		m_threads.push_back(std::thread(&sserialize::ThreadPool::taskWorkerFunc, this, (uint32_t) m_threads.size()));
	}
	
	//wait until all threads are up and running
	auto rthlck(m_runningThreads.uniqueLock());
	while (m_runningThreads.unsyncedValue() < m_threads.size()) {
		m_runningThreads.wait(rthlck);
	}
}

void ThreadPool::stop() {
	m_online = false;
	auto rthlck(m_runningThreads.uniqueLock());
	while(m_runningThreads.unsyncedValue() > 0) {
		//this is needed to ensure that worker threads are either waiting or are before their quelock
		//if they are waiting then this will all wake them up
		//if they are before their queue lock then m_online will already be false and they will stop execution
		m_qi.syncedWithNotifyAll([](const QueueInfo &){});
		m_runningThreads.wait(rthlck); //is wait atomic for unlock?
	}
	for(std::thread & t : m_threads) {
		t.join();
	}
}

ThreadPool::ThreadPool(uint32_t numThreads) :
m_online(false),
m_runningThreads(0)
{
	if (!numThreads) {
		numThreads = std::thread::hardware_concurrency();
	}
	start(numThreads);
}

ThreadPool::~ThreadPool() {
	stop();
}

void ThreadPool::flushQueue() {
	auto qlck(m_qi.uniqueLock());
	while(m_qi.unsyncedValue().q.size() || m_qi.unsyncedValue().runningTasks > 0) {
		m_qi.wait(qlck);
	}
	SSERIALIZE_CHEAP_ASSERT_EQUAL((uint32_t) 0, m_qi.unsyncedValue().runningTasks);
	SSERIALIZE_CHEAP_ASSERT_EQUAL((std::size_t) 0, m_qi.unsyncedValue().q.size());
}

std::size_t ThreadPool::queueSize() const {
	return m_qi.unsyncedValue().q.size();
}

void ThreadPool::numThreads(uint32_t num) {
	stop();
	start(num);
}

uint32_t ThreadPool::numThreads() const {
	return (uint32_t) m_threads.size();
}

bool ThreadPool::sheduleTask(QueuedTaskFunction t) {
	m_qi.syncedWithNotifyOne([&t](QueueInfo & v) {
		v.q.push( QueuedTaskFunction(t) );
	});
	return true;
}

void ThreadPool::execute(QueuedTaskFunction t, uint32_t threadCount) {
	if (!threadCount) {
		threadCount = globals::threadPool.numThreads();
	}
	
	for(uint32_t i(0); i < threadCount; ++i) {
		globals::threadPool.sheduleTask(t);
	}
}

namespace globals {
	
sserialize::ThreadPool threadPool;
	
} //end namespace globals


}//end namespace
