#include <sserialize/mt/ThreadPool.h>
#include <sserialize/utility/assert.h>

namespace sserialize {

void ThreadPool::taskWorkerFunc(uint32_t /*myThreadNumber*/) {
	m_runningThreads.syncedWithNotifyAll([](uint32_t & v) { v += 1;});
	while(m_online.load() == true) {
		QueuedTaskFunction t;
		{
			auto qlck(m_qi.uniqueLock());
			if (m_online.load() == true && m_qi.unsyncedValue().q.empty()) {
				m_qi.wait(qlck);
			}
			if (m_online.load() == false || m_qi.unsyncedValue().q.empty()) {
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

ThreadPool::ThreadPool(uint32_t numThreads) : m_online(false), m_runningThreads(0) {
	this->numThreads(numThreads);
}

ThreadPool::~ThreadPool() {
	m_online = false;
	auto rthlck(m_runningThreads.uniqueLock());
	while(m_runningThreads.unsyncedValue() > 0) {
		//this is needed to ensure that worker threads are either waiting or are before their quelock
		//if they are waiting then this will all wake them up
		//if they are before their queue lock then m_online will already be false and they will stop execution
		m_qi.syncedWithNotifyAll([](const QueueInfo & q){});
		m_runningThreads.wait(rthlck); //is wait atomic for unlock?
	}
	for(std::thread & t : m_threads) {
		t.join();
	}
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
	std::size_t rt;
	m_qi.syncedWithoutNotify([&rt](const QueueInfo & qi) {
		rt = qi.q.size();
	});
	return rt;
}

void ThreadPool::numThreads(uint32_t num) {
	if (num < numThreads()) {
		m_online.store(false);
		auto rthlck(m_runningThreads.uniqueLock());
		while(m_runningThreads.unsyncedValue() > 0) {
			m_runningThreads.wait(rthlck);
		}
		for(std::thread & t : m_threads) {
			t.join();
		}
		m_threads.clear();
	}
	m_online.store(true);
	m_threads.reserve(num);
	while (m_threads.size() < num) {
		m_threads.push_back(std::thread(&sserialize::ThreadPool::taskWorkerFunc, this, (uint32_t) m_threads.size()));
	}
	
	//wait until all threads are up and running
	auto rthlck(m_runningThreads.uniqueLock());
	while (m_runningThreads.unsyncedValue() < m_threads.size()) {
		m_runningThreads.wait(rthlck);
	}
}

uint32_t ThreadPool::numThreads() const {
	return (uint32_t) m_threads.size();
}

bool ThreadPool::sheduleTask(QueuedTaskFunction t) {//std::function als parameter
	m_qi.syncedWithNotifyOne([&t](QueueInfo & v) { v.q.push( QueuedTaskFunction(t) );});
	return true;
}

void ThreadPool::execute(QueuedTaskFunction t, uint32_t threadCount) {
	if (!threadCount) {
		threadCount = std::max<uint32_t>(1, std::thread::hardware_concurrency());
	}
	
	if (threadCount == 1) {
		t();
		return;
	}
	
	std::vector<std::thread> threads;
	threads.reserve(threadCount);
	for(uint32_t i(0); i < threadCount; ++i) {
		threads.emplace_back(std::thread(t));
	}
	for(std::thread & th : threads) {
		th.join();
	}
}


}//end namespace