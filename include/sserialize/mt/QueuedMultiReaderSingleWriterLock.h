#ifndef SSERIALIZE_QUEUED_MULTI_READER_SINGLE_WRITER_LOCK_H
#define SSERIALIZE_QUEUED_MULTI_READER_SINGLE_WRITER_LOCK_H
#include <mutex>
#include <condition_variable>
#include <deque>

namespace sserialize {

class QueuedMultiReaderSingleWriterLock {
private:
	uint64_t m_count;
	std::mutex m_mtx;
	std::deque<std::condition_variable*> m_cvs;
private:
	inline void unlockAndNotify() {
		if (m_cvs.size()) {
			std::condition_variable * cv = m_cvs.front();
			m_cvs.pop_front();
			m_mtx.unlock();//unlock so that notified thread can take a lock
			cv->notify_one();
		}
		else {
		}
		m_mtx.unlock();
	}
public:
	QueuedMultiReaderSingleWriterLock() : m_count(std::numeric_limits<uint64_t>::max()) {}
	~QueuedMultiReaderSingleWriterLock() {
		acquireWriteLock();
		releaseWriteLock();
	}
	inline void acquireReadLock() {
		std::unique_lock<std::mutex> lck(m_mtx);
		if (m_count > 0) {
			--m_count;
		}
		else {
			std::condition_variable cv;
			while (m_count == 0) { //write is active
				m_cvs.push_back(&cv);
				cv.wait(lck);
			}
			--m_count;
		}
	}
	inline void releaseReadLock() {
		m_mtx.lock();
		++m_count;
		unlockAndNotify();
	}
	inline void acquireWriteLock() {
		std::unique_lock<std::mutex> lck(m_mtx);
		if (m_count == std::numeric_limits<uint64_t>::max()) {
			m_count = 0;
		}
		else {
			std::condition_variable cv;
			m_cvs.push_back(&cv);
			cv.wait(lck);
			while (m_count != std::numeric_limits<uint64_t>::max()) { //a read is still active
				m_cvs.push_front(&cv); //first acquire the write lock before anything else
				cv.wait(lck);
			}
		}
	}
	inline void releaseWriteLock() {
		m_mtx.lock();
		m_count = std::numeric_limits<uint64_t>::max();
		unlockAndNotify();
	}
};

}//end namespace
#endif