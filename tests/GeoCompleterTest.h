#ifndef SSERIALIZE_TESTS_GEO_COMPLETER_TEST_H
#define SSERIALIZE_TESTS_GEO_COMPLETER_TEST_H
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/completers/GeoCompleter.h>
#include <sserialize/templated/GeoStringsItemDB.h>
#include "TestItemData.h"
#include "datacreationfuncs.h"
#include <sserialize/utility/printers.h>

namespace sserialize {


typedef spatial::GeoWay<spatial::GeoPoint, std::vector<spatial::GeoPoint> > MyGeoWay;


/** Items with a single Point should be a point, others a way */
class GeoCompleterTest: public CppUnit::TestFixture {
private:
	std::deque<TestItemData> m_items;
	SamplePolygonTestData m_geoData;
private:
	std::set<uint32_t> itemsWithinRect(uint32_t rectId) {
		std::set<uint32_t> & polyIds = m_geoData.rectPolyIds[rectId];
		std::set<uint32_t> & pointIds = m_geoData.rectPointIds[rectId];
		std::set<uint32_t> ret;
		for(size_t i = 0; i < m_items.size(); ++i) {
			if (m_items[i].points.size() == 1 && pointIds.count( m_items[i].geoId ) > 0)
				ret.insert(i);
			else if (m_items[i].points.size() > 1 && polyIds.count( m_items[i].geoId ) > 0)
				ret.insert(i);
		}
		return ret;
	}
protected:
	virtual GeoCompleter & geoCompleter() = 0;
	const std::deque<TestItemData> & items() { return m_items;}
	
	GeoStringsItemDB<TestItemData> createDB() const {
		GeoStringsItemDB<TestItemData> db;
		for(std::deque<TestItemData>::const_iterator it = m_items.begin(); it != m_items.end(); ++it) {
			if (it->points.size() == 0)
				db.insert(it->strs, *it, 0);
			else if (it->points.size() == 1)
				db.insert(it->strs, *it, new spatial::GeoPoint(it->points.front()));
			else
				db.insert(it->strs, *it, new MyGeoWay(it->points) );
		}
		return db;
	}

public:
	GeoCompleterTest() : CppUnit::TestFixture() {
		m_items = createSampleData();
		createHandSamplePolygons(m_geoData);
		
	}
    virtual ~GeoCompleterTest() {}
	
	/** Derived classes need to set the stringCompleter via stringCompleter() */
	virtual void setUp() {}
	
	/** derived classes need to do clean-up of data-structures associated with StringCompleter */
	virtual void tearDown() {}

	void testGeoCompletion() {
		for(size_t i = 0; i < m_geoData.rects.size(); i++) {
			ItemIndex idx = geoCompleter().complete(m_geoData.rects.at(i), false);
			std::set<uint32_t> retIds = idx.toSet();
			std::set<uint32_t> realIds = itemsWithinRect(i);
			std::stringstream ss;
			ss << "Rect(" << i << "): " << m_geoData.rects.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), realIds, retIds);
		}
	}
	
	void testPartialGeoCompletion() {
		for(size_t i = 0; i < m_geoData.rects.size(); i++) {
			ItemIndex idx = geoCompleter().partialComplete(m_geoData.rects.at(i), false).toItemIndex();
			std::set<uint32_t> retIds = idx.toSet();
			std::set<uint32_t> realIds = itemsWithinRect(i);
			std::stringstream ss;
			ss << "Rect(" << i << "): " << m_geoData.rects.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), realIds, retIds);
		}
	}
};

}

#endif