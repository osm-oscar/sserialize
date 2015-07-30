#include <sserialize/mt/ThreadPool.h>

namespace sserialize {

void ThreadPool::taskWorkerFunc(uint32_t /*myThreadNumber*/) {
	m_runningThreads.syncedWithNotifyAll([](uint32_t & v) { v += 1;});
	while(m_online.load() == true) {
		QueuedTaskFunction t;
		{
			auto qlck(m_queue.uniqueLock());
			if (m_queue.unsyncedValue().empty()) {
				m_queue.wait(qlck);
			}
			if (m_queue.unsyncedValue().empty())
				continue;
			t = std::move(m_queue.unsyncedValue().front());
			m_queue.unsyncedValue().pop();
		}
		m_runningTasks.syncedWithNotifyAll([](uint32_t & v) { v += 1;});
		t();//do the task
		m_runningTasks.syncedWithNotifyAll([](uint32_t & v) { v -= 1;});
	}
	m_runningThreads.syncedWithNotifyAll([](uint32_t & v) { v -= 1;});
}

ThreadPool::ThreadPool(uint32_t numThreads) : m_online(false), m_runningThreads(0), m_runningTasks(0) {
	this->numThreads(numThreads);
}

ThreadPool::~ThreadPool() {
	m_online = false;
	auto rthlck(m_runningThreads.uniqueLock());
	while(m_runningThreads.unsyncedValue() > 0) {
		m_queue.notify_one(); //notify a thread in case it's waiting for a new queue element
		m_runningThreads.wait(rthlck); //is wait atomic for unlock?
	}
	for(std::thread & t : m_threads) {
		t.join();
	}
}

void ThreadPool::flushQueue() {
	auto qlck(m_queue.uniqueLock());
	auto rtlck(m_runningTasks.uniqueLock());
	while(m_queue.unsyncedValue().size() || m_runningTasks.unsyncedValue() > 0) {
		qlck.unlock();
		m_runningTasks.wait(rtlck);
		qlck.lock();
	}
}

void ThreadPool::numThreads(uint32_t num) {
	if (num < numThreads()) {
		m_online.store(false);
		auto rtlck(m_runningThreads.uniqueLock());
		while(m_runningThreads.unsyncedValue() > 0) {
			m_runningThreads.wait(rtlck);
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
}

uint32_t ThreadPool::numThreads() const {
	return m_threads.size();
}

bool ThreadPool::sheduleTask(QueuedTaskFunction t) {//std::function als parameter
	m_queue.syncedWithNotifyOne([&t](TaskQueue & v) { v.push( QueuedTaskFunction(t) );});
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