#include <sserialize/mt/MultiReaderSingleWriterLock.h>

namespace sserialize {

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

MultiReaderSingleWriterLock::MultiReaderSingleWriterLock() :
m_readerCount(0)
{}

MultiReaderSingleWriterLock::~MultiReaderSingleWriterLock()
{}

void MultiReaderSingleWriterLock::acquireReadLock() {
	m_writeLockMtx.lock();//wait for writers
	m_readerCountMtx.lock();
	++m_readerCount;
	m_readerCountMtx.unlock();
	m_writeLockMtx.unlock();
}
void MultiReaderSingleWriterLock::releaseReadLock() {
	m_readerCountMtx.lock();
	--m_readerCount;
	m_readerCountMtx.unlock();
	m_readerCountCv.notify_one();//notify writer about change in reader count
}

void MultiReaderSingleWriterLock::acquireWriteLock() {
	//acquiere CountMax of m_count and m_pendingWriteLock
	m_writeLockMtx.lock();
	std::unique_lock<std::mutex> lck(m_readerCountMtx);
	while(m_readerCount) { //wait until there's no more reader reading. all other readers wait at acquireReadLock, writers at acquiere WriteLock
		m_readerCountCv.wait(lck);
	}
}
void MultiReaderSingleWriterLock::releaseWriteLock() {
	m_writeLockMtx.unlock();
}

}//end namespace