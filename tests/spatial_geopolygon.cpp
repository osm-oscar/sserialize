#include <algorithm>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/log.h>
#include "datacreationfuncs.h"
#include "utilalgos.h"
#include "TestBase.h"
#include <sserialize/spatial/GeoGrid.h>

using namespace sserialize;

class GeoPolygonTest:public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( GeoPolygonTest );
CPPUNIT_TEST( testIntersect );
CPPUNIT_TEST( testPointIntersect );
CPPUNIT_TEST( testEnclosing );
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
		for(uint32_t i = 0; i < m_data.polys.size(); ++i) {
			for(uint32_t j = 0; j < m_data.polys.size(); ++j) {
				std::string msg = brokenPolyIntersect(i, j);
				bool shouldIntersect = (m_data.polyIntersects.count(std::pair<uint32_t, uint32_t>(i,j)) > 0 || i == j);
				bool intersectResult = m_data.polys.at(i).first.intersects(m_data.polys.at(j).first);
				if (shouldIntersect != intersectResult)
					intersectResult = m_data.polys.at(i).first.intersects(m_data.polys.at(j).first);
				
				CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, shouldIntersect, intersectResult);
			}
		}
	}
	
	void testPointIntersect() {
		for(uint32_t polyIt = 0; polyIt < m_data.polys.size(); ++polyIt) {
			for(uint32_t pointsIt = 0; pointsIt < m_data.points.size(); ++pointsIt) {
				std::string msg = brokenPolyPointIntersect(polyIt, pointsIt);
				bool shouldIntersect = m_data.pointPolyIntersects.count(std::pair<uint32_t, uint32_t>(pointsIt,polyIt)) > 0;
				bool intersectResult = m_data.polys.at(polyIt).first.contains( m_data.points.at(pointsIt) );
				if (intersectResult != shouldIntersect)
					intersectResult = m_data.polys.at(polyIt).first.contains( m_data.points.at(pointsIt) );

				CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, shouldIntersect, intersectResult);
			}
		}
	}
	
	void testEnclosing() {
	double scaleFactors[6] = {0.1, 0.2, 0.4, 0.8, 1.6, 3.2};
		for(uint32_t i = 0; i < m_data.polys.size(); ++i) {
			sserialize::spatial::GeoPolygon rectPoly( sserialize::spatial::GeoPolygon::fromRect(m_data.poly(i).boundary()) );
			for(uint32_t j = 0; j < 6; ++j) {
				sserialize::spatial::GeoRect testRect = rectPoly.boundary();
				testRect.resize(scaleFactors[j], scaleFactors[j]);
				bool shouldResult = (scaleFactors[j] < 1.0);
				bool res = rectPoly.encloses(sserialize::spatial::GeoPolygon::fromRect(testRect));
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString(rectPoly.boundary(), " in? ", testRect), shouldResult, res);
			}
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( GeoPolygonTest::suite() );
	runner.run();
	return 0;
}
