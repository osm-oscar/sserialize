#include <sserialize/mt/MultiReaderSingleWriterLock.h>
#include <shared_mutex>

namespace sserialize {
	
#if defined(__cplusplus) && __cplusplus > 201402L
inline namespace MultiReaderSingleWriterLock_shared_mutex {
#else
inline namespace MultiReaderSingleWriterLock_shared_timed_mutex {
#endif

MultiReaderSingleWriterLock::ReadLock::ReadLock(MultiReaderSingleWriterLock & lock) :
m_lock(lock),
m_locked(false)
{
	this->lock();
}

MultiReaderSingleWriterLock::ReadLock::~ReadLock() {
	this->unlock();
}

void MultiReaderSingleWriterLock::ReadLock::lock() {
	if (!m_locked) {
		m_lock.acquireReadLock();
		m_locked = true;
	}
}

void MultiReaderSingleWriterLock::ReadLock::unlock() {
	if (m_locked) {
		m_lock.releaseReadLock();
		m_locked = false;
	}
}

MultiReaderSingleWriterLock::WriteLock::WriteLock(MultiReaderSingleWriterLock & lock) :
m_lock(lock),
m_locked(false)
{
	this->lock();
}

MultiReaderSingleWriterLock::WriteLock::~WriteLock() {
	this->unlock();
}

void MultiReaderSingleWriterLock::WriteLock::lock() {
	if (!m_locked) {
		m_lock.acquireWriteLock();
		m_locked = true;
	}
}

void MultiReaderSingleWriterLock::WriteLock::unlock() {
	if (m_locked) {
		m_lock.releaseWriteLock();
		m_locked = false;
	}
}

MultiReaderSingleWriterLock::MultiReaderSingleWriterLock()
{}

MultiReaderSingleWriterLock::~MultiReaderSingleWriterLock()
{}

void MultiReaderSingleWriterLock::acquireReadLock() {
	m_mutex.lock_shared();
}
void MultiReaderSingleWriterLock::releaseReadLock() {
	m_mutex.unlock_shared();
}

void MultiReaderSingleWriterLock::acquireWriteLock() {
	m_mutex.lock();
}
void MultiReaderSingleWriterLock::releaseWriteLock() {
	m_mutex.unlock();
}

}//end inline protection namespace

}//end namespace
