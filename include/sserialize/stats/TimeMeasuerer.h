#ifndef SSERIALIZE_TIME_MEASURER_H
#define SSERIALIZE_TIME_MEASURER_H
#include <time.h>
#include <sys/time.h>
#include <cstdlib> 
#include <stdint.h>
#include <string.h>
#include <iostream>


namespace sserialize {

class TimeMeasurer {
public:
	using TimeType = uint64_t;
public:
	TimeMeasurer() {
		::memset(&m_begin, 0, sizeof(struct timeval));
		::memset(&m_end, 0, sizeof(struct timeval));
	}
	
	~TimeMeasurer() {}
	
	inline void begin() {
		gettimeofday(&m_begin, NULL);
	}
	
	inline void end() {
		gettimeofday(&m_end, NULL);
	}
	
	inline TimeType beginTime() const {
		return m_begin.tv_sec;
	}
	
	/** @return returns the elapsed time in useconds  */
	inline TimeType elapsedTime() const {
		long mtime, seconds, useconds;
		seconds  = m_end.tv_sec  - m_begin.tv_sec;
		useconds = m_end.tv_usec - m_begin.tv_usec;
		mtime = (TimeType)((double)((seconds) * 1000*1000 + useconds) + 0.5);
		return mtime;
	}
	
	inline TimeType elapsedUseconds() const {
		return elapsedTime();
	}

	inline TimeType elapsedMilliSeconds() const {
		return elapsedTime()/1000;
	}
	
	inline TimeType elapsedSeconds() const {
		return elapsedTime()/1000000;
	}

	inline TimeType elapsedMinutes() {
		return elapsedSeconds()/60;
	}
	
	inline void reset() {
		m_begin = m_end;
	}
private:
	struct timeval m_begin, m_end;
};

std::ostream & operator<<(std::ostream & out, const TimeMeasurer & tm);

}//end namespace

#endif
