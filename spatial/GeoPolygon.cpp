#include <sserialize/spatial/GeoPolygon.h>

namespace sserialize {
namespace spatial {
namespace detail {

template<>
GeoPolygon<std::vector<sserialize::spatial::GeoPoint> > GeoPolygon<std::vector<sserialize::spatial::GeoPoint> >::fromRect(const GeoRect & rect) {
	std::vector<GeoPoint> points;
	points.push_back( GeoPoint(rect.lat()[0], rect.lon()[0]) );
	points.push_back( GeoPoint(rect.lat()[1], rect.lon()[0]) );
	points.push_back( GeoPoint(rect.lat()[1], rect.lon()[1]) );
	points.push_back( GeoPoint(rect.lat()[0], rect.lon()[1]) );
	return GeoPolygon(points);
}

}}}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPolygon & p) {
	return p.append(destination);
}