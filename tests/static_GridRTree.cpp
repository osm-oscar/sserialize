#include "GeoCompleterTest.h"
#include <sserialize/Static/RTreeGeoDBCompleter.h>
#include "Static/GeoStringsItemDB.h"
#include <sserialize/Static/RTree.h>
#include <sserialize/spatial/GridRTree.h>
#include "utilalgos.h"
#define EPS 0.000025

using namespace sserialize;

template<uint32_t latcount, uint32_t loncount>
class StaticGridRTreeTest: public GeoCompleterTest {
CPPUNIT_TEST_SUITE( StaticGridRTreeTest );
CPPUNIT_TEST( testGeoCompletion );
CPPUNIT_TEST( testPartialGeoCompletion );
CPPUNIT_TEST_SUITE_END();
public:
	typedef GeoStringsItemDB<TestItemData> MyDBType;
	typedef Static::GeoStringsItemDB<TestItemData> MySDBType;
	typedef Static::spatial::RTree MyStaticRTree;
	typedef Static::spatial::RTreeGeoDBCompleter<MySDBType> MySGeoCompleterPrivateType;
private:

	struct ItemBoundaryGenerator {
		MyDBType * c;
		sserialize::spatial::GeoShape * g;
		uint32_t p;
		ItemBoundaryGenerator(MyDBType * c) : c(c), p(0) {}
		inline bool valid() const { return p < c->size(); }
		inline uint32_t id() const { return p; }
		inline const sserialize::spatial::GeoShape * shape() {
			return c->geoShape(p);
		}
		void next() { ++p; }
	};

private:
	MySDBType m_sdb;
	Static::ItemIndexStore m_indexStore;
	spatial::GridRTree * m_srcGridRTree;
    MyStaticRTree m_srtree;
    GeoCompleter m_geoCmp;
    sserialize::spatial::GeoRect m_rect;
private:
	virtual GeoCompleter & geoCompleter() {
		return m_geoCmp;
	}
public:
	virtual void setUp() {
		GeoStringsItemDB<TestItemData> srcDB = createDB();
		m_rect = bbox(srcDB);
		m_srcGridRTree = new spatial::GridRTree(spatial::GeoRect(0, 20, 0, 20), latcount, loncount);

		ItemBoundaryGenerator generator(&srcDB);
		m_srcGridRTree->bulkLoad(generator);
		
		ItemIndexFactory indexFactory(true);
		indexFactory.setType(ItemIndex::T_WAH);
		UByteArrayAdapter gridAdap(new std::vector<uint8_t>(), true);
		UByteArrayAdapter idxStoreAdap;
		UByteArrayAdapter stableAdap(new std::vector<uint8_t>(), true);
		UByteArrayAdapter sdbAdap(new std::vector<uint8_t>(), true);
		srcDB.createStaticStringTable(stableAdap);
		sdbAdap << srcDB;
		m_srcGridRTree->serialize(gridAdap, indexFactory);
		indexFactory.flush();
		idxStoreAdap = indexFactory.getFlushedData();
		
		//set the all up
		m_sdb = MySDBType(sdbAdap, stableAdap);
		m_indexStore = Static::ItemIndexStore(idxStoreAdap);
		m_srtree = MyStaticRTree(gridAdap, m_indexStore);
		m_geoCmp = GeoCompleter( new MySGeoCompleterPrivateType(m_sdb, m_srtree) );
		
// 		m_grid.dump();
	}
	
	/** derived classes need to do clean-up of data-structures associated with StringCompleter */
	virtual void tearDown() {
		delete m_srcGridRTree;
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::argc = argc;
	sserialize::tests::TestBase::argv = argv;
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticGridRTreeTest<1, 1>::suite() );
	runner.addTest( StaticGridRTreeTest<2, 2>::suite() );
	runner.addTest( StaticGridRTreeTest<3, 3>::suite() );
	runner.addTest( StaticGridRTreeTest<4, 4>::suite() );
	runner.addTest( StaticGridRTreeTest<5, 5>::suite() );
	runner.addTest( StaticGridRTreeTest<6, 6>::suite() );
	runner.addTest( StaticGridRTreeTest<7, 7>::suite() );
	runner.addTest( StaticGridRTreeTest<8, 8>::suite() );
	runner.addTest( StaticGridRTreeTest<9, 9>::suite() );
	runner.addTest( StaticGridRTreeTest<3, 2>::suite() );
	runner.addTest( StaticGridRTreeTest<7, 11>::suite() );
	runner.addTest( StaticGridRTreeTest<20, 20>::suite() );
	runner.addTest( StaticGridRTreeTest<13, 31>::suite() );
	runner.addTest( StaticGridRTreeTest<200, 200>::suite() );
	runner.run();
	return 0;
};