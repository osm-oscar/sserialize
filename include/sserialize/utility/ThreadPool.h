#ifndef SSERIALIZE_THREAD_POOL_H
#define SSERIALIZE_THREAD_POOL_H
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <sserialize/templated/GuardedVariable.h>

namespace sserialize {

class ThreadPool final {
private:
	typedef std::function< void(void) > QueuedTaskFunction;
	typedef std::queue< std::function<void(void)> > TaskQueue;
private:
	std::vector<std::thread> m_threads;
	GuardedVariable<TaskQueue> m_queue;
	GuardedVariable<uint32_t> m_runningThreads;//threads MUST notify this variable if they decrement/increment it
	std::atomic<bool> m_queueIsOnline;
	std::atomic<bool> m_online;
public:
	void taskWorkerFunc(uint32_t /*myThreadNumber*/) {
		auto lck(m_queue.uniqueLock());
		{
			auto rtlck(m_runningThreads.uniqueLock());
			m_runningThreads.value() += 1;
			m_runningThreads.notify_all(rtlck);
		}
		while(m_online.load() == true) {
			m_queue.wait(lck);
			if (m_online.load() == true && m_queue.value().size()) {
				QueuedTaskFunction t = m_queue.value().front();
				m_queue.value().pop();
				lck.unlock();
				t();//do the task
			}
		}
		{
			auto rtlck(m_runningThreads.uniqueLock());
			m_runningThreads.value() -= 1;
			m_runningThreads.notify_all(rtlck);
		}
	}
public:
	ThreadPool(uint32_t numThreads = 1) : m_runningThreads(0), m_online(false) {
		this->numThreads(numThreads);
		m_queueIsOnline = true;
	}
	///processes all the work before destruction (might take some time)
	~ThreadPool() {
		//wait for empty queue and remaining threads
		m_online = false;
		auto rtlck(m_runningThreads.uniqueLock());
		while(m_runningThreads.value() > 0) {
			m_queue.notify_one(); //notify a thread
			m_runningThreads.wait_for(rtlck, 1000000);
		}
		for(std::thread & t : m_threads) {
			t.join();
		}
	}
	void offlineQueue() {
		auto qlck(m_queue.uniqueLock());
		m_queueIsOnline = false;
	}
	void onlineQueue() {
		auto qlck(m_queue.uniqueLock());
		m_queueIsOnline = true;
	}
	///Wait for empty queue, takes the queue offline for flushing and enables after flushing
	void flushQueue() {
		auto qlck(m_queue.uniqueLock());
		m_queueIsOnline = false;
		while(m_queue.value().size()) {
			qlck.unlock();
			m_queue.notify_one();
			qlck.lock();
			if (!m_queue.value().size())
				break;
			m_queue.wait(qlck);
		}
		//if we're here, then qlck is locked
		m_queueIsOnline = true;
	}
	void numThreads(uint32_t num) {
		if (num < numThreads()) {
			m_online.store(false);
			auto rtlck(m_runningThreads.uniqueLock());
			while(m_runningThreads.value()) {
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
	uint32_t numThreads() const { return m_threads.size(); }
	template<typename T_TASKFUNC>
	bool sheduleTask(T_TASKFUNC t) {
		if (m_queueIsOnline) {
			auto lck(m_queue.uniqueLock());
			m_queue.value().push( QueuedTaskFunction(t) );
			m_queue.notify_one(lck);
			return true;
		}
		else {
			return false;
		}
	}
	template<typename T_TASKFUNC, typename... Args>
	bool sheduleTaskWithArgs(T_TASKFUNC t, Args...args) {
		auto tmp = std::bind(t, args...);
		return sheduleTask(tmp);
	}
};


}

#endif