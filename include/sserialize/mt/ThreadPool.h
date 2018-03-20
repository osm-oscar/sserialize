#ifndef SSERIALIZE_THREAD_POOL_H
#define SSERIALIZE_THREAD_POOL_H
#include <sserialize/mt/GuardedVariable.h>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace sserialize {

class ThreadPool final {
public:
	struct SingletonTaskTag {};
	struct CopyTaskTag {};
private:
	typedef std::function< void(void) > QueuedTaskFunction;
	typedef std::queue<QueuedTaskFunction> TaskQueue;
	struct QueueInfo {
		TaskQueue q;
		uint32_t runningTasks;
		QueueInfo() : runningTasks(0) {}
	};
private:
	std::vector<std::thread> m_threads;
	GuardedVariable<QueueInfo> m_qi;
	std::atomic<bool> m_online; //is ThreadPool online
	GuardedVariable<uint32_t> m_runningThreads;//threads MUST notify this variable if they decrement/increment it
private:
	void stop();
	void start(uint32_t count);
public:
	void taskWorkerFunc(uint32_t /*myThreadNumber*/);
public:
	ThreadPool(uint32_t numThreads = 1);
	///Destroys the thread pool without doing any more work
	///Call flushQueue() to flush the queue
	~ThreadPool();
	///Wait for empty queue
	void flushQueue();
	std::size_t queueSize() const;
	void numThreads(uint32_t num);
	uint32_t numThreads() const;
	bool sheduleTask(QueuedTaskFunction t);
	template<typename T_TASKFUNC, typename... Args>
	bool sheduleTaskWithArgs(T_TASKFUNC t, Args&&...args) {
		auto tmp = std::bind(t, std::forward<Args>(args)...);
		return sheduleTask(tmp);
	}
	
	///execute task t with threadCount threads by spawning new threads
	static void execute(QueuedTaskFunction t, uint32_t threadCount = 0);
	
	///execute task t with threadCount threads by spawning new threads
	template<typename T_TASKFUNC>
	static void execute(T_TASKFUNC t, uint32_t threadCount, SingletonTaskTag const &) {
		execute(QueuedTaskFunction(t), threadCount);
	}
	
	template<typename T_TASKFUNC>
	static void execute(T_TASKFUNC t, uint32_t threadCount, CopyTaskTag const &) {
		std::vector<std::thread> threads;
		threads.reserve(threadCount);
		for(uint32_t i(0); i < threadCount; ++i) {
			threads.emplace_back(t);
		}
		for(std::thread & x : threads) {
			x.join();
		}
	}
	
	///execute task t with threadCount threads by spawning new threads
	template<typename T_TASKFUNC, typename... Args>
	static void execute(T_TASKFUNC t, uint32_t threadCount, SingletonTaskTag const &, Args&&...args) {
		auto tmp = std::bind(t, std::forward<Args>(args)...);
		return execute(tmp, threadCount);
	}
	
	///execute task t with threadCount threads by spawning new threads
	///Calls t for each element in the range exactly once
	template<typename T_TASKFUNC, typename T_ITERATOR>
	static void map(T_TASKFUNC t, T_ITERATOR begin, T_ITERATOR end, uint32_t threadCount = 0) {
		struct State {
			T_ITERATOR b;
			T_ITERATOR e;
			std::mutex lck;
			State(T_ITERATOR begin, T_ITERATOR end) : b(begin), e(end) {}
		};
		State state(begin, end);
		execute([&state, t]() {
			std::unique_lock<std::mutex> lck(state.lck);
			lck.unlock();
			while (true) {
				lck.lock();
				if (state.b != state.e) {
					T_ITERATOR m = state.b;
					++state.b;
					lck.unlock();
					t(*m);
				}
				else {
					return;
				}
			}
		}, threadCount);
	}
};

}

#endif
