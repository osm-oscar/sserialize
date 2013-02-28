#include <sserialize/Static/Map.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <map>
#include "datacreationfuncs.h"

using namespace sserialize;

std::map<std::string, uint32_t> createStringNumberMap(uint32_t maxStrLen, uint32_t count) {
	std::deque<std::string> strs(createStrings(maxStrLen, count));
	std::deque<uint32_t> numbers(createNumbers(count));
	std::map<std::string, uint32_t> map;
	for(size_t i = 0; i < strs.size(); i++) {
		map[strs[i]] = numbers[i];
	}
	return map;
}

std::map<std::string, uint16_t> createStringNumberMap16(uint32_t maxStrLen, uint32_t count) {
	std::deque<std::string> strs(createStrings(maxStrLen, count));
	std::deque<uint16_t> numbers(createNumbers16(count));
	std::map<std::string, uint16_t> map;
	for(size_t i = 0; i < strs.size(); i++) {
		map[strs[i]] = numbers[i];
	}
	return map;
}


std::map<uint32_t, std::string> createNumberStringMap(uint32_t maxStrLen, uint32_t count) {
	std::deque<std::string> strs(createStrings(maxStrLen, count));
	std::deque<uint32_t> numbers(createNumbers(count));
	std::map<uint32_t, std::string> map;
	for(size_t i = 0; i < strs.size(); i++) {
		map[numbers[i]] = strs[i];
	}
	return map;
}

template<typename TKey, typename TValue>
bool testMap(std::map<TKey, TValue> & realValues) {
	UByteArrayAdapter adap(new std::deque<uint8_t>(), true);
	adap << realValues;

	sserialize::Static::Map<TKey, TValue> smap;
	adap >> smap;
	return (smap == realValues);
}

void fillMap(std::map<uint32_t, uint32_t> & map) {
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	for(uint32_t i = 0; i < 1024; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 31; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = rndNum & mask;
		uint32_t value = rand();
		map[key] = value;
	}
}

int main() {
	bool allOk = true;

	std::map<std::string, uint32_t> map = createStringNumberMap(33, 1023);
	if (testMap(map)) {
		std::cout << "Passed <std::string,uint32_t> test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed <std::string,uint32_t> test" << std::endl;
	}

	std::map<std::string, uint16_t> map16 = createStringNumberMap16(33, 1023);
	if (testMap(map16)) {
		std::cout << "Passed <std::string,uint16_t> test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed <std::string,uint16_t> test" << std::endl;
	}

	std::map<uint32_t, std::string> map2 = createNumberStringMap(33, 1023);
	if (testMap(map2)) {
		std::cout << "Passed <uint32_t, std::string> test" << std::endl;
	}
	else {
		allOk = false;
		std::cout << "Failed <uint32_t, std::string> test" << std::endl;
	}



	if (allOk)
		std::cout << "PASSED all test." << std::endl;
	else
		std::cout << "FAILED at least one test!" << std::endl;

	return 0;
}