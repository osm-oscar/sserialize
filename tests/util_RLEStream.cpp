#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestAssert.h>
#include "datacreationfuncs.h"
#include <sserialize/utility/RLEStream.h>
#include <sserialize/utility/printers.h>

class TestRLEStream: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( TestRLEStream );
CPPUNIT_TEST( testPositiveRle );
CPPUNIT_TEST( testNegativeRle );
CPPUNIT_TEST( testRandom );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testPositiveRle() {
		sserialize::UByteArrayAdapter tmpData(new std::vector<uint8_t>(), true);
		sserialize::RLEStream::Creator cto(tmpData);
		for(uint32_t i=0; i < 1000000; ++i) {
			cto.put(i);
		}
		cto.flush();
		CPPUNIT_ASSERT(tmpData.size() < 10);
	}
	void testNegativeRle() {
		sserialize::UByteArrayAdapter tmpData(new std::vector<uint8_t>(), true);
		sserialize::RLEStream::Creator cto(tmpData);
		for(uint32_t i=1000000; i > 0; --i) {
			cto.put(i);
		}
		cto.flush();
		CPPUNIT_ASSERT(tmpData.size() < 15);
	}
	void testRandom() {
		sserialize::UByteArrayAdapter tmpData(new std::vector<uint8_t>(), true);
		std::vector<uint32_t> real(100000);
		sserialize::TestDataGenerator<std::vector<uint32_t>::iterator>::generate(real.size(), real.begin());
		sserialize::RLEStream::Creator cto(tmpData);
		for(uint32_t i(0), s(real.size()); i < s; ++i) {
			cto.put(real[i]);
		}
		cto.flush();
		tmpData.resetPtrs();
		sserialize::RLEStream rls(tmpData);
		for(uint32_t i(0), s(real.size()); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("At ", i), *rls, real[i]);
			++rls;
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestRLEStream::suite() );
	runner.run();
	return 0;
}