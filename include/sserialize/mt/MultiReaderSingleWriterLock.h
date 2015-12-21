#ifndef SSERIALIZE_QUEUED_MULTI_READER_SINGLE_WRITER_LOCK_H
#define SSERIALIZE_QUEUED_MULTI_READER_SINGLE_WRITER_LOCK_H
#include <mutex>
#include <condition_variable>

namespace sserialize {

//Reader preferred multiple reader single writer lock
//There's no queue, conseuqnelt there might be starvation of writers
class MultiReaderSingleWriterLock {
private:
	std::mutex m_readerCountMtx;
	std::condition_variable m_readerCountCv;
	uint32_t m_readerCount;
	std::mutex m_writeLockMtx;
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