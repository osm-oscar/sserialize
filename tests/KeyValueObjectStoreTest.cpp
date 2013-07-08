#include <iostream>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/containers/KeyValueObjectStore.h>
#include <sserialize/Static/KeyValueObjectStore.h>
#define EPS 0.000025


using namespace sserialize;

class KeyValueObjectStoreTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( KeyValueObjectStoreTest );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST( testFindKey );
CPPUNIT_TEST( testFindValue );
CPPUNIT_TEST( testItem );
CPPUNIT_TEST_SUITE_END();
public:
	typedef GeoStringsItemDB<TestItemData> MyDBType;
	typedef typename GeoStringsItemDB< TestItemData >::Item MyDBItemType;
	
	typedef Static::GeoStringsItemDB<TestItemData> MyStaticDBType;
	typedef typename Static::GeoStringsItemDB<TestItemData>::Item MyStaticDBItemType;
private:
	std::vector< std::pair<std::string, std::string>  > m_items;
	UByteArrayAdapter m_skvData;
	Static::KeyValueObjectStore m_skv;
public:
	virtual void setUp() {

	}
	virtual void tearDown() {}
	
	void testSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_item.size() ), m_sdb.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Reported sizeInBytes don't match", static_cast<uint32_t>( m_sdbData.size() ), m_sdb.getSizeInBytes());
	}
};


int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( KeyValueObjectStoreTest::suite() );
	runner.run();
	return 0;
}