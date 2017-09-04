#include "TestBase.h"
#include "datacreationfuncs.h"

#include <sserialize/iterator/UnaryCodeIterator.h>

class UnaryCodeIteratorTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( UnaryCodeIteratorTest );
CPPUNIT_TEST( testRandom );
CPPUNIT_TEST( testZero );
CPPUNIT_TEST( testMonotoneSequence );
CPPUNIT_TEST_SUITE_END();
public:
	static constexpr std::size_t test_data_size = 10240;
	static constexpr int test_count = 16;
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testRandom() {
		for(int i(0); i < test_count; ++i) {
			std::vector<uint32_t> realValues(test_data_size);
			for(uint32_t & v : realValues) {
				v = rand() & 0xFF;
			}
			
			sserialize::UByteArrayAdapter tmp(sserialize::UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY));
			sserialize::UnaryCodeCreator ucc(tmp);
			
			ucc.put(realValues.cbegin(), realValues.cend());
			ucc.flush();
			
			tmp.resetPtrs();
			sserialize::UnaryCodeIterator uci(tmp);
			
			for(std::size_t j(0), s(realValues.size()); j < s; ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE("Run " + std::to_string(i) + "; Entry " + std::to_string(j), realValues.at(j), *uci);
				++uci;
			}
		}
	}
	void testZero() {
		for(int i(0); i < test_count; ++i) {
			std::vector<uint32_t> realValues(test_data_size, 0);
			
			sserialize::UByteArrayAdapter tmp(sserialize::UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY));
			sserialize::UnaryCodeCreator ucc(tmp);
			
			ucc.put(realValues.cbegin(), realValues.cend());
			ucc.flush();
			
			tmp.resetPtrs();
			sserialize::UnaryCodeIterator uci(tmp);
			
			for(std::size_t j(0), s(realValues.size()); j < s; ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE("Run " + std::to_string(i) + "; Entry " + std::to_string(j), realValues.at(j), *uci);
				++uci;
			}
		}
	}
	void testMonotoneSequence() {
		for(int i(0); i < test_count; ++i) {
			std::vector<uint32_t> realValues(test_data_size);
			for(uint32_t j(0); j < test_data_size; ++j) {
				realValues[j] = j;
			}
			
			sserialize::UByteArrayAdapter tmp(sserialize::UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY));
			sserialize::UnaryCodeCreator ucc(tmp);
			
			ucc.put(realValues.cbegin(), realValues.cend());
			ucc.flush();
			
			tmp.resetPtrs();
			sserialize::UnaryCodeIterator uci(tmp);
			
			for(std::size_t j(0), s(realValues.size()); j < s; ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE("Run " + std::to_string(i) + "; Entry " + std::to_string(j), realValues.at(j), *uci);
				++uci;
			}
		}
	}
};

int main(int argc, char ** argv) {
	srand( 0 );
	
	sserialize::tests::TestBase::init(argc, argv);
	
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  UnaryCodeIteratorTest::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}