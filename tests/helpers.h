#ifndef SSERIALIZE_TESTS_HELPERS_H
#define SSERIALIZE_TESTS_HELPERS_H
#include <sserialize/spatial/GeoPoint.h>
#include <cppunit/TestAssert.h>

namespace CppUnit {

template<>
struct assertion_traits<sserialize::spatial::GeoPoint> {
	static bool equal( const sserialize::spatial::GeoPoint & x, const sserialize::spatial::GeoPoint & y ) {
		return sserialize::spatial::equal(x, y, 0.0);
	}

	static std::string toString( const sserialize::spatial::GeoPoint & x ) {
		std::stringstream ost;
		ost << x;
		return ost.str();
    }
};

}//end namespace sserialize


#endif