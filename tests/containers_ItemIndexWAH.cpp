#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <vector>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateWAH.h>
#include <sserialize/containers/ItemIndex.h>
#include "datacreationfuncs.h"

using namespace sserialize;


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


class ItemIndexPrivateWAHTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( ItemIndexPrivateWAHTest );
// CPPUNIT_TEST( testRandomEquality );
// CPPUNIT_TEST( testSpecialEquality );
// CPPUNIT_TEST( testIntersect );
// CPPUNIT_TEST( testUnite );
// CPPUNIT_TEST( testRandomMaxSetEquality );
CPPUNIT_TEST( testDynamicBitSet );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	
	void testRandomEquality() {
		srand(0);
		uint32_t setCount = 2048;

		for(size_t i = 0; i < setCount; i++) {
			std::set<uint32_t> realValues( myCreateNumbers(rand() % setCount) );
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			ItemIndexPrivateWAH::create(realValues, dest);
			ItemIndex idx(dest, ItemIndex::T_WAH);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), idx.size());
		
			uint32_t count = 0;
			for(std::set<uint32_t>::iterator it = realValues.begin(); it != realValues.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "id at " << count;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, idx.at(count));
			}
		}
	}
	
	void testSpecialEquality() {
		std::set<uint32_t> realValues;
		addRange(0, 32, realValues);
		addRange(98, 100, realValues);
		addRange(127, 128, realValues);
		addRange(256, 512, realValues);
		realValues.insert(19);
		realValues.insert(62);
		UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
		ItemIndexPrivateWAH::create(realValues, dest);
		ItemIndex idx(dest, ItemIndex::T_WAH);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), idx.size());
		
		uint32_t count = 0;
		for(std::set<uint32_t>::iterator it = realValues.begin(); it != realValues.end(); ++it, ++count) {
			std::stringstream ss;
			ss << "id at " << count;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, idx.at(count));
		}
	}
	
	void testIntersect() {
		std::set<uint32_t> a,b;
		createOverLappingSets(a, b,  0xFF, 0xFF, 0xFF);
		UByteArrayAdapter destA(new std::vector<uint8_t>(), true);
		UByteArrayAdapter destB(new std::vector<uint8_t>(), true);
		ItemIndexPrivateWAH::create(a, destA);
		ItemIndexPrivateWAH::create(b, destB);
		ItemIndex idxA(destA, ItemIndex::T_WAH);
		ItemIndex idxB(destB, ItemIndex::T_WAH);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)a.size(), idxA.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)b.size(), idxB.size());
		CPPUNIT_ASSERT_MESSAGE("A set unequal", a == idxA);
		CPPUNIT_ASSERT_MESSAGE("A set unequal", b == idxB);
		
		std::set<uint32_t> intersected;
		std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::insert_iterator<std::set<uint32_t> >(intersected, intersected.end()));
		ItemIndex intIdx = idxA / idxB;
		uint32_t count = 0;
		for(std::set<uint32_t>::iterator it = intersected.begin(); it != intersected.end(); ++it, ++count) {
			std::stringstream ss;
			ss << "id at " << count;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, intIdx.at(count));
		}
	}

	void testUnite() {
		srand(0);
		uint32_t setCount = 2048;

		for(size_t i = 0; i < 256; i++) {
			std::set<uint32_t> realValuesA( myCreateNumbers(rand() % setCount) );
			std::set<uint32_t> realValuesB( myCreateNumbers(rand() % setCount) );
			UByteArrayAdapter destA(new std::vector<uint8_t>(), true);
			UByteArrayAdapter destB(new std::vector<uint8_t>(), true);
			ItemIndexPrivateWAH::create(realValuesA, destA);
			ItemIndexPrivateWAH::create(realValuesB, destB);
			
			ItemIndex idxA(destA, ItemIndex::T_WAH);
			ItemIndex idxB(destB, ItemIndex::T_WAH);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("A set size", (uint32_t)realValuesA.size(), idxA.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("B set size", (uint32_t)realValuesB.size(), idxB.size());
			CPPUNIT_ASSERT_MESSAGE("A set unequal", realValuesA == idxA);
			CPPUNIT_ASSERT_MESSAGE("B set unequal", realValuesB == idxB);

			std::set<uint32_t> united(realValuesA);
			united.insert(realValuesB.begin(), realValuesB.end());
			ItemIndex unitedIdx = idxA + idxB;
			CPPUNIT_ASSERT_EQUAL_MESSAGE("united size", (uint32_t)united.size(), unitedIdx.size());
			uint32_t count = 0;
			for(std::set<uint32_t>::iterator it = united.begin(); it != united.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "run i=" << i << "; id at " << count;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, unitedIdx.at(count));
			}
		}
	}
	
	void testRandomMaxSetEquality() {
		srand(0);
		uint32_t setCount = 16;

		for(size_t i = 0; i < setCount; i++) {
			if (i == 8)
				std::cout << "reached" << std::endl;
		
			DynamicBitSet bitSet;
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			ItemIndexPrivateWAH::create(realValues, dest);
			ItemIndex idx(dest, ItemIndex::T_WAH);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), idx.size());
		
			uint32_t count = 0;
			for(std::set<uint32_t>::iterator it = realValues.begin(); it != realValues.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "id at " << count << "; run=" << i;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, idx.at(count));
			}
		}
	}

	void testDynamicBitSet() {
		srand(0);
		uint32_t setCount = 128;

		for(size_t i = 0; i < setCount; i++) {
			DynamicBitSet bitSet;
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			ItemIndexPrivateWAH::create(realValues, dest);
			ItemIndex idx(dest, ItemIndex::T_WAH);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), idx.size());
			CPPUNIT_ASSERT_MESSAGE("test index unequality", realValues == idx);
			
			idx.putInto(bitSet);
			ItemIndex simpleIndexFromSet( bitSet.toIndex(ItemIndex::T_SIMPLE) );
			ItemIndex roundTripIndex( bitSet.toIndex(ItemIndex::T_WAH) );
			
			CPPUNIT_ASSERT_MESSAGE("simple index from DynamicBitSet from wah index unequality", idx == simpleIndexFromSet);
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("index from DynamicBitSet from wah index size unequal", idx.size(), roundTripIndex.size());
			
			//now test roundtrip index
			for(uint32_t i = 0; i < idx.size(); ++i) {
				std::stringstream ss;
				ss << "index from DynamicBitSet from wah index unequal at position=" << i;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), idx.at(i), roundTripIndex.at(i));
			}
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  ItemIndexPrivateWAHTest::suite() );
	runner.run();
	return 0;
}