#ifndef SSERIALIZE_TESTS_TEST_BASE_H
#define SSERIALIZE_TESTS_TEST_BASE_H
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestResult.h>
#include <sserialize/utility/printers.h>

namespace std {
	using sserialize::operator<<;
}

namespace sserialize {
namespace tests {

class TestBase: public CppUnit::TestFixture {
// CPPUNIT_TEST_SUITE( Test );
// CPPUNIT_TEST( test );
// CPPUNIT_TEST_SUITE_END();
public:
	TestBase();
	virtual ~TestBase();
public:
	static void init(int argc, char ** argv);
	static bool popProtector();
	static bool printHelp();
private:
	static int argc;
	static char ** argv;
	static bool sm_popProtector;
	static bool sm_printHelp;
};

}} //end namespace sserialize::tests

/*
int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  Test::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
*/

#endif
