#include <algorithm>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/spatial/polygonstore.h>
#include "datacreationfuncs.h"
#include "utilalgos.h"

using namespace sserialize;

std::ostream & operator<<(std::ostream & out, const std::set<uint32_t> & s) {
	if (!s.size())
		return (out << "[]");
	out << "[";
	std::set<uint32_t>::const_iterator it = s.cbegin();
	while(true) {
		out << *it;
		++it;
		if (it == s.cend())
			break;
		out << ", ";
	}
	out << "]";
	return out;
}

std::ostream & operator<<(std::ostream & out, const sserialize::spatial::GeoPoint & p) {
	out << "GeoPoint[" << p.lat << ", " << p.lon << "]";
	return out;
}

template<uint32_t T_RASTER_X, uint32_t T_RASTER_Y>
class GeoPolygonStoreTest:public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( GeoPolygonStoreTest );
CPPUNIT_TEST( testIntersect );
CPPUNIT_TEST( testPointIntersect );
CPPUNIT_TEST( testOutOfBounds );
CPPUNIT_TEST_SUITE_END();
private:
	SamplePolygonTestData m_data;
	spatial::PolygonStore<uint32_t> m_polyStore;
private:
	std::string brokenPolyPolyIntersect(uint32_t poly) {
		std::stringstream ss;
		ss << "Broken poly-collide between "  << poly << std::endl;
		return ss.str();
	}
	
	std::string brokenPolyPointIntersect(uint32_t point) {
		std::stringstream ss;
		ss << "Broken point/poly intersect with point " << point << ": ";
		ss << m_data.points.at(point);
		ss << std::endl;
		return ss.str();
	}
	
	std::set<uint32_t> polysIntersectingPoint(uint32_t point) {
		std::set<uint32_t> pIds = m_polyStore.test(m_data.points[point]);
		std::set<uint32_t> res;
		for(std::set<uint32_t>::const_iterator it = pIds.begin(); it != pIds.end(); ++it) {
			res.insert( m_polyStore.values().at(*it) );
		}
		return res;
	}
	
	std::set<uint32_t> realPolysIntersectingPoint(uint32_t point) {
		std::set<uint32_t> res;
		for(std::set< std::pair<uint32_t, uint32_t> >::const_iterator it = m_data.pointPolyIntersects.begin(); it != m_data.pointPolyIntersects.end() ;++it) {
			if (it->first == point)
				res.insert((*it).second);
		}
		return res;
	}

	std::set<uint32_t> polysIntersectingPoly(uint32_t poly) {
		std::set<uint32_t> pIds = m_polyStore.test(m_data.polys[poly].first.points());
		std::set<uint32_t> res;
		for(std::set<uint32_t>::const_iterator it = pIds.begin(); it != pIds.end(); ++it) {
			res.insert( m_polyStore.values().at(*it));
		}
		return res;
	}
	
	std::set<uint32_t> realPolysIntersectingPoly(uint32_t poly) {
		uint32_t polyId = m_data.polys.at(poly).second;
		std::set<uint32_t> res;
		res.insert(polyId);
		for(std::set< std::pair<uint32_t, uint32_t> >::const_iterator it = m_data.polyPointsInOtherPolys.begin(); it != m_data.polyPointsInOtherPolys.end() ;++it) {
			if ((*it).first == polyId)
				res.insert((*it).second);
		}
		return res;
	}
	
public:

	virtual void setUp() {
		createHandSamplePolygons(m_data);
		for (size_t i = 0; i < m_data.polys.size(); ++i) {
			m_polyStore.push_back(m_data.polys[i].first, m_data.polys[i].second);
		}
		if (T_RASTER_X && T_RASTER_Y)
			m_polyStore.addPolygonsToRaster(T_RASTER_X, T_RASTER_Y);
		
		std::cout << m_data.points.at(0) << std::endl;

	}

	virtual void tearDown() {}
	
	void testIntersect() {
		for(size_t polyIt = 0; polyIt < m_data.polys.size(); ++polyIt) {
			std::string msg = brokenPolyPolyIntersect(polyIt);
			std::set<uint32_t> intersectingPolys = polysIntersectingPoly(polyIt);
			std::set<uint32_t> realIntersectingPolys = realPolysIntersectingPoly(polyIt);

			//TODO: Should we define points lying directly on the border as points within?
			//What about precission?
			//Let's just skip this for now

			intersectingPolys.erase(m_data.polys[polyIt].second);
			realIntersectingPolys.erase(m_data.polys[polyIt].second);
			
			if (realIntersectingPolys != intersectingPolys)
				intersectingPolys = polysIntersectingPoly(polyIt);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, realIntersectingPolys, intersectingPolys);
		}
	}
	
	void testPointIntersect() {
		for(size_t pointsIt = 0; pointsIt < m_data.points.size(); ++pointsIt) {
			std::string msg = brokenPolyPointIntersect(pointsIt);
			std::set<uint32_t> intersectingPolys = polysIntersectingPoint(pointsIt);
			std::set<uint32_t> realIntersectingPolys = realPolysIntersectingPoint(pointsIt);
			if (realIntersectingPolys != intersectingPolys)
				intersectingPolys = polysIntersectingPoint(pointsIt);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, realIntersectingPolys, intersectingPolys);
		}
	}
	
	void testOutOfBounds() {
		std::set<uint32_t> t;
		CPPUNIT_ASSERT_NO_THROW_MESSAGE("Single-Point Test created an exception", t =m_polyStore.test(spatial::GeoPoint(137.0, 137.0)) );
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Sinlge Point TestSet is not empty", std::set<uint32_t>(), t);
		
		std::vector< spatial::GeoPoint > way;
		way.push_back( spatial::GeoPoint(100.0, 100.0));
		way.push_back( spatial::GeoPoint(100.0, 137.0));
		way.push_back( spatial::GeoPoint(137.0, 137.0));
		CPPUNIT_ASSERT_NO_THROW_MESSAGE("Complete out-of-bounds Way Test created an exception", t = m_polyStore.test( way ) );
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Complete Out-of-Bounds TestSet is not empty", std::set<uint32_t>(), t);
		
		way.clear();
		way.push_back(spatial::GeoPoint(1.5, 2.0) );
		way.push_back(spatial::GeoPoint(2.0, 2.0) );
		way.push_back( spatial::GeoPoint(10.0, 10.0) );
		way.push_back( spatial::GeoPoint(10.0, 13.7) );
		way.push_back( spatial::GeoPoint(23.0, 13.7) );
		CPPUNIT_ASSERT_NO_THROW_MESSAGE("partial out-of-bounds Way Test created an exception", t = m_polyStore.test( way ) );
		CPPUNIT_ASSERT_MESSAGE("partial out-of-bounds Way returned empty set", std::set<uint32_t>() != t);
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( GeoPolygonStoreTest<0,0>::suite() );
	runner.addTest( GeoPolygonStoreTest<1,1>::suite() );
	runner.addTest( GeoPolygonStoreTest<2,2>::suite() );
	runner.addTest( GeoPolygonStoreTest<2,3>::suite() );
	runner.addTest( GeoPolygonStoreTest<3,2>::suite() );
	runner.addTest( GeoPolygonStoreTest<6,5>::suite() );
	runner.addTest( GeoPolygonStoreTest<12,10>::suite() );
	runner.addTest( GeoPolygonStoreTest<47,23>::suite() );
	runner.run();
	return 0;
}