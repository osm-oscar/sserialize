#include <algorithm>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/spatial/GridRegionTree.h>
#include "datacreationfuncs.h"
#include "utilalgos.h"
#include <sserialize/utility/printers.h>

using namespace sserialize;

template<uint32_t T_RINIT_X, uint32_t T_RINIT_Y, uint32_t T_RF_X, uint32_t T_RF_Y>
class GridRegionTreeTest:public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( GridRegionTreeTest );
CPPUNIT_TEST( testPointIntersect );
CPPUNIT_TEST( testOutOfBounds );
CPPUNIT_TEST_SUITE_END();
private:
	SamplePolygonTestData m_data;
	spatial::GridRegionTree m_grt;
	std::unordered_map<spatial::GeoRegion*, uint32_t> m_grIdMap;
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
	
	std::set<uint32_t> polysIntersectingPoint(const sserialize::spatial::GeoPoint & p) {
		std::vector<spatial::GeoRegion*> regions;
		typedef std::back_insert_iterator< std::vector<spatial::GeoRegion*> > MyInserIt; 
		m_grt.find(p, MyInserIt(regions), MyInserIt(regions));
		std::set<uint32_t> res;
		for(spatial::GeoRegion * r : regions) {
			res.insert( m_grIdMap.at(r) );
		}
		return res;
	}
	
	std::set<uint32_t> polysIntersectingPoint(uint32_t point) {
		const sserialize::spatial::GeoPoint & p = m_data.points[point];
		return polysIntersectingPoint(p);
	}
	
	std::set<uint32_t> realPolysIntersectingPoint(uint32_t point) {
		std::set<uint32_t> res;
		for(std::set< std::pair<uint32_t, uint32_t> >::const_iterator it = m_data.pointPolyIntersects.begin(); it != m_data.pointPolyIntersects.end() ;++it) {
			if (it->first == point)
				res.insert((*it).second);
		}
		return res;
	}

public:

	virtual void setUp() {
		createHandSamplePolygons(m_data);
		for (size_t i = 0; i < m_data.polys.size(); ++i) {
			m_grIdMap[&(m_data.polys[i].first)] =  m_data.polys[i].second;
		}
		if (T_RINIT_X && T_RINIT_Y) {
			std::vector<spatial::GeoRegion*> regions;
			regions.reserve(m_grIdMap.size());
			for(auto & x : m_grIdMap) {
				regions.push_back(x.first);
			}
			spatial::GeoRect initialRect = spatial::GeoShape::bounds(regions.cbegin(), regions.cend());
			spatial::detail::GridRegionTree::FixedSizeRefiner refiner(0.1, 0.1, T_RF_X, T_RF_Y);
			m_grt = spatial::GridRegionTree(spatial::GeoGrid(initialRect, T_RINIT_X, T_RINIT_Y), regions.begin(), regions.end(), refiner);
		}
	}

	virtual void tearDown() {}
	
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
		CPPUNIT_ASSERT_NO_THROW_MESSAGE("Single-Point Test created an exception", t = polysIntersectingPoint(spatial::GeoPoint(137.0, 137.0)) );
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Sinlge Point TestSet is not empty", std::set<uint32_t>(), t);
	}
};

int main() {
	srand( 0 );
// 	foo();
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( GridRegionTreeTest<1,1,2,2>::suite() );
	runner.run();
	return 0;
}