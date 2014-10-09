#ifndef SSERIALIZE_QUEUED_MULTI_READER_SINGLE_WRITER_LOCK_H
#define SSERIALIZE_QUEUED_MULTI_READER_SINGLE_WRITER_LOCK_H
#include <mutex>
#include <condition_variable>
#include <deque>

namespace sserialize {

class MultiReaderSingleWriterLock {
private:
	typedef uint32_t CountType;
	static constexpr CountType CountMax = 0xFFFFFFFF;
private:
	std::mutex m_mtx;
	std::condition_variable m_cv;
	CountType m_count;
private:
	inline void unlockAndNotify() {
		m_mtx.unlock();
		m_cv.notify_one();
	}
public:
	MultiReaderSingleWriterLock() : m_count(CountMax) {}
	~MultiReaderSingleWriterLock() {
		acquireWriteLock();
		releaseWriteLock();
	}
	inline void acquireReadLock() {
		std::unique_lock<std::mutex> lck(m_mtx);
		while (m_count == 0) { //write is active
			m_cv.wait(lck);
		}
		--m_count;
	}
	inline void releaseReadLock() {
		m_mtx.lock();
		++m_count;
		unlockAndNotify();
	}
	inline void acquireWriteLock() {
		std::unique_lock<std::mutex> lck(m_mtx);
		CountType acquiredCount = m_count;
		m_count = 0;
		while (~acquiredCount != static_cast<CountType>(0)) {
			m_cv.wait(lck);
			acquiredCount += m_count;
			m_count = 0;
		}
	}
	inline void releaseWriteLock() {
		m_mtx.lock();
		m_count = CountMax;
		unlockAndNotify();
	}
};

}//end namespace
#endif