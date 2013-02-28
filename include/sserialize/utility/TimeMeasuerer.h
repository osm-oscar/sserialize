#ifndef SSERIALIZE_TIME_MEASURER_H
#define SSERIALIZE_TIME_MEASURER_H
#include <time.h>
#include <sys/time.h>
#include <cstdlib> 
#include <stdint.h>


namespace sserialize {

class TimeMeasurer {
private:
	struct timeval m_begin, m_end;
public:
	TimeMeasurer() {}
	~TimeMeasurer() {}
	inline void begin() {
		gettimeofday(&m_begin, NULL);
	}
	inline void end() {
		gettimeofday(&m_end, NULL);
	}
	/** @return returns the elapsed time in useconds  */
	long elapsedTime() const {
		long mtime, seconds, useconds;
		seconds  = m_end.tv_sec  - m_begin.tv_sec;
		useconds = m_end.tv_usec - m_begin.tv_usec;
		mtime = ((seconds) * 1000*1000 + useconds) + 0.5;
		return mtime;
	}

	long elapsedMilliSeconds() const {
		return elapsedTime()/1000;
	}
	
	long elapsedSeconds() const {
		return elapsedTime()/1000000;
	}

	long elapsedMinutes() {
		return elapsedSeconds()/60;
	}
};

}//end namespace

#endif