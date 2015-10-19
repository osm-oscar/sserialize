#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestAssert.h>
#include "datacreationfuncs.h"
#include <sserialize/utility/printers.h>
#include <sserialize/algorithm/oom_algorithm.h>

class OomAlgorithm: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( OomAlgorithm );
CPPUNIT_TEST( testSort );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testSort() {
		for(uint32_t i(0); i < 16; ++i) {
			std::vector<uint32_t> data(1025*1023*519);
			std::generate(data.begin(), data.end(), []() { return rand(); });
			sserialize::oom_sort(data.begin(), data.end(), [](uint32_t a, uint32_t b) { return a < b; }, 1 << 22, 1 << 19);
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not sorted in run ", i), std::is_sorted(data.begin(), data.end()));
		}
	}
	
	void testUnique() {
		for(uint32_t i(0); i < 16; ++i) {
			std::vector<uint32_t> data(1025*1023*519);
			uint32_t tmp = 0;
			std::generate(data.begin(), data.end(), [&tmp]() {
				if (rand() % 4 == 1) {
					return tmp;
				}
				return ++tmp;
			});
			sserialize::oom_sort(data.begin(), data.end(), std::less<uint32_t>(), 1 << 22, 1 << 19);
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not sorted in run ", i), std::is_sorted(data.begin(), data.end()));
			sserialize::oom_unique(data.begin(), data.end());
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not sorted in run ", i), sserialize::is_unique(data.begin(), data.end()));
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  OomAlgorithm::suite() );
	runner.run();
	return 0;
}