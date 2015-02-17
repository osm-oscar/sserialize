#ifndef SSERIALIZE_PROGRESS_INFO_H
#define SSERIALIZE_PROGRESS_INFO_H
#include <iostream>
#include <iomanip>
#include <time.h>
#include <sys/time.h>


namespace sserialize {

struct ProgressInfo {
	uint64_t targetCount;
	time_t startTimer;
	time_t prevTimer;
	std::string message;

	void begin(uint64_t tCount, const std::string & message);
	void begin(uint64_t tCount);
	
	void end();
	void end(const std::string & message);
	
	void operator()(uint64_t currentCount);
	void operator()(uint64_t currentCount, const std::string & message);
};

}//end namespace

#endif