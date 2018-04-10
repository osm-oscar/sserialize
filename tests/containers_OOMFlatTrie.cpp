#include "TestBase.h"
#include <sserialize/search/OOMFlatTrie.h>

class TestTemplate: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( TestTemplate );
CPPUNIT_TEST( test );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void test() {}
};

int main(int argc, char ** argv) {
	srand( 0 );
	
	sserialize::tests::TestBase::init(argc, argv);
	
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestTemplate::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
