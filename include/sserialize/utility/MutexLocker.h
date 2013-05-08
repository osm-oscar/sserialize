#ifndef SSERIALIZE_MUTEX_LOCKER_H
#define SSERIALIZE_MUTEX_LOCKER_H
#include <mutex>

namespace sserialize {

class MutexLocker {
public:
	std::mutex & m_mutex;
	MutexLocker(std::mutex & mutex) : m_mutex(mutex) {
		m_mutex.lock();
	}
	~MutexLocker() {
		m_mutex.unlock();
	}
};

}//end namespace


#endif