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

template<bool enable>
struct OptionalProgressInfo;

template<>
struct OptionalProgressInfo<false> {
	inline void begin(uint64_t /*tCount*/, const std::string & /*message*/) {}
	inline void begin(uint64_t /*tCount*/) {}
	
	inline void end() {}
	inline void end(const std::string & /*message*/) {}
	
	inline void operator()(uint64_t /*currentCount*/) {}
	inline void operator()(uint64_t /*currentCount*/, const std::string & /*message*/) {}
};


template<>
struct OptionalProgressInfo<true> {
	sserialize::ProgressInfo base;
	inline void begin(uint64_t tCount, const std::string & message) {base.begin(tCount, message);}
	inline void begin(uint64_t tCount) {base.begin(tCount);}
	
	inline void end() {base.end();}
	inline void end(const std::string & message) {base.end(message);}
	
	inline void operator()(uint64_t currentCount) {base.operator()(currentCount);}
	inline void operator()(uint64_t currentCount, const std::string & message) {base.operator()(currentCount, message);}
};

}//end namespace

#endif