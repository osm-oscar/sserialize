#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/stats/statfuncs.h>
#include <sserialize/utility/checks.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <iostream>
#include <set>
#include <limits>

std::vector<uint32_t> createNumbersSet(std::size_t count) {
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

std::vector<uint64_t> createNumbersSet64(std::size_t count) {
	//Fill the first
	uint64_t rndNum;
	uint64_t rndMask;
	uint64_t mask;
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

std::vector<long int> test32Pack(const std::vector<uint32_t> & nums, std::size_t testCount) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint8_t> data;
		data.resize(testLength*4);
		uint8_t * dptr = data.data();
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i, dptr += 4) {
			sserialize::p_cl<uint32_t>(nums[i], dptr);
		}
		uint32_t num = 0;
		for(uint8_t* iptr(data.data()), * eptr(dptr); iptr < eptr; iptr += 4) {
			num += sserialize::up_cl<uint32_t>(iptr);
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test32VlPack(const std::vector<uint32_t> & nums, std::size_t testCount) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint8_t> data;
		data.resize(testLength*5);
		uint8_t * dptr = data.data();
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i) {
			uint32_t len = sserialize::p_v<uint32_t>(nums[i], dptr);
			dptr += len;
		}
		uint32_t num = 0;
		for(uint8_t* iptr(data.data()), * eptr(dptr); iptr < eptr;) {
			int len = 5;
			num += sserialize::up_v<uint32_t>(iptr, &len);
			iptr += len;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test32UBA(const std::vector<uint32_t> & nums, std::size_t testCount) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*4);
		tm.begin();
		for(std::size_t i(0), p(0); i < testLength; ++i, p += 4) {
			uba.putUint32(p, nums[i]);
		}
		uint32_t num = 0;
		for(std::size_t i(0), p(0); i < testLength; ++i, p += 4) {
			num += uba.getUint32(p);
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test32VLUBA(const std::vector<uint32_t> & nums, std::size_t testCount) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*4);
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i) {
			uba.putVlPackedUint32(nums[i]);
		}
		uint32_t num = 0;
		for(std::size_t i = 0; i < testLength; ++i) {
			num += uba.getVlPackedUint32();
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test32Vec(const std::vector<uint32_t> & nums, std::size_t testCount) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint32_t> vec;
		vec.resize(testLength);
		uint32_t * dptr = vec.data();
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i, ++dptr) {
			*dptr = nums[i];
		}
// 		::memcpy(vec.data(), nums.data(), sizeof(uint32_t)*nums.size());
		uint32_t num = 0;
		for(uint32_t * iptr(vec.data()), * eptr(dptr); iptr < eptr; ++iptr) {
			num += *iptr;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test32VecChecked(const std::vector<uint32_t> & nums, std::size_t testCount) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint32_t> vec;
		vec.reserve(testLength);
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i) {
			vec.push_back(nums[i]);
		}
		uint32_t num = 0;
		for(std::size_t i = 0; i < testLength; ++i) {
			num += vec.at(i);
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test64Pack(const std::vector<uint64_t> & nums, std::size_t testCount) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint8_t> data;
		data.resize(testLength*8);
		uint8_t * dptr = data.data();
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i, dptr += 8) {
			sserialize::p_cl<uint64_t>(nums[i], dptr);
		}
		uint64_t num = 0;
		for(uint8_t* iptr(data.data()), * eptr(dptr); iptr < eptr; iptr += 8) {
			num += sserialize::up_cl<uint64_t>(iptr);
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test64VlPack(const std::vector<uint64_t> & nums, std::size_t testCount) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint8_t> data;
		data.resize(testLength*9);
		uint8_t * dptr = data.data();
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i) {
			uint32_t len = sserialize::p_v<uint64_t>(nums[i], dptr);
			dptr += len;
		}
		uint32_t num = 0;
		for(uint8_t* iptr(data.data()), * eptr(dptr); iptr < eptr;) {
			int len = 9;
			num += sserialize::up_v<uint64_t>(iptr, &len);
			iptr += len;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}


std::vector<long int> test64UBA(const std::vector<uint64_t> & nums, std::size_t testCount) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*8);
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i) {
			uba.putUint64(8*i, nums[i]);
		}
		uint64_t num = 0;
		for(std::size_t i = 0; i < testLength; ++i) {
			num += uba.getUint64(8*i);
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test64VLUBA(const std::vector<uint64_t> & nums, std::size_t testCount) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*8);
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i) {
			uba.putVlPackedUint64(nums[i]);
		}
		uint64_t num = 0;
		for(std::size_t i = 0; i < testLength; ++i) {
			num += uba.getVlPackedUint64();
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test64Vec(const std::vector<uint64_t> & nums, std::size_t testCount) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint64_t> vec;
		vec.resize(testLength);
		uint64_t * dptr = vec.data();
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i, ++dptr) {
			*dptr = nums[i];
		}
// 		::memmove(vec.data(), nums.data(), sizeof(uint64_t)*nums.size());
		uint64_t num = 0;
		for(uint64_t * iptr(vec.data()), * eptr(dptr); iptr < eptr; ++iptr) {
			num += *iptr;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}

std::vector<long int> test64VecChecked(const std::vector<uint64_t> & nums, std::size_t testCount) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint64_t> vec;
		vec.reserve(testLength);
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i) {
			vec.push_back(nums[i]);
		}
		uint64_t num = 0;
		for(std::size_t i = 0; i < testLength; ++i) {
			num += vec.at(i);
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
	}
	return res;
}


void printTimeVector(const std::vector<long int> & v) {
	for(std::vector<long int>::const_iterator it(v.begin()); it != v.end(); ++it)
		std::cout << *it << " ";
}

int main(int argc, char ** argv) {
	if (argc < 3) {
		std::cout << "testLengthBegin testLengthEnd testLengthMul testCount" << std::endl;
		return -1;
		
	}
	
	bool printVectors = false;
	
	int testLengthBegin = atoi(argv[1]);
	int testLengthEnd = atoi(argv[2]);
	int testLengthMul = atoi(argv[3]);
	int testCount = atoi(argv[4]);
	
	std::cout << "#TestLengthBegin: " << testLengthBegin << std::endl;
	std::cout << "#TestLengthEnd: " << testLengthEnd << std::endl;
	std::cout << "#TestLengthMul: " << testLengthMul << std::endl; 
	std::cout << "#Test Count: " << testCount << std::endl;
	if (printVectors) {
		std::cout << "#Timings are in useconds" << std::endl;
	}
	else {
		std::cout << "#Entries are in M/s" << std::endl;
	}
	std::cout << "count;pc32;pvl32;uba32;uba32vl;vec32;vec32c;pc64;pvl64;uba64;uba64vl;vec64;vec64c" << std::endl;
	sserialize::TimeMeasurer tm;
	for(; testLengthBegin <= testLengthEnd; testLengthBegin *= testLengthMul) {
		
		std::vector<uint32_t> nums = createNumbersSet(testLengthBegin);
		std::vector<uint64_t> nums64 = createNumbersSet64(testLengthBegin);
		
		
		std::vector<long int> pc32 = test32Pack(nums, testCount);
		std::vector<long int> pvl32 = test32VlPack(nums, testCount);
		std::vector<long int> uba32 = test32UBA(nums, testCount);
		std::vector<long int> uba32vl = test32VLUBA(nums, testCount);
		std::vector<long int> vec32 = test32Vec(nums, testCount);
		std::vector<long int> vec32c = test32VecChecked(nums, testCount);
		
		std::vector<long int> pc64 = test64Pack(nums64, testCount);
		std::vector<long int> pvl64 = test64VlPack(nums64, testCount);
		std::vector<long int> uba64 = test64UBA(nums64, testCount);
		std::vector<long int> uba64vl = test64VLUBA(nums64, testCount);
		std::vector<long int> vec64 = test64Vec(nums64, testCount);
		std::vector<long int> vec64c = test64VecChecked(nums64, testCount);
		
		if (printVectors) {
			
			std::cout << "P32:";
			printTimeVector(pc32);
			std::cout << std::endl;
			
			std::cout << "PVL32:";
			printTimeVector(pvl32);
			std::cout << std::endl;
		
			std::cout << "UBA32:";
			printTimeVector(uba32);
			std::cout << std::endl;
			
			std::cout << "UBA32VL:";
			printTimeVector(uba32vl);
			std::cout << std::endl;
			
			std::cout << "VEC32:";
			printTimeVector(vec32);
			std::cout << std::endl;
			
			std::cout << "VEC32C:";
			printTimeVector(vec32c);
			std::cout << std::endl;
			
			std::cout << "P64:";
			printTimeVector(pc64);
			std::cout << std::endl;
			
			std::cout << "PVL64:";
			printTimeVector(pvl64);
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
			
			std::cout << "VEC64C:";
			printTimeVector(vec64c);
			std::cout << std::endl;
		}
		else {
			std::cout << testLengthBegin << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(pc32.begin(), pc32.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(pvl32.begin(), pvl32.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(uba32.begin(), uba32.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(uba32vl.begin(), uba32vl.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(vec32.begin(), vec32.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(vec32c.begin(), vec32c.end(), 0) << ";";
			
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(pc64.begin(), pc64.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(pvl64.begin(), pvl64.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(uba64.begin(), uba64.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(uba64vl.begin(), uba64vl.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(vec64.begin(), vec64.end(), 0) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(vec64c.begin(), vec64c.end(), 0) << std::endl;
		}
	}
	return 0;
}
