#include <sserialize/Static/GeoPolygon.h>

namespace sserialize {
namespace spatial {
namespace detail {

template<>
GeoPolygon< sserialize::Static::spatial::detail::GeoWayPointsContainer>::GeoPolygon(const sserialize::UByteArrayAdapter & d) :
MyBaseClass(d)
{}

template<>
GeoPolygon<sserialize::Static::spatial::detail::GeoWayPointsContainer> GeoPolygon<sserialize::Static::spatial::detail::GeoWayPointsContainer>::fromRect(const GeoRect & rect) {
	std::vector<GeoPoint> points;
	points.push_back( GeoPoint(rect.lat()[0], rect.lon()[0]) );
	points.push_back( GeoPoint(rect.lat()[1], rect.lon()[0]) );
	points.push_back( GeoPoint(rect.lat()[1], rect.lon()[1]) );
	points.push_back( GeoPoint(rect.lat()[0], rect.lon()[1]) );
	sserialize::UByteArrayAdapter d(new std::vector<uint8_t>(), true);
	sserialize::Static::spatial::detail::GeoWayPointsContainer::append(points.begin(), points.end(), d);
	sserialize::Static::spatial::detail::GeoWayPointsContainer gwc(d);
	return GeoPolygon<sserialize::Static::spatial::detail::GeoWayPointsContainer>(rect, gwc);
}

}}}//end namespaces