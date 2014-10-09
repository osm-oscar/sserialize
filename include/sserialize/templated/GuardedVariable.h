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
	std::mutex m_mtx;
	std::condition_variable m_cv;
public:
	GuardedVariable() {}
	GuardedVariable(const T_VALUE & value) : m_value(value) {}
	GuardedVariable(const GuardedVariable & other) {
		UniqueLock mylck(m_mtx);
		UniqueLock olck(const_cast<GuardedVariable&>(other).m_mtx);//this doesn't look good
		m_value = other.m_value;
	}
	~GuardedVariable() {}
	void set(const T_VALUE & value) {
		      UniqueLock lck(m_mtx);
		m_value = value;
	}
	GuardedVariable & operator=(const T_VALUE & value) {
		set(value);
		return *this;
	}
	GuardedVariable & operator=(const GuardedVariable & other) {
		if (this != &other) {
			UniqueLock mylck(m_mtx);
			UniqueLock olck( const_cast<GuardedVariable&>(other).m_mtx);//this doesn't look good
			m_value = other.value();
		}
		return *this;
	}
	T_VALUE & value() { return m_value; }
	const T_VALUE & value() const { return m_value; }
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
	///@param ul unlock lock associated with this variable before notification
	void notify_one(UniqueLock & ul) {
		ul.unlock();
		m_cv.notify_one();
	}
	void notify_all() {
		m_cv.notify_all();
	}
	///@param ul unlock lock before notification
	void notify_all(UniqueLock & ul) {
		ul.unlock();
		m_cv.notify_all();
	}
	operator T_VALUE() {
		m_mtx.lock();
		T_VALUE tmp = m_value;
		m_mtx.unlock();
		return tmp;
	}
};

}//end namespace


#endif