#include "GeoCompleterTest.h"
#include <sserialize/search/GeoCompleterPrivateProxy.h>
#include "Static/GeoStringsItemDB.h"
#include <sserialize/Static/ItemGeoGrid.h>
#include <sserialize/spatial/ItemGeoGrid.h>
#include "utilalgos.h"
#include "helpers.h"
#define EPS 0.000001

using namespace sserialize;

template<uint32_t latcount, uint32_t loncount>
class StaticItemGeoGridTest: public GeoCompleterTest {
CPPUNIT_TEST_SUITE( StaticItemGeoGridTest );
CPPUNIT_TEST( testHeaderInfo );
CPPUNIT_TEST( testIndices );
CPPUNIT_TEST( testGeoCompletion );
CPPUNIT_TEST( testPartialGeoCompletion );
CPPUNIT_TEST_SUITE_END();
public:
	typedef Static::GeoStringsItemDB<TestItemData> MySDBType;
	typedef Static::spatial::ItemGeoGrid<MySDBType> MySItemGeoGridType;
	typedef GeoCompleterPrivateProxy<MySItemGeoGridType> MySGeoCompleterPrivateType;
private:
	MySDBType m_sdb;
	Static::ItemIndexStore m_indexStore;
	spatial::ItemGeoGrid m_srcGrid;
    MySItemGeoGridType m_grid;
    GeoCompleter m_geoCmp;
private:
	virtual GeoCompleter & geoCompleter() {
		return m_geoCmp;
	}
public:
	virtual void setUp() {
		GeoStringsItemDB<TestItemData> srcDB = createDB();
		m_srcGrid = spatial::ItemGeoGrid(spatial::GeoRect(0, 20, 0, 20), latcount, loncount);
		for(uint32_t i = 0; i < srcDB.size(); i++) {
			if (srcDB.geoShape(i)) {
				m_srcGrid.addItem(i, srcDB.geoShape(i));
			}
		}
		
		ItemIndexFactory indexFactory(true);
		UByteArrayAdapter gridAdap(new std::vector<uint8_t>(), true);
		UByteArrayAdapter idxStoreAdap;
		UByteArrayAdapter stableAdap(new std::vector<uint8_t>(), true);
		UByteArrayAdapter sdbAdap(new std::vector<uint8_t>(), true);
		srcDB.createStaticStringTable(stableAdap);
		sdbAdap << srcDB;
		m_srcGrid.serialize(gridAdap, indexFactory);
		indexFactory.flush();
		idxStoreAdap = indexFactory.getFlushedData();
		
		//set the all up
		m_sdb = MySDBType(sdbAdap, stableAdap);
		m_indexStore = Static::ItemIndexStore(idxStoreAdap);
		m_grid = MySItemGeoGridType(gridAdap, m_sdb, m_indexStore);
		m_geoCmp = GeoCompleter( new MySGeoCompleterPrivateType(m_grid) );
		
// 		m_grid.dump();
	}
	
	/** derived classes need to do clean-up of data-structures associated with StringCompleter */
	virtual void tearDown() {}

	void testHeaderInfo() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("LatCount", m_srcGrid.latCount(), m_grid.latCount());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("LonCount", m_srcGrid.lonCount(), m_grid.lonCount());
		CPPUNIT_ASSERT_MESSAGE("LatStep", doubleEq(m_srcGrid.latStep(), m_grid.latStep(), EPS) );
		CPPUNIT_ASSERT_MESSAGE("LonStep", doubleEq(m_srcGrid.lonStep(), m_grid.lonStep(), EPS) );
		
		CPPUNIT_ASSERT_MESSAGE("Rect-lat[0]", doubleEq(m_srcGrid.rect().lat()[0], m_grid.rect().lat()[0], EPS) );
		CPPUNIT_ASSERT_MESSAGE("Rect-lat[1]", doubleEq(m_srcGrid.rect().lat()[1], m_grid.rect().lat()[1], EPS) );
		CPPUNIT_ASSERT_MESSAGE("Rect-lon[0]", doubleEq(m_srcGrid.rect().lon()[0], m_grid.rect().lon()[0], EPS) );
		CPPUNIT_ASSERT_MESSAGE("Rect-lon[1]", doubleEq(m_srcGrid.rect().lon()[1], m_grid.rect().lon()[1], EPS) );
	}
	
	void testIndices() {
		CPPUNIT_ASSERT(m_srcGrid.latCount() == m_grid.latCount());
		CPPUNIT_ASSERT(m_srcGrid.lonCount() == m_grid.lonCount());
		
		for(uint32_t x = 0; x < m_srcGrid.latCount(); ++x) {
			for(uint32_t y = 0; y < m_srcGrid.lonCount(); ++y) {
				std::set<uint32_t> * realSet = m_srcGrid.binAt(x,y);
				std::set<uint32_t> testSet = m_grid.indexAt(x, y).toSet();
				std::stringstream ss;
				ss << "ItemIndex at("  << x << ", " << y << ")";  
				if (!realSet) {
					CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), static_cast<size_t>(0), testSet.size());
				}
				else {
					CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), realSet->size(), testSet.size());
					CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), *realSet, testSet);
				}
			}
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	SSERIALIZE_TESTS_EPS = 0.000001;
	
	CppUnit::TextUi::TestRunner runner;
// 	runner.addTest( StaticItemGeoGridTest<1, 1>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<3, 2>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<3, 3>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<4, 4>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<5, 5>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<5, 5>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<6, 6>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<7, 7>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<8, 8>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<9, 9>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<7, 11>::suite() );
// 	runner.addTest( StaticItemGeoGridTest<20, 20>::suite() );
	runner.addTest( StaticItemGeoGridTest<200, 200>::suite() );
	runner.eventManager().popProtector();
	bool ok = runner.run();
	return ok ? 0 : 1;
};