#include <sserialize/iterator/RangeGenerator.h>
#include <sserialize/utility/printers.h>
#include <sserialize/algorithm/utilmath.h>
#include <sserialize/stats/statfuncs.h>
#include <iomanip>
#include "TestBase.h"

bool deq(double a, double b) { return std::abs<double>(a-b) < 0.0001; }

class LinearRegressionTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( LinearRegressionTest );
CPPUNIT_TEST( testSimple );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testSimple() {
		for(uint64_t size = 10000; size < 10000000; size *= 10) {
			for(uint64_t stride = 1; stride < 15; ++stride) {
				double slope, yintercept;
				sserialize::RangeGenerator<uint64_t> rg(0, size*stride, stride);
				sserialize::statistics::linearRegression(rg.begin(), rg.end(), slope, yintercept);
				CPPUNIT_ASSERT_MESSAGE(sserialize::toString("slope with size=", size, " stride=", stride," was=", slope), deq(slope, stride));
				CPPUNIT_ASSERT_MESSAGE(sserialize::toString("yintercept with size=", size, " stride=", stride," was=", yintercept), deq(yintercept, 0));
			}
		}
	}
	
	
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	std::cout << std::setprecision(9);
	std::cerr << std::setprecision(9);
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  LinearRegressionTest::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}