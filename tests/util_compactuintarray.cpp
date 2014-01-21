#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/utility/CompactUintArray.h>
#include <sserialize/utility/utilmath.h>
#include <sserialize/utility/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

using namespace sserialize;


int testLen = 0xFFFfF;

class TestCompactUintArray: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( TestCompactUintArray );
CPPUNIT_TEST( createAutoBitsTest );
CPPUNIT_TEST( createManuBitsTest );
CPPUNIT_TEST( boundedTest );
CPPUNIT_TEST_SUITE_END();
private:
	std::vector<uint64_t> compSrcArrays[64];
public:

	virtual void setUp() {
		//Fill the first
		srand( 0 );
		uint64_t rndNum;
		uint64_t mask;
		for(uint32_t i = 0; i < testLen; i++) {
			rndNum = static_cast<uint64_t>(rand()) << 32 | rand();
			for(int j=0; j < 64; j++) {
				mask = createMask64(j+1);
				compSrcArrays[j].push_back(rndNum & mask);
			}
		}
	}
	
	virtual void tearDown() {}

	void createAutoBitsTest() {
		for(uint32_t bits = 0; bits < 64; ++bits) {
			UByteArrayAdapter d(new std::vector<uint8_t>(), true);
			CompactUintArray::create(compSrcArrays[bits], d);
			d.resetPtrs();
			CompactUintArray carr(d, bits+1);
			for(uint32_t i = 0, s = compSrcArrays[bits].size(); i < s; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), compSrcArrays[bits][i], carr.at64(i));
			}
		}
	}
	
	void createManuBitsTest() {
		for(uint32_t bits = 0; bits < 64; ++bits) {
			UByteArrayAdapter d(new std::vector<uint8_t>(), true);
			CompactUintArray::create(compSrcArrays[bits], d, bits+1);
			d.resetPtrs();
			CompactUintArray carr(d, bits+1);
			for(uint32_t i = 0, s = compSrcArrays[bits].size(); i < s; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), compSrcArrays[bits][i], carr.at64(i));
			}
		}
	}

	void boundedTest() {
		for(uint32_t bits = 0; bits < 64; ++bits) {
			UByteArrayAdapter d(new std::vector<uint8_t>(), true);
			BoundedCompactUintArray::create(compSrcArrays[bits], d);
			d.resetPtrs();
			BoundedCompactUintArray bcarr(d);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("for bits=", bits+1), compSrcArrays[bits].size(), bcarr.size());
			for(uint32_t i = 0, s = compSrcArrays[bits].size(); i < s; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), compSrcArrays[bits][i], bcarr.at64(i));
			}
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestCompactUintArray::suite() );
	runner.run();
	return 0;
}
