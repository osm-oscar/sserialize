#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <vector>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/DynamicBitSet.h>
#include "datacreationfuncs.h"

using namespace sserialize;


std::set<uint32_t> myCreateNumbers(uint32_t count, uint32_t maxNum) {
	std::set<uint32_t> ret;
	uint32_t rndNum;
	for(uint32_t i = 0; i < count; i++) {
		rndNum = (double)rand()/RAND_MAX * maxNum;
		ret.insert(rndNum);
	}
	return ret;
}

class DynamicBitSetTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( DynamicBitSetTest );
CPPUNIT_TEST( testRandomEquality );
CPPUNIT_TEST( testIndexCreation );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	
	void testRandomEquality() {
		srand(0);
		uint32_t setCount = 4;

		for(size_t i = 0; i < setCount; i++) {
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			DynamicBitSet bitSet(dest);
			
			uint32_t maxNum = *realValues.rbegin();
			
			for(std::set<uint32_t>::const_iterator it = realValues.begin(); it != realValues.end(); ++it) {
				bitSet.set(*it);
			}

			for(uint32_t j = 0; j <= maxNum+16 || j < bitSet.data().size()*8; ++j) {
				std::stringstream ss;
				ss << "id " << i;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), (realValues.count(j) > 0), bitSet.isSet(j));
			}
		}
	}
	
	void testIndexCreation() {
		srand(0);
		uint32_t setCount = 4;

		for(size_t i = 0; i < setCount; i++) {
			std::set<uint32_t> realValues( myCreateNumbers(rand() % 2048, 0xFFFFF) );
			UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
			DynamicBitSet bitSet(dest);
			
			for(std::set<uint32_t>::const_iterator it = realValues.begin(); it != realValues.end(); ++it) {
				bitSet.set(*it);
			}
			ItemIndex idx(bitSet.toIndex(ItemIndex::T_SIMPLE));
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t) realValues.size(),  idx.size());
			
			CPPUNIT_ASSERT_MESSAGE("idx equality", (realValues == idx));
		}
	}

};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  DynamicBitSetTest::suite() );
	runner.run();
	return 0;
}