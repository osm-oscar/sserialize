#include <utility/CompactUintArray.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

using namespace sserialize;

bool checkCompactArray(std::deque<uint32_t> & real, std::deque<uint8_t> & coded, uint8_t bpn) {
	uint8_t * arr = new uint8_t[coded.size()];
	for(size_t i = 0; i < coded.size(); i++) {
		arr[i] = coded.at(i);
	}
	UByteArrayAdapter arrAdap(arr, 0, coded.size());
	CompactUintArray compArr(arrAdap, bpn);
	for(size_t i = 0; i < real.size(); i++) {
		uint32_t realVal = real.at(i);
		uint32_t codedVal = compArr.at(i);
		if (codedVal != realVal) {
			compArr.at(i);
			return false;
		}
	}
	return true;
}

int main() {
	
	std::deque<uint8_t> compArrays[32];
	std::deque<uint32_t> compSrcArrays[32];

	
	//Fill the first
	srand( 0 );
	uint32_t rndNum;
	uint32_t mask;
	uint32_t testCount = 0xFFFF;
	for(uint32_t i = 0; i < testCount; i++) {
		rndNum = rand();
		for(int j=0; j < 32; j++) {
			mask = ((j+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (j+1)) - 1));
			compSrcArrays[j].push_back(rndNum & mask);
		}
	}
	
	//Test single
	{
		std::deque<uint8_t> singleCompArray;
		uint32_t bits = 8;
		std::cout << "Creating array for bit length " << bits << "...";
		if (!CompactUintArray::createFromSet(compSrcArrays[bits-1], singleCompArray, bits)) {
			std::cout << "Failed!" << std::endl;
		}
		else {
			std::cout << "Passed!" << std::endl;
		}
		std::cout << "Testing bit length: " << bits << "...";
		if (checkCompactArray(compSrcArrays[bits-1], singleCompArray, bits)) std::cout << "passed" << std::endl;
		else std::cout << "failed!" << std::endl;
	}

	for(int i = 0; i < 32; i++) {
		std::cout << "Creating array for bit length " << i+1 << "...";
		if (!CompactUintArray::createFromSet(compSrcArrays[i], compArrays[i], i+1)) {
			std::cout << "Failed!" << std::endl;
		}
		else {
			std::cout << "Passed!" << std::endl;
		}
	}
	
	
	for(int i = 0; i < 32; i++) {
		std::cout << "Testing bit length: " << i+1 << "...";
		if (checkCompactArray(compSrcArrays[i], compArrays[i], i+1)) std::cout << "passed" << std::endl;
		else std::cout << "failed!" << std::endl;
	}

	std::cout << "Checking same sets with set() function" << std::endl;
	std::deque<uint8_t> compSetFuncArrays[32];
	UByteArrayAdapter compArrayAdapters[32];
	CompactUintArray compUintArrays[32];
	for(int i =0; i < 32; i++) {
		compSetFuncArrays[i] = std::deque<uint8_t>(compArrays[i].size(), 0xFF);
		compArrayAdapters[i] = UByteArrayAdapter(&(compSetFuncArrays[i]), 0, compSetFuncArrays[i].size());
		compUintArrays[i] = CompactUintArray(compArrayAdapters[i], i+1);
	}

	for(int i = 0; i < 32; i++) {
		std::cout << "Creating array for bit length " << i+1 << "...";
		uint32_t count = 0;
		uint32_t value;
		bool allOk = true;
		for(size_t j = 0; j < compSrcArrays[i].size(); j++) {
			value = compSrcArrays[i][j];
			uint32_t ret = compUintArrays[i].set(count, value);
			count++;
			if (ret != value) {
				allOk = false;
			}
		}
		if (allOk) {
			std::cout << "Passed!" << std::endl;
		}
		else {
			std::cout << "Failed!" << std::endl;
		}
	}
	
	
	for(int i = 0; i < 32; i++) {
		std::cout << "Testing bit length: " << i+1 << "...";
		if (checkCompactArray(compSrcArrays[i], compSetFuncArrays[i], i+1)) std::cout << "passed" << std::endl;
		else std::cout << "failed!" << std::endl;
	}

	return 0;
}