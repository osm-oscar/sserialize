#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/Static/GeoShape.h>
#include <sserialize/Static/GeoWay.h>
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/Static/GeoMultiPolygon.h>
#include <sserialize/utility/log.h>
#include "datacreationfuncs.h"
#include "helpers.h"

sserialize::spatial::GeoPoint createPoint() {
	sserialize::spatial::GeoPoint prev;
	prev.lat() = ((double)rand()/RAND_MAX)*180.0-90.0;
	prev.lon() = ((double)rand()/RAND_MAX)*360.0-180.0;
	prev.normalize();
	return prev;
}

std::vector<sserialize::spatial::GeoPoint> createPoints(uint32_t count, double maxdiff) {
	std::vector<sserialize::spatial::GeoPoint> res;
	res.reserve(count);
	
	sserialize::spatial::GeoPoint prev;
	prev.lat() = ((double)rand()/RAND_MAX)*180.0-90.0;
	prev.lon() = ((double)rand()/RAND_MAX)*360.0-180.0;
	
	res.push_back(prev);
	
	for(; count > 0; --count) {
		double difflat = ((double)rand()/RAND_MAX)*maxdiff;
		double difflon = ((double)rand()/RAND_MAX)*maxdiff;
		prev.lat() += difflat;
		prev.lon() += difflon;
		prev.normalize();
		res.push_back(prev);
	}
	return res;
}

uint32_t gwLen = 100;
double maxDiff = 0.5;

class StaticGeoWayTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( StaticGeoWayTest );
CPPUNIT_TEST( testRect );
CPPUNIT_TEST( testPoint );
CPPUNIT_TEST( testWay );
CPPUNIT_TEST( testPolygon );
CPPUNIT_TEST( testMultiPolygon );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}

	void testRect() {
		sserialize::OffsetType geoRectLen =sserialize::SerializationInfo<sserialize::spatial::GeoRect>::length;
		for(uint32_t i = 0; i < gwLen; ++i) {
			std::vector<sserialize::spatial::GeoPoint> srcPts = createPoints(2, 10.0);
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("GeoPoint0 invalid at ", i), srcPts[0].valid());
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("GeoPoint1 invalid at ", i), srcPts[1].valid());
			sserialize::spatial::GeoRect srcRect(srcPts[0].lat(), srcPts[1].lat(), srcPts[0].lon(), srcPts[1].lon());
			sserialize::spatial::GeoRect tRect, stRect;
			
			std::vector<uint8_t> ds;
			sserialize::UByteArrayAdapter d(&ds, false);
			d << srcRect;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("data size at ", i), geoRectLen, d.size());
			
			d.resetPtrs();
			d >> tRect;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("streaming rect at ", i), srcRect, tRect);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("getptr increase", geoRectLen, d.tellGetPtr());
			
			d.resetPtrs();
			stRect = sserialize::spatial::GeoRect(d);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("ctor rect at ", i), srcRect, stRect);
		}
	}
	
	void testPoint() {
		sserialize::OffsetType geoPointLen = sserialize::SerializationInfo<sserialize::spatial::GeoPoint>::length;
		for(uint32_t i = 0; i < gwLen; ++i) {
			std::vector<uint8_t> ds;
			sserialize::UByteArrayAdapter d(&ds, false);

			sserialize::spatial::GeoPoint srcPt = createPoint();
			sserialize::spatial::GeoPoint tPt, stPt;
				
			d << srcPt;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("data size at ", i), geoPointLen, d.size());

			
			d.resetPtrs();
			d >> tPt;
			CPPUNIT_ASSERT_EQUAL_MESSAGE("getptr increase", geoPointLen, d.tellGetPtr());
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("streaming point at ", i), srcPt, tPt);

			d.resetPtrs();
			stPt = sserialize::spatial::GeoPoint(d);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("ctor point at ", i), srcPt, stPt);

			d.resetPtrs();
			uint32_t intLat = d.getUint32(0);
			uint32_t intLon = d.getUint32(4);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("intLat", sserialize::spatial::GeoPoint::toIntLat(srcPt.lat()), intLat);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("intLon", sserialize::spatial::GeoPoint::toIntLon(srcPt.lon()), intLon);
		}
	}
	
	void testWay() {
		std::vector<sserialize::spatial::GeoPoint> pts = createPoints(gwLen, maxDiff);
		sserialize::spatial::GeoShape * gw = new sserialize::spatial::GeoWay(pts);
		std::vector<uint8_t> rd;
		sserialize::UByteArrayAdapter d(&rd, false);
		sserialize::spatial::GeoShape::appendWithTypeInfo(gw, d);
		d.resetPtrs();
		sserialize::Static::spatial::GeoShape gs(d);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("type", gs.type(), sserialize::spatial::GeoShapeType::GS_WAY);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", (sserialize::OffsetType)rd.size(), gs.getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", gw->size(), gs.size());
		const sserialize::Static::spatial::GeoWay * sgw = gs.get<sserialize::Static::spatial::GeoWay>();
		CPPUNIT_ASSERT_MESSAGE("static geoway cast", sgw);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("boundary", gw->boundary(), gs.boundary());
		for(uint32_t i = 0, s = pts.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("GeoPoint at ", i), pts.at(i), sgw->points().at(i));
		}
	}
	
	void testPolygon() {
		sserialize::SamplePolygonTestData testData;
		sserialize::createHandSamplePolygons(testData);
		for(uint32_t i = 0, s = testData.polys.size(); i < s; ++i) {
			sserialize::spatial::GeoPolygon * gpoly = &(testData.polys[i].first);
			sserialize::spatial::GeoShape * gs = gpoly;
			std::vector<uint8_t> ds;
			sserialize::UByteArrayAdapter d(&ds, false);
			sserialize::spatial::GeoShape::appendWithTypeInfo(gs, d);
			d.resetPtrs();
			
			sserialize::Static::spatial::GeoShape sgs(d);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("type", sgs.type(), sserialize::spatial::GeoShapeType::GS_POLYGON);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", (sserialize::OffsetType)d.size(), sgs.getSizeInBytes());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", gs->size(), sgs.size());
			const sserialize::Static::spatial::GeoPolygon * sgpoly = sgs.get<sserialize::Static::spatial::GeoPolygon>();
			CPPUNIT_ASSERT_MESSAGE("static geopolygon cast", sgpoly);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("boundary", gs->boundary(), sgs.boundary());
			for(uint32_t j = 0, s = gpoly->points().size(); j < s; ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("GeoPoint of Polygon ",i," at ", j), gpoly->points().at(j), sgpoly->points().at(j));
			}
		}
	}
	
	void testMultiPolygon() {
		sserialize::SamplePolygonTestData testData;
		sserialize::createHandSamplePolygons(testData);
		for(uint32_t i = 0, s = testData.polys.size(); i < s; ++i) {
			sserialize::spatial::GeoMultiPolygon * gmpo = new sserialize::spatial::GeoMultiPolygon();
			for(uint32_t j = 0; j < i; ++j) {
				gmpo->outerPolygons().push_back(testData.polys[i].first);
			}
			for(uint32_t j = i; j < s; ++j) {
				gmpo->innerPolygons().push_back(testData.polys[i].first);
			}
			gmpo->recalculateBoundary();
			std::vector<uint8_t> ds;
			sserialize::UByteArrayAdapter d(&ds, false);
			sserialize::spatial::GeoShape::appendWithTypeInfo(gmpo, d);
			d.resetPtrs();

			sserialize::Static::spatial::GeoShape sgs(d);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("type", sgs.type(), sserialize::spatial::GeoShapeType::GS_MULTI_POLYGON);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", (sserialize::OffsetType)d.size(), sgs.getSizeInBytes());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", gmpo->size(), sgs.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("boundary", gmpo->boundary(), sgs.boundary());

			const sserialize::Static::spatial::GeoMultiPolygon * sgmpo = sgs.get<sserialize::Static::spatial::GeoMultiPolygon>();
			CPPUNIT_ASSERT_MESSAGE("static GeoMultiPolygon cast", sgmpo);
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("outerPolygons.size()", (uint32_t)gmpo->outerPolygons().size(), sgmpo->outerPolygons().size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("innerPolygons.size()", (uint32_t)gmpo->innerPolygons().size(), sgmpo->innerPolygons().size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("outerBoundary", gmpo->outerPolygonsBoundary(), sgmpo->outerPolygonsBoundary());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("innerBoundary", gmpo->innerPolygonsBoundary(), sgmpo->innerPolygonsBoundary());
			
			for(uint32_t j = 0, s = gmpo->outerPolygons().size(); j < s; ++j) {
				const sserialize::spatial::GeoPolygon & gpoly = gmpo->outerPolygons().at(j);
				sserialize::Static::spatial::GeoPolygon sgpoly = sgmpo->outerPolygons().at(j);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("size of outerPolygon ", j, " in run=", i), gpoly.size(), sgpoly.size());
				for(uint32_t k = 0, s = gpoly.size(); k < s; ++k) {
					CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("GeoPoint at ", k," of run=", i, ", outerPolygons=", j), gpoly.points().at(k), sgpoly.points().at(k));
				}
			}
			for(uint32_t j = 0, s = gmpo->innerPolygons().size(); j < s; ++j) {
				const sserialize::spatial::GeoPolygon & gpoly = gmpo->innerPolygons().at(j);
				sserialize::Static::spatial::GeoPolygon sgpoly = sgmpo->innerPolygons().at(j);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("size of innerPolygon ", j, " in run=", i), gpoly.size(), sgpoly.size());
				for(uint32_t k = 0, s = gpoly.size(); k < s; ++k) {
					CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("GeoPoint at ", k," of run=", i, ", outerPolygons=", j), gpoly.points().at(k), sgpoly.points().at(k));
				}
			}
		}
	}
};


int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	for(uint32_t i = 0; i < 10; ++i) {
		runner.addTest( StaticGeoWayTest::suite() );
	}
	bool ok = runner.run();
	return (ok ? 0 : 1);
}