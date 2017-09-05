#include <vector>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/log.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/DynamicBitSet.h>
#include "datacreationfuncs.h"
#include "TestBase.h"

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

class ItemIndexPrivateBaseTest: public sserialize::tests::TestBase {
protected:
	virtual bool create(const std::set<uint32_t> & srcSet, sserialize::ItemIndex & idx) = 0;
	virtual bool create(const std::vector<uint32_t> & srcSet, sserialize::ItemIndex & idx) = 0;
	size_t TEST_RUNS;
	sserialize::ItemIndex::Types m_idxType;
public:
	ItemIndexPrivateBaseTest(sserialize::ItemIndex::Types idxType) : m_idxType(idxType) {}
	virtual void setUp() {TEST_RUNS = 256;}
	virtual void tearDown() {}
	

	void testRandomEquality() {
// 		srand(0);
		uint32_t setCount = 2048;

		for(size_t i = 0; i < TEST_RUNS; i++) {
			std::set<uint32_t> realValues( myCreateNumbers(rand() % setCount) );
			ItemIndex idx;
			create(realValues, idx);
		
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
		ItemIndex idx;
		create(realValues, idx);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), idx.size());
		
		uint32_t count = 0;
		for(std::set<uint32_t>::iterator it = realValues.begin(); it != realValues.end(); ++it, ++count) {
			std::stringstream ss;
			ss << "id at " << count;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, idx.at(count));
		}
		{
			std::vector<uint32_t> src[3] = { std::vector<uint32_t>{1,2,3,5,7}, std::vector<uint32_t>{5}, std::vector<uint32_t>{0,2,3,4,5,7} };
			std::vector<ItemIndex> idcs(3);
			for(uint32_t i=0; i < 3; ++i) {
				create(src[i], idcs[i]);
			}
			for(uint32_t i(0), s((uint32_t) idcs.size()); i < s; ++i) {
				CPPUNIT_ASSERT_MESSAGE(sserialize::toString("united special created index=",i," is broken"), src[i] == idcs[i]);
			}
			ItemIndex unitedIdx = ItemIndex::unite(idcs);
			ItemIndex unitedReal(std::vector<uint32_t>{0,1,2,3,4,5,7});
	
			CPPUNIT_ASSERT_EQUAL_MESSAGE("united special 0+2 is broken", unitedReal, idcs[0] + idcs[2]);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("united special 0+1 is broken", idcs[0], idcs[0] + idcs[1]);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("united special manual is broken", unitedReal, (idcs[0] + idcs[2]) + idcs[1]);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("united special manual like ItemIndex::unite is broken", unitedReal, (idcs[0] + idcs[1]) + idcs[2]);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("united special is broken", unitedReal, unitedIdx);
		}
		{
			std::vector<uint32_t> src[2] = { std::vector<uint32_t>{15310}, std::vector<uint32_t>{13058, 13288} };
			std::vector<ItemIndex> idcs(2);
			for(uint32_t i=0; i < 2; ++i) {
				create(src[i], idcs[i]);
			}
			for(uint32_t i(0), s((uint32_t) idcs.size()); i < s; ++i) {
				CPPUNIT_ASSERT_MESSAGE(sserialize::toString("united special2 created index=",i," is broken"), src[i] == idcs[i]);
				idcs[0].back();
				idcs[1].back();
			}
			ItemIndex unitedIdx = ItemIndex::uniteK(idcs[0], idcs[1], 100);
			ItemIndex unitedReal(std::vector<uint32_t>{13058, 13288, 15310});
	
			CPPUNIT_ASSERT_EQUAL_MESSAGE("united special2 manual is broken", unitedReal, idcs[0] + idcs[1]);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("united special2 manual is broken", unitedReal, idcs[1] + idcs[0]);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("united special2 is broken", unitedReal, unitedIdx);
		}
		{
			for(uint32_t i(0); i < TEST_RUNS; ++i) {
				std::set<uint32_t> srcSet = createNumbersSet(rand() % 2048);
				ItemIndex idx;
				for(uint32_t j(0), js(32); j < js; ++j) {
					srcSet.insert(j);
					create(srcSet, idx);
					CPPUNIT_ASSERT_MESSAGE("single value index test", srcSet == idx);
				}
			}
		}

		{
			std::vector<uint32_t> srcSet(1);
			ItemIndex idx;
			for(uint32_t i(0), s(1024); i < s; ++i) {
				srcSet[0] = i;
				create(srcSet, idx);
				CPPUNIT_ASSERT_MESSAGE("single value index test", srcSet == idx);
			}
		}
		
		{
			std::vector<uint32_t> srcSet;
			ItemIndex idx;
			create(srcSet, idx);
			CPPUNIT_ASSERT_MESSAGE("empty index test", srcSet == idx);
		}
	}
	
	void testIntersect() {
		for(uint32_t runs = 0; runs < TEST_RUNS; ++runs) {
			std::set<uint32_t> a,b;
			createOverLappingSets(a, b,  0xFF, 0xFF, 0xFF);
			ItemIndex idxA;
			ItemIndex idxB;
			create(a, idxA);
			create(b, idxB);
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)a.size(), idxA.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)b.size(), idxB.size());
			CPPUNIT_ASSERT_MESSAGE("A set unequal", a == idxA);
			CPPUNIT_ASSERT_MESSAGE("B set unequal", b == idxB);
			
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
	}
	
	void testUnite() {
// 		srand(0);
		uint32_t setCount = 2048;

		for(size_t i = 0; i < TEST_RUNS; i++) {
			std::set<uint32_t> realValuesA( myCreateNumbers(rand() % setCount) );
			std::set<uint32_t> realValuesB( myCreateNumbers(rand() % setCount) );
			ItemIndex idxA;
			ItemIndex idxB;
			create(realValuesA, idxA);
			create(realValuesB, idxB);

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
	
	void testDifference() {
// 		srand(0);
		for(size_t i = 0; i < TEST_RUNS; ++i) {
			std::set<uint32_t> a,b;
			createOverLappingSets(a, b,  0xFF, 0xFF, 0xFF);
			ItemIndex idxA;
			ItemIndex idxB;
			create(a, idxA);
			create(b, idxB);
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)a.size(), idxA.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)b.size(), idxB.size());
			CPPUNIT_ASSERT_MESSAGE("A set unequal", a == idxA);
			CPPUNIT_ASSERT_MESSAGE("B set unequal", b == idxB);
			
			std::set<uint32_t> intersected;
			std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::insert_iterator<std::set<uint32_t> >(intersected, intersected.end()));
			ItemIndex diffIdx = idxA - idxB;
			uint32_t count = 0;
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size of difference", (uint32_t) intersected.size(), diffIdx.size());
			for(std::set<uint32_t>::iterator it = intersected.begin(); it != intersected.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "id at " << count << " run " << i;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, diffIdx.at(count));
			}
		}
		{
			std::set<uint32_t> realValuesA( myCreateNumbers(rand() % 2048) );
			std::set<uint32_t>::const_iterator rvaIt(realValuesA.cbegin());
			std::set<uint32_t>::const_iterator rvaEnd(realValuesA.cbegin());
			std::advance(rvaIt, (realValuesA.size()/2));
			std::advance(rvaEnd, (realValuesA.size()/2)+1);
			std::set<uint32_t> realValuesB(rvaIt, rvaEnd);
			ItemIndex idxA, idxB;
			create(realValuesA, idxA);
			create(realValuesB, idxB);
			CPPUNIT_ASSERT_MESSAGE("A set unequal", realValuesA == idxA);
			CPPUNIT_ASSERT_MESSAGE("B set unequal", realValuesB == idxB);
			realValuesA.erase(*realValuesB.begin());
			ItemIndex diffIdx = idxA - idxB;
			
			CPPUNIT_ASSERT_MESSAGE("diff index unequal", realValuesA == diffIdx);
		}
	}
	
	void testSymmetricDifference() {
// 		srand(0);
		for(size_t i = 0; i < TEST_RUNS; ++i) {
			std::set<uint32_t> a,b;
			createOverLappingSets(a, b,  0xFF, 0xFF, 0xFF);
			ItemIndex idxA;
			ItemIndex idxB;
			create(a, idxA);
			create(b, idxB);
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("A set size", (uint32_t)a.size(), idxA.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("B set size", (uint32_t)b.size(), idxB.size());
			CPPUNIT_ASSERT_MESSAGE("A set unequal", a == idxA);
			CPPUNIT_ASSERT_MESSAGE("B set unequal", b == idxB);
			
			std::set<uint32_t> resultSet;
			std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), std::insert_iterator<std::set<uint32_t> >(resultSet, resultSet.end()));
			ItemIndex symDiffIdx = idxA ^ idxB;
			uint32_t count = 0;
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size of symDiff", (uint32_t) resultSet.size(), symDiffIdx.size());
			for(std::set<uint32_t>::iterator it = resultSet.begin(); it != resultSet.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "id at " << count << " run " << i;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *it, symDiffIdx.at(count));
			}
		}
	}
	
	void testFind() {
		uint32_t setCount = 16;

		for(size_t i = 0; i < setCount; i++) {
		
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			std::set<uint32_t> testValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			testValues.insert(realValues.begin(), realValues.end());
			ItemIndex idx;
			create(realValues, idx);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), idx.size());
		
			int count = 0;
			for(std::set<uint32_t>::iterator it = realValues.begin(); it != realValues.end(); ++it, ++count) {
				std::stringstream ss;
				ss << "id at " << count << "; run=" << i;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), count, idx.find(*it));
			}

			for(std::set<uint32_t>::iterator it = testValues.begin(); it != testValues.end(); ++it) {
				std::stringstream ss;
				ss << "id at " << count << "; run=" << i;
				CPPUNIT_ASSERT_MESSAGE(ss.str(), (idx.find(*it) < 0) == (realValues.count(*it) == 0));
			}
		}
	}
	
	void testRandomMaxSetEquality() {
// 		srand(0);

		for(size_t i = 0; i < TEST_RUNS; i++) {
		
			DynamicBitSet bitSet;
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			ItemIndex idx;
			create(realValues, idx);
		
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
// 		srand(0);
		for(size_t i = 0; i < TEST_RUNS; i++) {
			DynamicBitSet bitSet;
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			ItemIndex idx;
			create(realValues, idx);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), idx.size());
			CPPUNIT_ASSERT_MESSAGE("test index unequal", realValues == idx);
			
			idx.putInto(bitSet);
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("index from DynamicBitSet unequal in testrun=", i), realValues == bitSet.toIndex(m_idxType));
			
		}
	}
	
	void testPutIntoVector() {
// 		srand(0);

		for(size_t i = 0; i < TEST_RUNS; i++) {
			std::vector<uint32_t> vec;
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 16395, 0xFFFFF) );
			ItemIndex idx;
			create(realValues, idx);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), idx.size());
			CPPUNIT_ASSERT_MESSAGE("test index unequal", realValues == idx);
			
			idx.putInto(vec);
			CPPUNIT_ASSERT_MESSAGE("vector from index unequal", idx == vec);
			
		}
	}
	
	void testIterator() {
// 		srand(0);
		uint32_t setCount = 10000;

		for(size_t i = 0; i < TEST_RUNS; i++) {
			std::set<uint32_t> realValues( myCreateNumbers(rand() % setCount) );
			ItemIndex idx;
			create(realValues, idx);
		
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), idx.size());
		
			std::set<uint32_t>::iterator it = realValues.begin();
			ItemIndex::const_iterator sit = idx.cbegin();
			for(uint32_t j = 0, s = (uint32_t) realValues.size(); j < s; ++j, ++it, ++sit) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("direct access at ", j), *it, idx.at(j));
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("iterator access at ", j), *it, *sit);
			}
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("sit != idx.cend() in run ", i),! (sit != idx.cend()));
		}
	}
};
