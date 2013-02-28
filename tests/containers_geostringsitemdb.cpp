#include <iostream>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/templated/GeoStringsItemDB.h>
#include <sserialize/Static/GeoStringsItemDB.h>
#include <sserialize/spatial/GeoWay.h>
#include "TestItemData.h"

using namespace sserialize;

typedef spatial::GeoWay<spatial::GeoPoint, std::vector<spatial::GeoPoint>> MyGeoWay;

class GeoStringsItemDBTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( GeoStringsItemDBTest );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST( testStrings );
CPPUNIT_TEST( testPoints );
CPPUNIT_TEST_SUITE_END();
public:
	typedef GeoStringsItemDB<TestItemData> MyDBType;
	typedef typename GeoStringsItemDB<TestItemData>::Item MyDBItemType;
private:
	std::deque<TestItemData> m_data; 
	GeoStringsItemDB<TestItemData> m_db;
public:
	virtual void setUp() {
		m_data = createSampleData();
		for(std::deque<TestItemData>::const_iterator it = m_data.begin(); it != m_data.end(); ++it) {
			if (it->points.size() == 0)
				m_db.insert(it->strs, *it, 0);
			else if (it->points.size() == 1)
				m_db.insert(it->strs, *it, new spatial::GeoPoint(it->points.front()));
			else
				m_db.insert(it->strs, *it, new MyGeoWay(it->points) );
		}
	}
	virtual void tearDown() {}
	
	void testSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("DB sizes don't match", m_data.size(), m_db.size());
	}
	
	void testStrings() {
		std::stringstream ss;
		CPPUNIT_ASSERT_EQUAL(m_data.size(), m_db.size());
		for(size_t i = 0; i < m_db.size(); i++) {
			MyDBItemType item = m_db.at(i);
			TestItemData & realItem = m_data.at(i);
			ss << "Testing Item" << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": string-count failed", static_cast<uint32_t>(realItem.strs.size()), item.strCount());
			uint32_t stringCount = item.strCount();
			for(size_t j = 0; j < stringCount; j++) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": string-equality failed", realItem.strs.at(j), item.strAt(j));
			}
		}
	}
	void testPoints() {
		std::stringstream ss;
		CPPUNIT_ASSERT_EQUAL(m_data.size(), m_db.size());
		for(size_t i = 0; i < m_db.size(); i++) {
			MyDBItemType item = m_db.at(i);
			TestItemData & realItem = m_data.at(i);
			ss << "Testing Item" << i;
			uint32_t realPointCount = realItem.points.size();
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": geopoint-count failed", realPointCount, (item.geoShape() ? item.geoShape()->size() : 0) );
			
			if (realPointCount == 1) {
				const spatial::GeoPoint * gp = dynamic_cast<const spatial::GeoPoint*>( item.geoShape() );
				CPPUNIT_ASSERT(gp);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": geopoint-lat-equality failed", realItem.points.front().lat, gp->lat);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": geopoint-lon-equality failed", realItem.points.front().lon, gp->lon);
			}
			else if (realPointCount > 1) {
				const MyGeoWay * gw = dynamic_cast<const MyGeoWay*>( item.geoShape() );
				CPPUNIT_ASSERT(gw);
				for(size_t j = 0; j < realPointCount; j++) {
					CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": geopoint-lat-equality failed", realItem.points.at(j).lat, gw->points().at(j).lat);
					CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": geopoint-lon-equality failed", realItem.points.at(j).lon, gw->points().at(j).lon);
				}
			}
			else {
				CPPUNIT_ASSERT( !item.geoShape() );
			}
		}
	}
};


int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( GeoStringsItemDBTest::suite() );
	runner.run();
	return 0;
}