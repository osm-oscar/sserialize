#include <iostream>
#include "containers/GeoStringsItemDB.h"
#include "Static/GeoStringsItemDB.h"
#include <sserialize/spatial/GeoWay.h>
#include "TestItemData.h"
#include "utilalgos.h"
#include "TestBase.h"
#define EPS 0.000025


using namespace sserialize;

typedef spatial::GeoWay MyGeoWay;

class StaticGeoStringsItemDBTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( StaticGeoStringsItemDBTest );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST( testStrings );
CPPUNIT_TEST( testPoints );
CPPUNIT_TEST( testPayload );
CPPUNIT_TEST_SUITE_END();
public:
	typedef GeoStringsItemDB<TestItemData> MyDBType;
	typedef typename GeoStringsItemDB< TestItemData >::Item MyDBItemType;
	
	typedef Static::GeoStringsItemDB<TestItemData> MyStaticDBType;
	typedef typename Static::GeoStringsItemDB<TestItemData>::Item MyStaticDBItemType;
private:
	GeoStringsItemDB<TestItemData> m_db;
	std::vector<uint8_t> m_sdbData;
	Static::GeoStringsItemDB<TestItemData> m_sdb;
public:
	virtual void setUp() {
		std::deque<TestItemData> data = createSampleData();
		for(std::deque<TestItemData>::const_iterator it = data.begin(); it != data.end(); ++it) {
			if (it->points.size() == 0)
				m_db.insert(it->strs, *it, 0);
			else if (it->points.size() == 1)
				m_db.insert(it->strs, *it, new spatial::GeoPoint(it->points.front()));
			else
				m_db.insert(it->strs, *it, new MyGeoWay(it->points) );
		}
		UByteArrayAdapter stableAdap(new std::vector<uint8_t>(), true);
		UByteArrayAdapter sdbAdap(&m_sdbData);
		m_db.createStaticStringTable(stableAdap);
		sdbAdap << m_db;
		
		m_sdb = MyStaticDBType(sdbAdap, stableAdap);

	}
	virtual void tearDown() {}
	
	void testSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("DB sizes don't match", static_cast<uint32_t>( m_db.size() ), m_sdb.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Reported sizeInBytes don't match", (sserialize::UByteArrayAdapter::SizeType) m_sdbData.size(), m_sdb.getSizeInBytes());
	}
	
	void testStrings() {
		std::stringstream ss;
		CPPUNIT_ASSERT_EQUAL(static_cast<uint32_t>( m_db.size() ), m_sdb.size());
		for(uint32_t i = 0; i < m_db.size(); i++) {
			MyDBItemType item = m_db.at(i);
			MyStaticDBItemType sitem = m_sdb.at(i);
			ss << "Testing Item" << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": string-count failed", item.strCount(), sitem.strCount());
			uint32_t stringCount = item.strCount();
			for(uint32_t j = 0; j < stringCount; j++) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": string-equality failed", item.strAt(j), sitem.strAt(j));
			}
		}
	}
	void testPoints() {
		std::stringstream ss;
		CPPUNIT_ASSERT_EQUAL(static_cast<uint32_t>( m_db.size() ), m_sdb.size());
		for(uint32_t i = 0; i < m_db.size(); i++) {
			MyDBItemType item = m_db.at(i);
			MyStaticDBItemType sitem = m_sdb.at(i);
			ss << "Testing Item" << i;
			uint32_t realPointCount = (item.geoShape() ? item.geoShape()->size() : 0);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": geopoint-count failed", realPointCount, sitem.geoPointCount());
			
			if (realPointCount == 1) {
				const spatial::GeoPoint * gp = dynamic_cast<const spatial::GeoPoint*>( item.geoShape() );
				CPPUNIT_ASSERT(gp);
				std::stringstream ss2; ss2 << "[" << gp->lat() << ", " << gp->lon() << "]!=[" << sitem.geoPointAt(0).lat() << ", " << sitem.geoPointAt(0).lon() << "]";
				CPPUNIT_ASSERT_MESSAGE(ss.str() + ":" + ss2.str(), doubleEq(gp->lat(), sitem.geoPointAt(0).lat(), EPS));
				CPPUNIT_ASSERT_MESSAGE(ss.str() + ":" + ss2.str(), doubleEq(gp->lon(), sitem.geoPointAt(0).lon(), EPS));
			}
			else if (realPointCount > 1) {
				const MyGeoWay * gw = dynamic_cast<const MyGeoWay*>( item.geoShape() );
				CPPUNIT_ASSERT(gw);
				for(uint32_t j = 0; j < realPointCount; j++) {
					std::stringstream ss2; ss2 << "[" << gw->points().at(j).lat() << ", " << gw->points().at(j).lon() << "]!=[" << sitem.geoPointAt(j).lat() << ", " << sitem.geoPointAt(j).lon() << "]";
					CPPUNIT_ASSERT_MESSAGE(ss.str() + ":" + ss2.str(), doubleEq(gw->points().at(j).lat(), sitem.geoPointAt(j).lat(), EPS));
					CPPUNIT_ASSERT_MESSAGE(ss.str() + ":" + ss2.str(), doubleEq(gw->points().at(j).lon(), sitem.geoPointAt(j).lon(), EPS));
				}
			}
			else {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": GeoShapeType is not GS_NONE", sserialize::spatial::GS_NONE, sitem.geoShapeType());
			}
		}
	}
	
	void testPayload() {
		std::stringstream ss;
		CPPUNIT_ASSERT_EQUAL(static_cast<uint32_t>( m_db.size() ), m_sdb.size());
		for(uint32_t i = 0; i < m_db.size(); i++) {
			MyDBItemType item = m_db.at(i);
			MyStaticDBItemType sitem = m_sdb.at(i);
			ss << "Testing Item" << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": Payload missmatch", m_db.items().at(i).id , sitem.data().id);
		}
	}
};


int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticGeoStringsItemDBTest::suite() );
	runner.run();
	return 0;
}