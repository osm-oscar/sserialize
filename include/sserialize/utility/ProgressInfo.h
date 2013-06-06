#ifndef OSMFIND_COMMON_PROGRESS_INFO_H
#define OSMFIND_COMMON_PROGRESS_INFO_H
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

	inline void begin(uint64_t tCount, const std::string & message) {
		this->message = message;
		begin(tCount);
		operator()(0);
	}

	inline void begin(uint64_t tCount) {
		targetCount = tCount;
		startTimer = time(NULL);
		prevTimer = time(NULL);
		std::cout << std::endl;
	}

	inline void end(const std::string & message) {
			time_t timer = time(NULL);
			std::cout << std::endl;
			std::cout << message << ": " << (timer-startTimer) << " seconds for " << targetCount << std::endl;
	}

	inline void operator()(uint64_t currentCount) {
		(*this)(currentCount, message);
	}
	
	inline void operator()(uint64_t currentCount, const std::string & message) {
		time_t timer = time(NULL);
		if (timer != prevTimer) {
			std::streamsize prec = std::cout.precision();
			prevTimer = timer;
			std::cout << std::flush;
			std::cout << '\xd';
			std::cout << message << ": ";
			std::cout << currentCount  << '|' << targetCount << "=" <<  std::setprecision(4) << (double)currentCount/targetCount*100 << "%";
			std::cout << std::setprecision(prec);
			std::cout << " (" << (timer-startTimer) << "|";
			std::cout << static_cast<uint64_t>( (double)(timer-startTimer) / currentCount * (targetCount-currentCount) ) << "|";
			std::cout << static_cast<uint64_t>( (double)(timer-startTimer) / currentCount * (targetCount) ) << ")";
			std::cout << '\xd' << std::flush;
			std::cout.precision(prec);
		}
	}
};

struct ProgressInfo2 {
	uint64_t targetCount1;
	time_t startTimer1;
	std::string msg1;
	uint64_t targetCount2;
	time_t startTimer2;
	std::string msg2;

	time_t prevTimer;

	ProgressInfo2(const std::string & message1, const std::string & message2) {
		msg1 = message1;
		msg2 = message2;
	}

	inline void begin1(uint64_t tCount1) {
		targetCount1 = tCount1;
		startTimer1 = time(NULL);
		prevTimer = time(NULL);
	}


	inline void begin2(uint64_t tCount2) {
		targetCount2 = tCount2;
		startTimer2 = time(NULL);
	}

	inline void setMessage(const std::string & message1, const std::string & message2) {
		msg1 = message1;
		msg2 = message2;
	}

	inline void operator()(uint64_t currentCount1, uint64_t currentCount2) {
		time_t timer = time(NULL);
		if (timer != prevTimer) {
			prevTimer = timer;
			std::cout << std::flush;
			std::cout << '\xd';
			
			std::cout << msg2 << " ";
			
			std::cout << currentCount2  << '|' << targetCount2 << "=" << (double)currentCount2/targetCount2*100 << "%";
			std::cout << " (" << (timer-startTimer2) << "|";
			std::cout << (double)(timer-startTimer2) / currentCount2 * (targetCount2-currentCount2) << "|";
			std::cout << (double)(timer-startTimer2) / currentCount2 * (targetCount2) << ") of ";

			std::cout << msg1 << " ";

			std::cout << currentCount1  << '|' << targetCount1 << "=" << (double)currentCount1/targetCount1*100 << "%";
			std::cout << " (" << (timer-startTimer1) << "|";
			std::cout << (double)(timer-startTimer1) / currentCount1 * (targetCount1-currentCount1) << "|";
			std::cout << (double)(timer-startTimer1) / currentCount1 * (targetCount1) << ")";

			std::cout << '\xd' << std::flush;
		}
	}
};

}//end namespace

#endif