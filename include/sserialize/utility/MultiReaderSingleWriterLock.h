#ifndef SSERIALIZE_QUEUED_MULTI_READER_SINGLE_WRITER_LOCK_H
#define SSERIALIZE_QUEUED_MULTI_READER_SINGLE_WRITER_LOCK_H
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>

namespace sserialize {

//Reader preferred multiple reader single writer lock
//There's no queue, conseuqnelt there might be starvation of writers
class MultiReaderSingleWriterLock {
private:
	typedef uint32_t CountType;
	static constexpr CountType CountMax = 0xFFFFFFFF;
private:
	std::mutex m_mtx;
	std::condition_variable m_cv;
	CountType m_count;
	bool m_pendingWriteLock;
	
private:
	void unlockAndNotify();
public:
	MultiReaderSingleWriterLock();
	~MultiReaderSingleWriterLock();
	void acquireReadLock();
	void releaseReadLock();
	void acquireWriteLock();
	void releaseWriteLock();
};

}//end namespace
#endif