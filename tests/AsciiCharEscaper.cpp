#include <sserialize/strings/stringfunctions.h>
#include <sserialize/utility/printers.h>
#include "TestBase.h"

class TestAsciiCharEscaper: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( TestAsciiCharEscaper );
CPPUNIT_TEST( test );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void test() {
		std::string escapeStr("\"\\/\b\f\n\r\t");
		sserialize::AsciiCharEscaper escaper(escapeStr.cbegin(), escapeStr.cend());
		std::string escapedStr = escaper.escape(escapeStr);
		CPPUNIT_ASSERT_EQUAL(escapeStr.size()*2, escapedStr.size());
		for(uint32_t i(0), s((uint32_t) escapeStr.size()); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("original char pos=", i), (int)escapeStr[i], (int)escapedStr[2*i+1]);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("escape char pos=", i),(int)'\\', (int)escapedStr[2*i]);
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::argc = argc;
	sserialize::tests::TestBase::argv = argv;
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestAsciiCharEscaper::suite() );
	runner.run();
	return 0;
}