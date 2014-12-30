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
	typedef std::queue< std::function<void(void)> > TaskQueue;
private:
	std::vector<std::thread> m_threads;
	GuardedVariable<TaskQueue> m_queue;
	std::atomic<bool> m_online; //is ThreadPool online
	GuardedVariable<uint32_t> m_runningThreads;//threads MUST notify this variable if they decrement/increment it
	GuardedVariable<uint32_t> m_runningTasks;
public:
	void taskWorkerFunc(uint32_t /*myThreadNumber*/) {
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
public:
	ThreadPool(uint32_t numThreads = 1) : m_online(false), m_runningThreads(0), m_runningTasks(0) {
		this->numThreads(numThreads);
	}
	///Destroys the thread pool without doing any more work
	///Call flushQueue() to flush the queue
	~ThreadPool() {
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
	///Wait for empty queue
	void flushQueue() {
		auto qlck(m_queue.uniqueLock());
		auto rtlck(m_runningTasks.uniqueLock());
		while(m_queue.unsyncedValue().size() || m_runningTasks.unsyncedValue() > 0) {
			qlck.unlock();
			m_runningTasks.wait(rtlck);
			qlck.lock();
		}
	}
	void numThreads(uint32_t num) {
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
	uint32_t numThreads() const { return m_threads.size(); }
	bool sheduleTask(QueuedTaskFunction t) {//std::function als parameter
		m_queue.syncedWithNotifyOne([&t](TaskQueue & v) { v.push( QueuedTaskFunction(t) );});
		return true;
	}
	template<typename T_TASKFUNC, typename... Args>
	bool sheduleTaskWithArgs(T_TASKFUNC t, Args&&...args) {
		auto tmp = std::bind(t, std::forward<Args>(args)...);
		return sheduleTask(tmp);
	}
};


}

#endif