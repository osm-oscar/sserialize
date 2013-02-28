#include <Static/Map.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <set>
#include <Static/Set.h>
#include "datacreationfuncs.h"

using namespace sserialize;

template<typename TValue>
bool testSet(std::set<TValue> & realValues) {
	UByteArrayAdapter adap(new std::deque<uint8_t>(), true);
	adap << realValues;

	sserialize::Static::Set<TValue> sset;
	adap >> sset;
	return (sset == realValues);
}

void fillSet(std::set<int32_t> & set) {
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	for(uint32_t i = 0; i < 1024; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 30; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		int32_t key = (rndNum & mask) * ((rand() & 0x1) ? -1 : 1);
		set.insert(key);
	}
}

void fillSet(std::set<uint16_t> & set) {
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	for(uint32_t i = 0; i < 1024; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 16; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = rndNum & mask;
		set.insert(key);
	}
}


bool testInt32() {
	srand(0);
	std::deque<uint8_t> data;
	UByteArrayAdapter adapter(&data);
	std::set<int32_t> set;
	fillSet(set);
	adapter << set;

	sserialize::Static::Set<int32_t> staticSet(adapter);

	for(std::set<int32_t>::iterator it = set.begin(); it != set.end(); it++) {
		if (!staticSet.contains(*it)) {
			std::cout << "StaticMap wiht int32 is broken" << std::endl;
			return false;
		}
	}
	return true;
}

bool testUint16() {
	srand(0);
	std::deque<uint8_t> data;
	UByteArrayAdapter adapter(&data);
	std::set<uint16_t> set;
	fillSet(set);
	adapter << set;

	sserialize::Static::Set<uint16_t> staticSet(adapter);

	for(std::set<uint16_t>::iterator it = set.begin(); it != set.end(); it++) {
		if (!staticSet.contains(*it)) {
			std::cout << "StaticMap wiht uint16_t is broken" << std::endl;
			return false;
		}
	}
	return true;
}

int main() {
	bool allOk = true;

	std::set<uint32_t> nums = createNumbersSet(1023);
	if (testSet(nums)) {
		std::cout << "Passed number test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed number test" << std::endl;
	}

	std::set<std::string> strs = createStringsSet(33, 1023);
	if (testSet(strs)) {
		std::cout << "Passed string test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed string test" << std::endl;
	}

	if (testInt32()) {
		std::cout << "Passed StaticSet with Int32" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "FAILED StaticSet with Int32" << std::endl;
	}

	if (testUint16()) {
		std::cout << "PASSED StaticSet with uint16" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "FAILED StaticSet with uint16." << std::endl;
	}

	if (allOk)
		std::cout << "PASSED all test." << std::endl;
	else
		std::cout << "FAILED at least one test!" << std::endl;

	return 0;
}