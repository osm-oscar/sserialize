#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/stats/statfuncs.h>
#include <sserialize/utility/checks.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <iostream>
#include <set>
#include <limits>
#include <random>

template<typename T>
std::vector<T> createNumbersSet(std::size_t count) {
	std::mt19937 gen(0xFEFE); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<T> dis(1, std::numeric_limits<T>::max());
	
	std::vector<T> ret(count, 0);
	for(T & x : ret) {
		x = dis(gen);
	}
	return ret;
}

#define ASSERT_SUM_NUM if (num != sum) { throw sserialize::BugException("Incorrect sum. SHOULD=" + std::to_string(sum) + "; IS=" + std::to_string(num) + " in line " + std::to_string(__LINE__) ); }

std::vector<long int> test32Pack(const std::vector<uint32_t> & nums, std::size_t testCount, const uint32_t sum) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint8_t> data;
		data.resize(testLength*4);
		uint8_t * dptr = data.data();
		tm.begin();
		for(std::size_t i(0); i < testLength; ++i) {
			sserialize::p_cl<uint32_t>(nums[i], dptr);
			dptr += 4;
		}
		uint32_t num = 0;
		dptr = data.data();
		for(std::size_t i(0); i < testLength; ++i) {
			num += sserialize::up_cl<uint32_t>(dptr);
			dptr += 4;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test32VlPack(const std::vector<uint32_t> & nums, std::size_t testCount, const uint32_t sum) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint8_t> data;
		data.resize(testLength*5);
		uint8_t * dptr = data.data();
		tm.begin();
		for(std::size_t i(0); i < testLength; ++i) {
			uint32_t len = sserialize::p_v<uint32_t>(nums[i], dptr, dptr+5);
			dptr += len;
		}
		uint32_t num = 0;
		dptr = data.data();
		for(std::size_t i(0); i < testLength; ++i) {
			int len = 5;
			num += sserialize::up_v<uint32_t>(dptr, dptr+5, &len);
			dptr += len;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test32UBA(const std::vector<uint32_t> & nums, std::size_t testCount, const uint32_t sum) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*4);
		std::size_t p = 0;
		tm.begin();
		for(std::size_t i(0); i < testLength; ++i) {
			uba.putUint32(p, nums[i]);
			p += 4;
		}
		uint32_t num = 0;
		p = 0;
		for(std::size_t i(0); i < testLength; ++i) {
			num += uba.getUint32(p);
			p += 4;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test32VLUBA(const std::vector<uint32_t> & nums, std::size_t testCount, const uint32_t sum) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*5);
		int len = 0;
		std::size_t p = 0;
		tm.begin();
		for(std::size_t i(0); i < testLength; ++i) {
			len = uba.putVlPackedUint32(p, nums[i]);
			p += len;
		}
		uint32_t num = 0;
		len = 0;
		p = 0;
		for(std::size_t i(0); i < testLength; ++i) {
			num += uba.getVlPackedUint32(p, &len);
			p += len;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test32Vec(const std::vector<uint32_t> & nums, std::size_t testCount, const uint32_t sum) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint32_t> data;
		data.resize(testLength);
		uint32_t * dptr = data.data();
		tm.begin();
		for(std::size_t i (0); i < testLength; ++i) {
			*dptr = nums[i];
			++dptr;
		}
		uint32_t num = 0;
		dptr = data.data();
		for(std::size_t i (0); i < testLength; ++i) {
			num += *dptr;
			++dptr;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test32VecChecked(const std::vector<uint32_t> & nums, std::size_t testCount, const uint32_t sum) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint32_t> data;
		data.reserve(testLength);
		tm.begin();
		for(std::size_t i(0); i < testLength; ++i) {
			data.push_back(nums[i]);
		}
		uint32_t num = 0;
		for(std::size_t i(0); i < testLength; ++i) {
			num += data.at(i);
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test64Pack(const std::vector<uint64_t> & nums, std::size_t testCount, const uint64_t sum) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint8_t> data;
		data.resize(testLength*8);
		uint8_t * dptr = data.data();
		tm.begin();
		for(std::size_t i(0); i < testLength; ++i) {
			sserialize::p_cl<uint64_t>(nums[i], dptr);
			dptr += 8;
		}
		uint64_t num = 0;
		dptr = data.data();
		for(std::size_t i(0); i < testLength; ++i) {
			num += sserialize::up_cl<uint64_t>(dptr);
			dptr += 8;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test64VlPack(const std::vector<uint64_t> & nums, std::size_t testCount, const uint64_t sum) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint8_t> data;
		data.resize(testLength*10);
		uint8_t * dptr = data.data();
		tm.begin();
		for(std::size_t i(0); i < testLength; ++i) {
			uint32_t len = sserialize::p_v<uint64_t>(nums[i], dptr, dptr+10);
			dptr += len;
		}
		uint64_t num = 0;
		dptr = data.data();
		for(std::size_t i(0); i < testLength; ++i) {
			int len = 10;
			num += sserialize::up_v<uint64_t>(dptr, dptr+10, &len);
			dptr += len;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}


std::vector<long int> test64UBA(const std::vector<uint64_t> & nums, std::size_t testCount, const uint64_t sum) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*8);
		std::size_t p = 0;
		tm.begin();
		for(std::size_t i(0); i < testLength; ++i) {
			uba.putUint64(p, nums[i]);
			p += 8;
		}
		uint64_t num = 0;
		p = 0;
		for(std::size_t i(0), p(0); i < testLength; ++i) {
			num += uba.getUint64(p);
			p += 8;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test64VLUBA(const std::vector<uint64_t> & nums, std::size_t testCount, const uint64_t sum) {
	std::size_t testLength = sserialize::narrow_check<int>(nums.size());
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;
	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		sserialize::UByteArrayAdapter uba(new std::vector<uint8_t>(), true);
		uba.resize(testLength*10);
		int len = 0;
		std::size_t p = 0;
		tm.begin();
		for(std::size_t i(0); i < testLength; ++i) {
			len = uba.putVlPackedUint64(p, nums[i]);
			p += len;
		}
		uint64_t num = 0;
		p = 0;
		for(std::size_t i(0); i < testLength; ++i) {
			num += uba.getVlPackedUint64(p, &len);
			p += len;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test64Vec(const std::vector<uint64_t> & nums, std::size_t testCount, const uint64_t sum) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint64_t> data;
		data.resize(testLength);
		uint64_t * dptr = data.data();
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i) {
			*dptr = nums[i];
			++dptr;
		}
		uint64_t num = 0;
		dptr = data.data();
		for(std::size_t i = 0; i < testLength; ++i) {
			num += *dptr;
			++dptr;
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}

std::vector<long int> test64VecChecked(const std::vector<uint64_t> & nums, std::size_t testCount, const uint64_t sum) {
	std::size_t testLength = nums.size();
	std::vector<long int> res;
	sserialize::TimeMeasurer tm;

	for(std::size_t testNum = 0; testNum < testCount; ++testNum) {
		std::vector<uint64_t> data;
		data.reserve(testLength);
		tm.begin();
		for(std::size_t i = 0; i < testLength; ++i) {
			data.push_back(nums[i]);
		}
		uint64_t num = 0;
		for(std::size_t i = 0; i < testLength; ++i) {
			num += data.at(i);
		}
		tm.end();
		res.push_back( tm.elapsedTime() );
		ASSERT_SUM_NUM;
	}
	return res;
}


void printTimeVector(const std::vector<long int> & v) {
	for(std::vector<long int>::const_iterator it(v.begin()); it != v.end(); ++it)
		std::cout << *it << " ";
}

int main(int argc, char ** argv) {
	if (argc < 6) {
		std::cout << "testLengthBegin testLengthEnd testLengthMul testCount (random|range)" << std::endl;
		return -1;
		
	}
	
	bool printVectors = false;
	
	int testLengthBegin = atoi(argv[1]);
	int testLengthEnd = atoi(argv[2]);
	int testLengthMul = atoi(argv[3]);
	int testCount = atoi(argv[4]);
	std::string rangeType = std::string(argv[5]);
	
	std::cout << "#TestLengthBegin: " << testLengthBegin << std::endl;
	std::cout << "#TestLengthEnd: " << testLengthEnd << std::endl;
	std::cout << "#TestLengthMul: " << testLengthMul << std::endl; 
	std::cout << "#Test Count: " << testCount << std::endl;
	std::cout << "#type: " << rangeType << std::endl;
	if (printVectors) {
		std::cout << "#Timings are in useconds" << std::endl;
	}
	else {
		std::cout << "#Entries are in M/s" << std::endl;
	}
	std::cout << "count;pc32;pvl32;uba32;uba32vl;vec32;vec32c;pc64;pvl64;uba64;uba64vl;vec64;vec64c" << std::endl;
	sserialize::TimeMeasurer tm;
	for(; testLengthBegin <= testLengthEnd; testLengthBegin *= testLengthMul) {
		
		std::vector<uint32_t> nums;
		std::vector<uint64_t> nums64;
		
		if (rangeType == "range") {
			for(int i(0); i < testLengthBegin; ++i) {
				nums.push_back(i);
				nums64.push_back(i);
			}
		}
		else {
			 nums = createNumbersSet<uint32_t>(testLengthBegin);
			 nums64 = createNumbersSet<uint64_t>(testLengthBegin);
		}
		
		uint32_t sum = std::accumulate(nums.begin(), nums.end(), uint32_t(0), std::plus<uint32_t>());
		uint64_t sum64 = std::accumulate(nums64.begin(), nums64.end(), uint64_t(0), std::plus<uint64_t>());
		
		std::vector<long int> pc32 = test32Pack(nums, testCount, sum);
		std::vector<long int> pvl32 = test32VlPack(nums, testCount, sum);
		std::vector<long int> uba32 = test32UBA(nums, testCount, sum);
		std::vector<long int> uba32vl = test32VLUBA(nums, testCount, sum);
		std::vector<long int> vec32 = test32Vec(nums, testCount, sum);
		std::vector<long int> vec32c = test32VecChecked(nums, testCount, sum);
		
		std::vector<long int> pc64 = test64Pack(nums64, testCount, sum64);
		std::vector<long int> pvl64 = test64VlPack(nums64, testCount, sum64);
		std::vector<long int> uba64 = test64UBA(nums64, testCount, sum64);
		std::vector<long int> uba64vl = test64VLUBA(nums64, testCount, sum64);
		std::vector<long int> vec64 = test64Vec(nums64, testCount, sum64);
		std::vector<long int> vec64c = test64VecChecked(nums64, testCount, sum64);
		
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
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(pc32.begin(), pc32.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(pvl32.begin(), pvl32.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(uba32.begin(), uba32.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(uba32vl.begin(), uba32vl.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(vec32.begin(), vec32.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(vec32c.begin(), vec32c.end(), int64_t(0)) << ";";
			
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(pc64.begin(), pc64.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(pvl64.begin(), pvl64.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(uba64.begin(), uba64.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(uba64vl.begin(), uba64vl.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(vec64.begin(), vec64.end(), int64_t(0)) << ";";
			std::cout << double(testLengthBegin)/sserialize::statistics::mean(vec64c.begin(), vec64c.end(), int64_t(0)) << std::endl;
		}
	}
	return 0;
}
