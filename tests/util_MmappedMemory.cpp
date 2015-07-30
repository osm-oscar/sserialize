#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/storage/MmappedMemory.h>

class MmappedMemoryTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( MmappedMemoryTest );
CPPUNIT_TEST( testDeletion );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testDeletion() {
		{
			sserialize::MmappedMemory<uint8_t> mm(static_cast<uint64_t>(0xFFFFFFFF)+1, sserialize::MM_SHARED_MEMORY);
			mm.data()[0xFFFFFFFF] = 1;
		}
		
		{
			std::cout << "MM should now be dead!!" << std::endl;
		}
	}
};


int main(int argc, char ** argv) {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( MmappedMemoryTest::suite() );
	bool ok = runner.run();
	return (ok ? 0 : 1);
}