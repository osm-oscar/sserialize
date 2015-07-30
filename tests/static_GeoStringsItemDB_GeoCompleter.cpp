#include "GeoCompleterTest.h"
#include <sserialize/search/GeoCompleterPrivateProxy.h>
#include "Static/GeoStringsItemDB.h"
#include "utilalgos.h"
#define EPS 0.000025

using namespace sserialize;

class StaticGeoStringsItemDBGeoCompleterTest: public GeoCompleterTest {
CPPUNIT_TEST_SUITE( StaticGeoStringsItemDBGeoCompleterTest );
CPPUNIT_TEST( testGeoCompletion );
CPPUNIT_TEST( testPartialGeoCompletion );
CPPUNIT_TEST_SUITE_END();
public:
	typedef Static::GeoStringsItemDB<TestItemData> MySDBType;
	typedef GeoCompleterPrivateProxy<MySDBType> MySGeoCompleterPrivateType;
private:
	MySDBType m_sdb;
	GeoCompleter m_geoCmp;
private:
	virtual GeoCompleter & geoCompleter() {
		return m_geoCmp;
	}
public:
	virtual void setUp() {
		GeoStringsItemDB<TestItemData> srcDB = createDB();
		UByteArrayAdapter stableAdap(new std::vector<uint8_t>(), true);
		UByteArrayAdapter sdbAdap(new std::vector<uint8_t>(), true);
		srcDB.createStaticStringTable(stableAdap);
		sdbAdap << srcDB;
		
		
		//set the all up
		m_sdb = MySDBType(sdbAdap, stableAdap);
		m_geoCmp = GeoCompleter( new MySGeoCompleterPrivateType(m_sdb) );
	}
	
	/** derived classes need to do clean-up of data-structures associated with StringCompleter */
	virtual void tearDown() {}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticGeoStringsItemDBGeoCompleterTest::suite() );
	runner.run();
	return 0;
};