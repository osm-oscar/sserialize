#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/TimeMeasuerer.h>
#include <iostream>

std::vector<uint32_t> createNumbersSet(uint32_t count) {
	std::set<uint32_t> ret;
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	while( ret.size() < count) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 31; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = rndNum & mask;
		ret.insert(key);
	}
	return std::vector<uint32_t>(ret.begin(), ret.end());
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
			for(uint32_t i = 0; i < testLength; ++i) {
				uba.putUint32(nums[i]);
			}
			uint32_t num = 0;
			for(uint32_t i = 0; i < testLength; ++i) {
				num += uba.getUint32(4*i);
			}
			tm.end();
			std::cout << "UBA-Write-Read-Time: " << tm.elapsedMilliSeconds() << std::endl;
			tm.begin();
			for(uint32_t i = 0; i < testLength; ++i) {
				vec.push_back(nums[i]);
			}
			uint32_t num = 0;
			for(uint32_t i = 0; i < testLength; ++i) {
				num += vec[i];
			}
			tm.end();
			std::cout << "Vector-Write-Read-Time: " << tm.elapsedMilliSeconds() << std::endl;
		}
	}
	
}