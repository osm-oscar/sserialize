#include <sserialize/utility/MultiReaderSingleWriterLock.h>

namespace sserialize {

MultiReaderSingleWriterLock::MultiReaderSingleWriterLock() : m_count(CountMax) {}
MultiReaderSingleWriterLock::~MultiReaderSingleWriterLock() {
	acquireWriteLock();
	releaseWriteLock();
}
void MultiReaderSingleWriterLock::acquireReadLock() {
	std::unique_lock<std::mutex> lck(m_countMtx);
	while (m_count == 0) { //write is active
		m_countCv.wait(lck);
	}
	--m_count;
}
void MultiReaderSingleWriterLock::releaseReadLock() {
	m_countMtx.lock();
	++m_count;
	m_countMtx.unlock();
	m_countCv.notify_one();
}
void MultiReaderSingleWriterLock::acquireWriteLock() {
	//acquiere CountMax of m_count and m_pendingWriteLock
	//disallow another writer to try to get the real write lock
	std::unique_lock<std::mutex> writeLck(m_writeLockMtx);
	std::unique_lock<std::mutex> countLck(m_countMtx);
	CountType acquiredCount = m_count;
	m_count = 0;
	while (~acquiredCount != static_cast<CountType>(0)) {
		m_countCv.wait(countLck);
		acquiredCount += m_count;
		m_count = 0;
	}
}
void MultiReaderSingleWriterLock::releaseWriteLock() {
	//release CountMax to m_count and m_pendingWriteLock
	m_countMtx.lock();
	m_count = CountMax;
	m_countMtx.unlock();
	m_countCv.notify_one();
}

}//end namespace