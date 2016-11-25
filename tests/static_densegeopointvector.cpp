#include <sserialize/Static/DenseGeoPointVector.h>
#include <sserialize/utility/log.h>
#include "datacreationfuncs.h"
#include "helpers.h"
#include "TestBase.h"

using namespace sserialize;

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
		res.push_back(prev);
	}
	return res;
}

uint32_t testPointCount = 1024;
double maxGpDiff = 1.;

class TestDenseGeoPointVector: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( TestDenseGeoPointVector );
CPPUNIT_TEST( testPoints );
CPPUNIT_TEST( testIterator );
CPPUNIT_TEST( testAbstractArray );
CPPUNIT_TEST( testAbstractArrayIterator );
CPPUNIT_TEST_SUITE_END();
private:
	static sserialize::spatial::GeoPoint tr(const sserialize::spatial::GeoPoint & p) {
		uint32_t intLat = sserialize::spatial::GeoPoint::toIntLat(p.lat());
		uint32_t intLon = sserialize::spatial::GeoPoint::toIntLon(p.lon());
		return sserialize::spatial::GeoPoint::fromIntLatLon(intLat, intLon);
	}
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testPoints() {
		std::vector<sserialize::spatial::GeoPoint> testPoints = createPoints(testPointCount, maxGpDiff);
		std::vector<uint8_t> d;
		sserialize::UByteArrayAdapter dA(&d, false);
		sserialize::Static::spatial::DenseGeoPointVector::append(testPoints.begin(), testPoints.end(), dA);
		
		sserialize::Static::spatial::DenseGeoPointVector dgpv(dA);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)testPoints.size(), dgpv.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", (sserialize::UByteArrayAdapter::OffsetType)d.size(), dgpv.getSizeInBytes());
		
		for(uint32_t i = 0, s = (uint32_t) testPoints.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("point at ", i), tr(testPoints.at(i)), dgpv.at(i));
		}
		
	}
	
	void testIterator() {
		std::vector<sserialize::spatial::GeoPoint> testPoints = createPoints(testPointCount, maxGpDiff);
		std::vector<uint8_t> d;
		sserialize::UByteArrayAdapter dA(&d, false);
		sserialize::Static::spatial::DenseGeoPointVector::append(testPoints.begin(), testPoints.end(), dA);
		
		sserialize::Static::spatial::DenseGeoPointVector dgpv(dA);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)testPoints.size(), dgpv.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", (sserialize::UByteArrayAdapter::OffsetType)d.size(), dgpv.getSizeInBytes());
		
		uint32_t pos = 0;
		for(sserialize::Static::spatial::DenseGeoPointVector::const_iterator it(dgpv.cbegin()), end(dgpv.cend()); it != end; ++it) {
		
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("iterator at ", pos), testPoints.size() > pos);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("point at ", pos), tr(testPoints.at(pos)), *it); 
			++pos;
		}
	}
	
	void testAbstractArray() {
		std::vector<sserialize::spatial::GeoPoint> testPoints = createPoints(testPointCount, maxGpDiff);
		std::vector<uint8_t> d;
		sserialize::UByteArrayAdapter dA(&d, false);
		sserialize::Static::spatial::DenseGeoPointVector::append(testPoints.begin(), testPoints.end(), dA);
		
		sserialize::Static::spatial::DenseGeoPointVector dgpv(dA);
		
		sserialize::AbstractArray<sserialize::spatial::GeoPoint> adgpv( new sserialize::Static::spatial::detail::DenseGeoPointVectorAbstractArray(dgpv) );
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)testPoints.size(), (uint32_t)adgpv.size());
		
		for(uint32_t i = 0, s = (uint32_t) testPoints.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("point at ", i), tr(testPoints.at(i)), adgpv.at(i));
		}
		
	}
	
	void testAbstractArrayIterator() {
		std::vector<sserialize::spatial::GeoPoint> testPoints = createPoints(testPointCount, maxGpDiff);
		std::vector<uint8_t> d;
		sserialize::UByteArrayAdapter dA(&d, false);
		sserialize::Static::spatial::DenseGeoPointVector::append(testPoints.begin(), testPoints.end(), dA);
		
		sserialize::Static::spatial::DenseGeoPointVector dgpv(dA);
		
		sserialize::Static::spatial::DenseGeoPointAbstractArray  adgpv( new sserialize::Static::spatial::detail::DenseGeoPointVectorAbstractArray(dgpv) );
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)testPoints.size(), (uint32_t) adgpv.size());
		
		uint32_t pos = 0;
		for(sserialize::Static::spatial::DenseGeoPointAbstractArray::const_iterator it(adgpv.cbegin()), end(adgpv.cend()); it != end; ++it) {
			sserialize::spatial::GeoPoint gp = *it;
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("iterator at ", pos), testPoints.size() > pos);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("point at ", pos), tr(testPoints.at(pos)), gp);
			++pos;
		}
	}


};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	for(uint32_t i = 0; i < 10; i++) {
		runner.addTest(  TestDenseGeoPointVector::suite() );
	}
	bool ok = runner.run();
	return ok ? 0 : 1;
}