#include <sserialize/utility/MultiReaderSingleWriterLock.h>

namespace sserialize {

void MultiReaderSingleWriterLock::unlockAndNotify() {
	m_mtx.unlock();
	m_cv.notify_one();
}
MultiReaderSingleWriterLock::MultiReaderSingleWriterLock() : m_count(CountMax), m_pendingWriteLock(false) {}
MultiReaderSingleWriterLock::~MultiReaderSingleWriterLock() {
	acquireWriteLock();
	releaseWriteLock();
}
void MultiReaderSingleWriterLock::acquireReadLock() {
	std::unique_lock<std::mutex> lck(m_mtx);
	while (m_count == 0) { //write is active
		m_cv.wait(lck);
	}
	--m_count;
}
void MultiReaderSingleWriterLock::releaseReadLock() {
	m_mtx.lock();
	++m_count;
	unlockAndNotify();
}
void MultiReaderSingleWriterLock::acquireWriteLock() {
	//acquiere CountMax of m_count and m_pendingWriteLock
	std::unique_lock<std::mutex> lck(m_mtx);
	//Make sure only one write lock can consume m_count
	while(m_pendingWriteLock) {
		m_cv.wait(lck);
	}
	//now we are the consumer of m_count
	m_pendingWriteLock = true;
	CountType acquiredCount = m_count;
	m_count = 0;
	while (~acquiredCount != static_cast<CountType>(0)) {
		m_cv.wait(lck);
		acquiredCount += m_count;
		m_count = 0;
	}
}
void MultiReaderSingleWriterLock::releaseWriteLock() {
	//release CountMax to m_count and m_pendingWriteLock
	m_mtx.lock();
	m_count = CountMax;
	m_pendingWriteLock = false;
	unlockAndNotify();
}

}//end namespace