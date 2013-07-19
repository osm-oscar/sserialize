#include <iostream>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/log.h>
#include <algorithm>
#include "printfunctions.h"
#define EPS 0.000025


using namespace sserialize;

std::vector<std::string> testStrings;

std::string getRandomString() {
	uint32_t pos = ((double)rand())/RAND_MAX*testStrings.size();
	return testStrings.at(pos);
}

template<int T_TEST_COUNT, int T_ITEM_COUNT>
class UtilFuncsTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( UtilFuncsTest );
CPPUNIT_TEST( test_reorder );
CPPUNIT_TEST_SUITE_END();
private:
private:
public:
	virtual void setUp() {}
	virtual void tearDown() {}

	void test_reorder() {
		for(uint32_t i = 0; i < T_TEST_COUNT; ++i) {
			std::vector<uint32_t> src = sserialize::range< std::vector<uint32_t>, uint32_t, uint32_t >(0, T_ITEM_COUNT, 1);
			std::vector<uint32_t> shuffled = src;
			std::random_shuffle(shuffled.begin(), shuffled.end());
			reorder(src, shuffled);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", shuffled.size(), src.size());
			for(uint32_t i = 0; i < shuffled.size(); ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(osmfindlog::toString("at ", i), shuffled[i], src[i]);
			}
		}
	}
};

std::ostream& operator<<(std::ostream & out, const std::vector<uint32_t> & vec) {
	out << "std::vector<" << vec.size() << ">[";
	for(std::vector<uint32_t>::const_iterator it(vec.begin()); it != vec.end(); ++it) {
		out << *it << ", ";
	}
	out << "]";
	return out;
}


int main(int argc, char ** argv) {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( UtilFuncsTest<10, 10000>::suite() );
	runner.addTest( UtilFuncsTest<100, 10000>::suite() );
	runner.addTest( UtilFuncsTest<10, 100000>::suite() );
	runner.run();
	return 0;
}