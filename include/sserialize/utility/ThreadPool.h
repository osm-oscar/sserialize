#ifndef SSERIALIZE_THREAD_POOL_H
#define SSERIALIZE_THREAD_POOL_H
#include <sserialize/templated/GuardedVariable.h>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace sserialize {

class ThreadPool final {
private:
	typedef std::function< void(void) > QueuedTaskFunction;
	class TaskQueue {
		std::queue< std::function<void(void)> > m_q;
		bool m_online;
	public:
		std::size_t size() const { return m_q.size(); }
		void pop() { m_q.pop(); }
		QueuedTaskFunction & front() { return m_q.front(); }
		void push(QueuedTaskFunction && f) { m_q.push(f); }
		void push(const QueuedTaskFunction & f) { m_q.push(f); }
		bool online() const { return m_online; }
		void online(bool v) { m_online = v; }
	};
private:
	std::vector<std::thread> m_threads;
	GuardedVariable<TaskQueue> m_queue;
	GuardedVariable<uint32_t> m_runningTasks;
	GuardedVariable<uint32_t> m_runningThreads;//threads MUST notify this variable if they decrement/increment it
	std::atomic<bool> m_online;
public:
	void taskWorkerFunc(uint32_t /*myThreadNumber*/) {
		{
			auto rtlck(m_runningThreads.uniqueLock());
			m_runningThreads.value() += 1;
			m_runningThreads.notify_all(rtlck);
		}
		auto lck(m_queue.uniqueLock());
		while(m_online.load() == true) {
			m_queue.wait(lck);
			if (m_online.load() == true && m_queue.value().size()) {
				QueuedTaskFunction t = m_queue.value().front();
				m_queue.value().pop();
				lck.unlock();
				{
					auto rtlck(m_runningTasks.uniqueLock());
					m_runningTasks.value() += 1;
					m_runningTasks.notify_one();
				}
				t();//do the task
				{
					auto rtlck(m_runningTasks.uniqueLock());
					m_runningTasks.value() += 1;
					m_runningTasks.notify_one();
				}
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
		auto qlck(m_queue.uniqueLock());
		m_queue.value().online(true);
	}
	///processes all the work before destruction (might take some time)
	~ThreadPool() {
		offlineQueue();
		//wait for empty queue and remaining threads
		flushQueue();
		//take the threads offline
		m_online = false;
		auto rthlck(m_runningThreads.uniqueLock());
		while(m_runningThreads.value() > 0) {
			m_queue.notify_one(); //notify a thread in case it's waiting for a new queue element
			m_runningThreads.wait(rthlck); //is wait atomic for unlock?
		}
		for(std::thread & t : m_threads) {
			t.join();
		}
	}
	void offlineQueue() {
		auto qlck(m_queue.uniqueLock());
		m_queue.value().online(false);
	}
	void onlineQueue() {
		auto qlck(m_queue.uniqueLock());
		m_queue.value().online(true);
	}
	///Wait for empty queue
	void flushQueue() {
		auto qlck(m_queue.uniqueLock());
		auto rtlck(m_runningTasks.uniqueLock());
		while(m_queue.value().size()) {
			qlck.unlock();
			m_runningTasks.wait(rtlck);
			qlck.lock();
		}
	}
	void numThreads(uint32_t num) {
		if (num < numThreads()) {
			m_online.store(false);
			auto rtlck(m_runningThreads.uniqueLock());
			while(m_runningThreads.value() > 0) {
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
		auto lck(m_queue.uniqueLock());
		if (m_queue.value().online()) {
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