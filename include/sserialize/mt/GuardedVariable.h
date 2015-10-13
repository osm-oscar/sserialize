#ifndef SSERIALIZE_GUARDED_VARIABLE_H
#define SSERIALIZE_GUARDED_VARIABLE_H
#include <mutex>
#include <condition_variable>

namespace sserialize {

template<typename T_VALUE>
class GuardedVariable {
public:
	typedef std::unique_lock<std::mutex> UniqueLock;
private:
	T_VALUE m_value;
	mutable std::mutex m_mtx;
	std::condition_variable m_cv;
public:
	GuardedVariable() {}
	GuardedVariable(const T_VALUE & value) : m_value(value) {}
	GuardedVariable(const GuardedVariable & other) {
		if (&other != this) {
			UniqueLock mylck(m_mtx);
			UniqueLock olck(other.m_mtx);
			m_value = other.m_value;
		}
	}
	///move the guarded content
	GuardedVariable(GuardedVariable && other) {
		if (this != &other) {
			UniqueLock mylck(m_mtx);
			UniqueLock olck(other.m_mtx);//this doesn't look good
			m_value = std::move(other.m_value);
		}
	}
	~GuardedVariable() {}
	GuardedVariable & operator=(const T_VALUE & value) {
		UniqueLock mylck(m_mtx);
		m_value = value;
		return *this;
	}
	GuardedVariable & operator=(const GuardedVariable & other) {
		if (this != &other) {
			UniqueLock mylck(m_mtx);
			UniqueLock olck(other.m_mtx);//this doesn't look good
			m_value = other.unsyncedValue();
		}
		return *this;
	}
	template<typename TCallback>
	void syncedWithoutNotify(TCallback && cb) {
		UniqueLock lck(m_mtx);
		cb(m_value);
	}
	template<typename TCallback>
	void syncedWithoutNotify(TCallback && cb) const {
		UniqueLock lck(m_mtx);
		cb(m_value);
	}
	template<typename TCallback>
	void syncedWithNotifyOne(TCallback && cb) {
		syncedWithoutNotify(std::forward<TCallback>(cb));
		m_cv.notify_one();
	}
	template<typename TCallback>
	void syncedWithNotifyAll(TCallback && cb) {
		syncedWithoutNotify(std::forward<TCallback>(cb));
		m_cv.notify_all();
	}
	T_VALUE & unsyncedValue() { return m_value; }
	const T_VALUE & unsyncedValue() const { return m_value; }
	UniqueLock uniqueLock() { return UniqueLock(m_mtx); }
	void wait(UniqueLock & ul) {
		m_cv.wait(ul);
	}
	void wait_for(UniqueLock & ul, uint64_t microseconds) {
		m_cv.wait_for(ul, std::chrono::microseconds(microseconds));
	}
	void notify_one() {
		m_cv.notify_one();
	}
	void notify_all() {
		m_cv.notify_all();
	}
	//synced cast operator
	operator T_VALUE() {
		m_mtx.lock();
		T_VALUE tmp = m_value;
		m_mtx.unlock();
		return tmp;
	}
	
	inline void lock() { m_mtx.lock();}
	inline void unlock() { m_mtx.unlock(); }
};

}//end namespace


#endif