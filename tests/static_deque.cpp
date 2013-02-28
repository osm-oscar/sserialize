#include <sserialize/Static/Deque.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <deque>
#include "datacreationfuncs.h"

using namespace sserialize;

std::deque< std::deque<uint32_t> > createDequeOfDequeNumbers(uint32_t maxCountPerDeque, uint32_t maxDequeCount) {
	std::deque< std::deque<uint32_t> > res;
	for(size_t i = 0; i < maxDequeCount; i++) {
		uint32_t tmpCount = (double)rand()/RAND_MAX * maxCountPerDeque;
		res.push_back(createNumbers(tmpCount));
	}
	return res;
}

template<typename TValue>
bool testDeque(std::deque<TValue> & realValues) {
	UByteArrayAdapter adap(new std::deque<uint8_t>(), true);
	adap << realValues;

	sserialize::Static::Deque<TValue> sdeque;
	adap >> sdeque;
	return (sdeque == realValues) && (adap.size() == sdeque.getSizeInBytes());
}

template<typename TValue>
bool testDequeCreatorRawPut(std::deque<TValue> & realValues) {
	UByteArrayAdapter adap(new std::vector<uint8_t>(), true);
	Static::DequeCreator<TValue> creator(adap);
	for(size_t i = 0; i< realValues.size(); ++i) {
		creator.beginRawPut();
		creator.rawPut() << realValues[i];
		creator.endRawPut();
	}
	creator.flush();
	sserialize::Static::Deque<TValue> sdeque;
	adap >> sdeque;
	return (sdeque == realValues) && (adap.size() == sdeque.getSizeInBytes());
}


bool testDeque(std::deque< std::deque<uint32_t> > & realValues) {
	UByteArrayAdapter adap(new std::deque<uint8_t>(), true);
	adap << realValues;

	sserialize::Static::Deque< sserialize::Static::Deque<uint32_t> > sdeque;
	adap >> sdeque;
	if (realValues.size() != sdeque.size())
		return false;
	for(size_t i = 0; i < realValues.size(); i++) {
		if (sdeque.at(i) != realValues.at(i))
			return false;
	}
	return adap.size() == sdeque.getSizeInBytes();
}

int main() {
	srand(0);
	bool allOk = true;
	
	std::deque<uint32_t> nums = createNumbers(1023);
	if (testDeque(nums)) {
		std::cout << "Passed number test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed number test" << std::endl;
	}

	if (testDequeCreatorRawPut(nums)) {
		std::cout << "Passed DequeCreatorRawPut number test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed DequeCreatorRawPut number test" << std::endl;
	}


	std::deque<std::string> strs = createStrings(33, 1023);
	if (testDeque(strs)) {
		std::cout << "Passed string test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed string test" << std::endl;
	}

	strs = createStrings(0xFFFF, 0xFF);
	if (testDeque(strs)) {
		std::cout << "Passed large string test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed large string test" << std::endl;
	}
	
	strs = createStrings(0xFFFF, 0xFF);
	if (testDequeCreatorRawPut(strs)) {
		std::cout << "Passed DequeCreatorRawPut large string test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed DequeCreatorRawPut large string test" << std::endl;
	}
	
	
	std::deque< std::deque<uint32_t> > dequeOfDequeNums = createDequeOfDequeNumbers(129, 33);
	if (testDeque(dequeOfDequeNums)) {
		std::cout << "Passed dequeOfDequeNums test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed dequeOfDequeNums test" << std::endl;
	}

	if (allOk)
		std::cout << "PASSED all test." << std::endl;
	else
		std::cout << "FAILED at least one test!" << std::endl;
}