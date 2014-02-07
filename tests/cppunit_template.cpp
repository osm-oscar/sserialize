#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
class Test: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( Test );
CPPUNIT_TEST( test );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}

	void test() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("equal", 0, 0);
		CPPUNIT_ASSERT_EQUAL(0, 0);
		CPPUNIT_ASSERT_MESSAGE("boolean", true);
		CPPUNIT_ASSERT(true);
	}
};


int main(int argc, char ** argv) {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( Test::suite() );
	bool ok = runner.run();
	return (ok ? 0 : 1);
}