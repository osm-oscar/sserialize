#include <sserialize/utility/ProgressInfo.h>

namespace sserialize {



void ProgressInfo::begin(uint64_t tCount, const std::string & message) {
	this->message = message;
	begin(tCount);
	operator()(0);
}

void ProgressInfo::begin(uint64_t tCount) {
	targetCount = tCount;
	startTimer = time(NULL);
	prevTimer = time(NULL);
	std::cout << std::endl;
}

void ProgressInfo::end() {
	end(message);
}

void ProgressInfo::end(const std::string & message) {
	time_t timer = time(NULL);
	time_t elapsedSecond = (timer-startTimer);
	std::cout << std::endl;
	std::cout << message << ": " << elapsedSecond << " seconds for " << targetCount;
	if (elapsedSecond)
		std::cout << "=" << targetCount/elapsedSecond << " 1/s";
	std::cout << std::endl;
}

void ProgressInfo::operator()(uint64_t currentCount) {
	(*this)(currentCount, message);
}

void ProgressInfo::operator()(uint64_t currentCount, const std::string & message) {
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

}//end namespace