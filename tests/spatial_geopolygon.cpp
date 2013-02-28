#include <algorithm>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/utility/utilfuncs.h>
#include "datacreationfuncs.h"
#include "utilalgos.h"

using namespace sserialize;

class GeoPolygonTest:public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( GeoPolygonTest );
CPPUNIT_TEST( testIntersect );
CPPUNIT_TEST( testPointIntersect );
CPPUNIT_TEST_SUITE_END();
private:
	SamplePolygonTestData m_data;
private:
	std::string brokenPolyIntersect(uint32_t i, uint32_t j) {
		std::stringstream ss;
		ss << "Broken poly-collide between "  << i << " and " << j << std::endl;
		return ss.str();
	}
	
	std::string brokenPolyPointIntersect(uint32_t point, uint32_t poly) {
		std::stringstream ss;
		ss << "Broken point/poly intersect between "  << point << " and " << poly << std::endl;
		return ss.str();
	}
public:

	virtual void setUp() {
		createHandSamplePolygons(m_data);
	}

	virtual void tearDown() {}
	
	void testIntersect() {
		for(size_t i = 0; i < m_data.polys.size(); ++i) {
			for(size_t j = 0; j < m_data.polys.size(); ++j) {
				std::string msg = brokenPolyIntersect(i, j);
				bool shouldIntersect = (m_data.polyIntersects.count(std::pair<uint32_t, uint32_t>(i,j)) > 0 || i == j);
				bool intersectResult = m_data.polys.at(i).first.collidesWithPolygon(m_data.polys.at(j).first);
				if (shouldIntersect != intersectResult)
					intersectResult = m_data.polys.at(i).first.collidesWithPolygon(m_data.polys.at(j).first);
				
				CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, shouldIntersect, intersectResult);
			}
		}
	}
	
	void testPointIntersect() {
		for(size_t polyIt = 0; polyIt < m_data.polys.size(); ++polyIt) {
			for(size_t pointsIt = 0; pointsIt < m_data.points.size(); ++pointsIt) {
				std::string msg = brokenPolyPointIntersect(polyIt, pointsIt);
				bool shouldIntersect = m_data.pointPolyIntersects.count(std::pair<uint32_t, uint32_t>(pointsIt,polyIt)) > 0;
				bool intersectResult = m_data.polys.at(polyIt).first.test( m_data.points.at(pointsIt) );
				if (intersectResult != shouldIntersect)
					intersectResult = m_data.polys.at(polyIt).first.test( m_data.points.at(pointsIt) );

				CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, shouldIntersect, intersectResult);
			}
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( GeoPolygonTest::suite() );
	runner.run();
	return 0;
}
