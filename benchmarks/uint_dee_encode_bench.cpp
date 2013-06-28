#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/TimeMeasuerer.h>
#include <sserialize/utility/statfuncs.h>
#include <iostream>
#include <set>
#include <limits>

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

std::vector<uint64_t> createNumbersSet64(uint32_t count) {
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	std::vector<uint64_t> ret;
	ret.reserve(count);
	while(ret.size() < count) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 63; 
		mask = ((rndMask+1 == 64) ? std::numeric_limits<uint64_t>::max() : ((static_cast<uint64_t>(1) << (rndMask+1)) - 1));
		uint64_t key = rndNum & mask;
		ret.push_back(key);
	}
	return ret;
}


std::vector<long int> test32UBA(const std::vector<uint32_t> & nums, int testCount) {
	int testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(int testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*4);
		tm.begin();
		for(int i = 0; i < testLength; ++i) {
			uba.putUint32(4*i, nums[i]);
		}
		uint32_t num = 0;
		for(int i = 0; i < testLength; ++i) {
			num += uba.getUint32(4*i);
		}
		tm.end();
		res.push_back( tm.elapsedMilliSeconds() );
	}
	return res;
}

std::vector<long int> test32VLUBA(const std::vector<uint32_t> & nums, int testCount) {
	int testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(int testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*4);
		tm.begin();
		for(int i = 0; i < testLength; ++i) {
			uba.putVlPackedUint32(nums[i]);
		}
		uint32_t num = 0;
		for(int i = 0; i < testLength; ++i) {
			num += uba.getVlPackedUint32();
		}
		tm.end();
		res.push_back( tm.elapsedMilliSeconds() );
	}
	return res;
}

std::vector<long int> test32Vec(const std::vector<uint32_t> & nums, int testCount) {
	int testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(int testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint32_t> vec;
		vec.reserve(testLength);
		tm.begin();
		for(int i = 0; i < testLength; ++i) {
			vec.push_back(nums[i]);
		}
		uint32_t num = 0;
		for(int i = 0; i < testLength; ++i) {
			num += vec[i];
		}
		tm.end();
		res.push_back( tm.elapsedMilliSeconds() );
	}
	return res;
}

std::vector<long int> test64UBA(const std::vector<uint64_t> & nums, int testCount) {
	int testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(int testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*8);
		tm.begin();
		for(int i = 0; i < testLength; ++i) {
			uba.putUint64(8*i, nums[i]);
		}
		uint64_t num = 0;
		for(int i = 0; i < testLength; ++i) {
			num += uba.getUint64(8*i);
		}
		tm.end();
		res.push_back( tm.elapsedMilliSeconds() );
	}
	return res;
}

std::vector<long int> test64VLUBA(const std::vector<uint64_t> & nums, int testCount) {
	int testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(int testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*8);
		tm.begin();
		for(int i = 0; i < testLength; ++i) {
			uba.putVlPackedUint64(nums[i]);
		}
		uint64_t num = 0;
		for(int i = 0; i < testLength; ++i) {
			num += uba.getVlPackedUint64();
		}
		tm.end();
		res.push_back( tm.elapsedMilliSeconds() );
	}
	return res;
}

std::vector<long int> test64Vec(const std::vector<uint64_t> & nums, int testCount) {
	int testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(int testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint64_t> vec;
		vec.reserve(testLength);
		tm.begin();
		for(int i = 0; i < testLength; ++i) {
			vec.push_back(nums[i]);
		}
		uint64_t num = 0;
		for(int i = 0; i < testLength; ++i) {
			num += vec[i];
		}
		tm.end();
		res.push_back( tm.elapsedMilliSeconds() );
	}
	return res;
}

void printTimeVector(const std::vector<long int> & v) {
	for(std::vector<long int>::const_iterator it(v.begin()); it != v.end(); ++it)
		std::cout << *it << " ";
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
	std::vector<uint64_t> nums64 = createNumbersSet64(testLength);
	
	std::vector<long int> uba32 = test32UBA(nums, testCount);
	std::vector<long int> uba32vl = test32VLUBA(nums, testCount);
	std::vector<long int> vec32 = test32Vec(nums, testCount);
	std::vector<long int> uba64 = test64UBA(nums64, testCount);
	std::vector<long int> uba64vl = test64VLUBA(nums64, testCount);
	std::vector<long int> vec64 = test64Vec(nums64, testCount);
	
	std::cout << "UBA32:";
	printTimeVector(uba32);
	std::cout << std::endl;
	
	std::cout << "UBA32VL:";
	printTimeVector(uba32vl);
	std::cout << std::endl;
	
	std::cout << "VEC32:";
	printTimeVector(vec32);
	std::cout << std::endl;

	std::cout << "UBA64:";
	printTimeVector(uba64);
	std::cout << std::endl;
	
	std::cout << "UBA64VL:";
	printTimeVector(uba64vl);
	std::cout << std::endl;
	
	std::cout << "VEC64:";
	printTimeVector(vec64);
	std::cout << std::endl;
	
	

}