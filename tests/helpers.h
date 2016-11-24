#ifndef SSERIALIZE_TESTS_HELPERS_H
#define SSERIALIZE_TESTS_HELPERS_H
#include <sserialize/spatial/GeoPoint.h>
#include <cppunit/TestAssert.h>

CPPUNIT_NS_BEGIN

template<>
struct assertion_traits<sserialize::spatial::GeoPoint> {

	static double eps;

	static bool equal( const sserialize::spatial::GeoPoint & x, const sserialize::spatial::GeoPoint & y ) {
		return sserialize::spatial::equal(x, y, eps);
	}

	static std::string toString( const sserialize::spatial::GeoPoint & x ) {
		std::stringstream ost;
		ost << x;
		return ost.str();
    }
};

CPPUNIT_NS_END

#define SSERIALIZE_TESTS_EPS CPPUNIT_NS::assertion_traits<sserialize::spatial::GeoPoint>::eps


#endif