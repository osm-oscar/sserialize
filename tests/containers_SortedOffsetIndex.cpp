#include <vector>
#include <fstream>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/iterator/RangeGenerator.h>
#include <sserialize/storage/MmappedFile.h>
#include <sserialize/stats/statfuncs.h>
#include "datacreationfuncs.h"
#include "TestBase.h"

using namespace sserialize;
using SizeType = Static::SortedOffsetIndex::SizeType;


std::set<uint32_t> myCreateNumbers(uint32_t count) {
	std::set<uint32_t> deque;
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	for(uint32_t i = 0; i < count; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 31; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = rndNum & mask;
		deque.insert(key);
	}
	return deque;
}

std::set<uint32_t> myCreateNumbers(uint32_t count, uint32_t maxNum) {
	std::set<uint32_t> ret;
	uint32_t rndNum;
	for(uint32_t i = 0; i < count; i++) {
		rndNum = (double)rand()/RAND_MAX * maxNum;
		ret.insert(rndNum);
	}
	return ret;
}

std::set<uint64_t> largeOffsets(uint32_t count, uint32_t minDistance) {
	uint32_t maxAdd = std::numeric_limits<uint32_t>::max()/2 - minDistance;
	std::set<uint64_t> ret;
	ret.insert(0);
	uint64_t last = 0;
	for(uint32_t i = 0; i < count; i++) {
		uint32_t rndNum = (double)rand()/RAND_MAX * maxAdd;
		last += minDistance + rndNum;
		ret.insert(last);
	}
	return ret;
}

std::string inFile;

class SortedOffsetIndexTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( SortedOffsetIndexTest );
CPPUNIT_TEST( testLinearRegression );
CPPUNIT_TEST( testLinearRegressionLarge );
CPPUNIT_TEST( testEmptyEquality );
CPPUNIT_TEST( testRandomEquality );
CPPUNIT_TEST( testLargeOffsets );
CPPUNIT_TEST( testLargestOffsetsSpecial );
CPPUNIT_TEST( testFromFile );
CPPUNIT_TEST( testRegularOffsets );
CPPUNIT_TEST( testSpecialOffsets );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	
	void testLinearRegression() {
		uint64_t size = uint64_t(1024);
		uint64_t stride = 4;
		sserialize::RangeGenerator<uint64_t> realValues(0, size*stride, stride);
		double slope;
		double yintercept;
		sserialize::statistics::linearRegression(realValues.begin(), realValues.end(), slope, yintercept);
		CPPUNIT_ASSERT(std::abs<double>(yintercept-0.0) <= 0.1);
		CPPUNIT_ASSERT(std::abs<double>(slope-4.0) <= 0.1);
	}
	
	void testLinearRegressionLarge() {
		uint64_t size = uint64_t(std::numeric_limits<uint32_t>::max()) + 1024;
		uint64_t stride = 4;
		sserialize::RangeGenerator<uint64_t> realValues(0, size*stride, stride);
		double slope;
		double yintercept;
		sserialize::statistics::linearRegression<sserialize::RangeGenerator<uint64_t>::const_iterator, uint64_t, double, true>(realValues.begin(), realValues.end(), slope, yintercept);
		CPPUNIT_ASSERT(std::abs<double>(yintercept-0.0) <= 0.1);
		CPPUNIT_ASSERT(std::abs<double>(slope-4.0) <= 0.1);
	}
	
	void testEmptyEquality() {
		srand(0);
			std::set<uint32_t> realValues;
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			Static::SortedOffsetIndexPrivate::create(realValues, dest);
			Static::SortedOffsetIndex idx(dest);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", SizeType(realValues.size()), idx.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("sizeInBytes", (OffsetType)dest.size(), idx.getSizeInBytes());
	}
	
	void testRandomEquality() {
		srand(0);
		uint32_t setCount = 2048;

		for(size_t i = 0; i < setCount; i++) {
			std::set<uint32_t> realValues( myCreateNumbers(rand() % setCount) );
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			Static::SortedOffsetIndexPrivate::create(realValues, dest);
			Static::SortedOffsetIndex idx(dest);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", SizeType(realValues.size()), idx.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("sizeInBytes", (OffsetType)dest.size(), idx.getSizeInBytes());
		
			std::size_t count = 0;
			for(std::set<uint32_t>::iterator it = realValues.begin(); it != realValues.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "id at " << count;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), (OffsetType)*it, idx.at(count));
			}
		}
	}
	

	void testLargeOffsets() {
		srand(0);
		uint32_t setCount = 8;

		for(std::size_t i = 0; i < setCount; i++) {
			std::set<uint64_t> realValues( largeOffsets(2*1000*1000, std::numeric_limits<uint32_t>::max()/128) );
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			CPPUNIT_ASSERT_MESSAGE("creation", Static::SortedOffsetIndexPrivate::create(realValues, dest));
			Static::SortedOffsetIndex idx(dest);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", SizeType(realValues.size()), idx.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("sizeInBytes", (OffsetType)dest.size(), idx.getSizeInBytes());
		
			std::size_t count = 0;
			for(std::set<uint64_t>::iterator it = realValues.begin(); it != realValues.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "id at " << count;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, idx.at(count));
			}
		}
	}
	
	void testLargestOffsetsSpecial() {
		for(int i = 0; i < 32; ++i) {
			uint64_t count = 0xFFFFF;
			uint64_t begin = (uint32_t)rand();
			uint64_t end = begin * ((uint32_t)rand());
			uint64_t add = (end-begin)/count;
			std::set<uint64_t> realValues;
			std::set<uint64_t>::iterator it(realValues.end());
			for(; begin < end; begin += add) {
				it = realValues.insert(it, begin);
			}
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			CPPUNIT_ASSERT_MESSAGE("creation", Static::SortedOffsetIndexPrivate::create(realValues, dest));
			Static::SortedOffsetIndex idx(dest);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", SizeType(realValues.size()), idx.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("sizeInBytes", (OffsetType)dest.size(), idx.getSizeInBytes());
		
			count = 0;
			for(std::set<uint64_t>::iterator it = realValues.begin(); it != realValues.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "id at " << count;
				CPPUNIT_ASSERT(count <= std::numeric_limits<uint32_t>::max());
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, idx.at(count));
			}
		}
	}
	
	void testRegularOffsets() {
		for(uint64_t size = 10000; size < 10000000; size *= 10) {
			for(uint64_t stride = 1; stride < 15; ++stride) {
				sserialize::RangeGenerator<uint64_t> realValues(0, size*stride, stride);
				UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
				CPPUNIT_ASSERT_MESSAGE("creation", Static::SortedOffsetIndexPrivate::create(realValues, dest));
				Static::SortedOffsetIndex idx(dest);
			
				CPPUNIT_ASSERT_EQUAL_MESSAGE("size", SizeType(realValues.size()), idx.size());
				CPPUNIT_ASSERT_EQUAL_MESSAGE("sizeInBytes", (OffsetType)dest.size(), idx.getSizeInBytes());
			
				std::size_t count = 0;
				for(auto it = realValues.begin(), end = realValues.end(); it != end; ++it, ++count) {
					std::stringstream ss;
					ss << "id at " << count;
					CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, idx.at(count));
				}
			}
		}
	}
	
	void testSpecialOffsets() {
		uint64_t size = uint64_t(std::numeric_limits<uint32_t>::max()) + 1024;
		uint64_t stride = 4;
		sserialize::RangeGenerator<uint64_t> realValues(0, size*stride, stride);
		UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
		CPPUNIT_ASSERT_MESSAGE("creation", Static::SortedOffsetIndexPrivate::create(realValues, dest));
		Static::SortedOffsetIndex idx(dest);
	
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", SizeType(realValues.size()), idx.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("sizeInBytes", (OffsetType)dest.size(), idx.getSizeInBytes());
	
		std::size_t count = 0;
		for(auto it = realValues.begin(), end = realValues.end(); it != end; ++it, ++count) {
			std::stringstream ss;
			ss << "id at " << count;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, idx.at(count));
		}
	}
	
	void testFromFile() {
		if (!inFile.empty()) {
			std::ifstream file;
			file.open(inFile);
			CPPUNIT_ASSERT_MESSAGE("Test file could not be opened", file.is_open());
			
			std::vector<uint64_t> values;
			while (!file.eof()) {
				uint64_t v;
				file >> v;
				values.push_back(v);
			}
			UByteArrayAdapter d(UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
			Static::SortedOffsetIndexPrivate::create(values, d);
			Static::SortedOffsetIndex idx(d);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", sserialize::Static::SortedOffsetIndex::SizeType(values.size()), idx.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("sizeInBytes", (OffsetType)d.size(), idx.getSizeInBytes());
		
			std::size_t count = 0;
			for(std::vector<uint64_t>::iterator it = values.begin(); it != values.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "id at " << count;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, idx.at(count));
			}
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	
	if (argc > 1 && sserialize::MmappedFile::fileExists(argv[1])) {
		inFile = std::string(argv[1]);
	}

	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  SortedOffsetIndexTest::suite() );
	
	if (sserialize::tests::TestBase::popProtector()) {
		runner.eventManager().popProtector();
	}
	
	bool ok = runner.run();
	return ok ? 0 : 1;
}
