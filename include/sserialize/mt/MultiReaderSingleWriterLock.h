#ifndef SSERIALIZE_MULTI_READER_SINGLE_WRITER_LOCK_H
#define SSERIALIZE_MULTI_READER_SINGLE_WRITER_LOCK_H
#include <shared_mutex>

namespace sserialize {

//Reader preferred multiple reader single writer lock
//There's no queue, conseuqnelt there might be starvation of writers
class MultiReaderSingleWriterLock {
public:
	class ReadLock final {
	public:
		ReadLock(MultiReaderSingleWriterLock & lock);
		~ReadLock();
	public:
		///call this only from one thread!
		void lock();
		///call this only from one thread!
		void unlock();
	private:
		MultiReaderSingleWriterLock & m_lock;
		bool m_locked;
	};
	class WriteLock final {
	public:
		WriteLock(MultiReaderSingleWriterLock & lock);
		~WriteLock();
	public:
		///call this only from one thread!
		void lock();
		///call this only from one thread!
		void unlock();
	private:
		MultiReaderSingleWriterLock & m_lock;
		bool m_locked;
	};
public:
	MultiReaderSingleWriterLock();
	~MultiReaderSingleWriterLock();
	void acquireReadLock();
	void releaseReadLock();
	void acquireWriteLock();
	void releaseWriteLock();
private:
	std::shared_timed_mutex m_mutex;
};

}//end namespace
#endif