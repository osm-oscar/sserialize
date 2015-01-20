#include <sserialize/utility/MultiReaderSingleWriterLock.h>

namespace sserialize {

MultiReaderSingleWriterLock::MultiReaderSingleWriterLock() : m_readerCount(0) {}
MultiReaderSingleWriterLock::~MultiReaderSingleWriterLock() {}
void MultiReaderSingleWriterLock::acquireReadLock() {
	std::unique_lock<std::mutex> lck(m_writeLockMtx);//wait for writers
	std::unique_lock<std::mutex> rLck(m_readerCountMtx);
	++m_readerCount;
}
void MultiReaderSingleWriterLock::releaseReadLock() {
	std::unique_lock<std::mutex> lck(m_readerCountMtx);
	--m_readerCount;
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