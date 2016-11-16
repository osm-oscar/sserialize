#include <sserialize/storage/MmappedMemory.h>
#include "TestBase.h"

class MmappedMemoryTest: public sserialize::tests::TestBase {
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
	sserialize::tests::TestBase::argc = argc;
	sserialize::tests::TestBase::argv = argv;
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( MmappedMemoryTest::suite() );
	bool ok = runner.run();
	return (ok ? 0 : 1);
}