#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/TimeMeasuerer.h>
#include <iostream>
#include <set>

std::vector<uint32_t> createNumbersSet(uint32_t count) {
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	std::vector<uint32_t> ret;
	ret.reserve(count);
	while(ret.size() < count) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 31; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = rndNum & mask;
		ret.push_back(key);
	}
	return ret;
}

int main(int argc, char ** argv) {
	if (argc < 3)
		return -1;
	int testLength = atoi(argv[1]);
	int testCount = atoi(argv[2]);
	std::cout << "Test Length: " << testLength << std::endl;
	std::cout << "Test Count: " << testCount << std::endl;
	
	sserialize::TimeMeasurer tm;
	
	std::vector<uint32_t> nums = createNumbersSet(testLength);
	
	for(int testNum = 0; testNum < testCount; ++testNum) {
		{
			sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
			std::vector<uint32_t> vec;
			uba.resize(testLength*4);
			vec.reserve(testLength);
			tm.begin();
			for(int i = 0; i < testLength; ++i) {
				uba.putUint32(4*i, nums[i]);
			}
			uint32_t num = 0;
			for(int i = 0; i < testLength; ++i) {
				num += uba.getUint32(4*i);
			}
			tm.end();
			std::cout << "UBA-Write-Read-Time: " << tm.elapsedMilliSeconds() << std::endl;
			tm.begin();
			for(int i = 0; i < testLength; ++i) {
				vec.push_back(nums[i]);
			}
			num = 0;
			for(int i = 0; i < testLength; ++i) {
				num += vec.at(i);
			}
			tm.end();
			std::cout << "Vector-Write-Read-Time: " << tm.elapsedMilliSeconds() << std::endl;
		}
	}
	
	for(int testNum = 0; testNum < testCount; ++testNum) {
		{
			sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
			std::vector<uint64_t> vec;
			uba.resize(testLength*4);
			vec.reserve(testLength);
			tm.begin();
			for(int i = 0; i < testLength; ++i) {
				uba.putUint64(8*i, nums[i]);
			}
			uint32_t num = 0;
			for(int i = 0; i < testLength; ++i) {
				num += uba.getUint64(8*i);
			}
			tm.end();
			std::cout << "UBA-Write-Read-Time: " << tm.elapsedMilliSeconds() << std::endl;
			tm.begin();
			for(int i = 0; i < testLength; ++i) {
				vec.push_back(nums[i]);
			}
			num = 0;
			for(int i = 0; i < testLength; ++i) {
				num += vec.at(i);
			}
			tm.end();
			std::cout << "Vector-Write-Read-Time: " << tm.elapsedMilliSeconds() << std::endl;
		}
	}
}