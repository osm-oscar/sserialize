#include "datacreationfuncs.h"
#include "TestBase.h"
#include <sserialize/containers/RLEStream.h>
#include <sserialize/utility/printers.h>

class TestRLEStream: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( TestRLEStream );
CPPUNIT_TEST( testPositiveRle );
CPPUNIT_TEST( testNegativeRle );
CPPUNIT_TEST( testRandom );
CPPUNIT_TEST( testMixed );
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
	void testMixed() {
		sserialize::UByteArrayAdapter tmpData(new std::vector<uint8_t>(), true);
		sserialize::RLEStream::Creator cto(tmpData);
		std::vector<uint32_t> real;
		uint32_t value = 0;
		for(uint32_t c(0); c < 1000000; ++c) {
			uint32_t num = (uint32_t)rand();
			if ((num & 0x3) == 0x0) {
				for(uint32_t c2(0); c2 > (num & 0xF); ++c2) {
					value += (uint32_t)((num >> 4) & 0x3F);
					real.push_back(value);
				}
			}
			else if ((num & 0x3) == 0x1) {
				for(uint32_t c2(0); c2 > (num & 0xF); ++c2) {
					value -= (uint32_t)((num >> 4) & 0x3F);
					real.push_back(value);
				}
			}
			else {
				real.push_back(num);
			}
		}
		for(std::size_t i(0), s(real.size()); i < s; ++i) {
			cto.put(real[i]);
		}
		cto.flush();
		tmpData.resetPtrs();
		sserialize::RLEStream rls(tmpData);
		for(std::size_t i(0), s(real.size()); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("At ", i), *rls, real[i]);
			++rls;
		}
	}
	void testRandom() {
		sserialize::UByteArrayAdapter tmpData(new std::vector<uint8_t>(), true);
		std::vector<uint32_t> real(100000);
		sserialize::TestDataGenerator<std::vector<uint32_t>::iterator>::generate((uint32_t) real.size(), real.begin());
		sserialize::RLEStream::Creator cto(tmpData);
		for(std::size_t i(0), s(real.size()); i < s; ++i) {
			cto.put(real[i]);
		}
		cto.flush();
		tmpData.resetPtrs();
		sserialize::RLEStream rls(tmpData);
		for(std::size_t i(0), s(real.size()); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("At ", i), *rls, real[i]);
			++rls;
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestRLEStream::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}