#include <sserialize/Static/GeoPolygon.h>

namespace sserialize {
namespace spatial {
namespace detail {

template<>
GeoPolygon< sserialize::Static::spatial::DenseGeoPointVector >::GeoPolygon(const sserialize::UByteArrayAdapter & d) :
MyBaseClass(d)
{}

template<>
GeoPolygon<sserialize::Static::spatial::DenseGeoPointVector> GeoPolygon<sserialize::Static::spatial::DenseGeoPointVector>::fromRect(const GeoRect & rect) {
	std::vector<GeoPoint> points;
	points.reserve(5);
	points.push_back( GeoPoint(rect.lat()[0], rect.lon()[0]) );
	points.push_back( GeoPoint(rect.lat()[1], rect.lon()[0]) );
	points.push_back( GeoPoint(rect.lat()[1], rect.lon()[1]) );
	points.push_back( GeoPoint(rect.lat()[0], rect.lon()[1]) );
	points.push_back( points.front() );
	sserialize::UByteArrayAdapter d(new std::vector<uint8_t>(), true);
	sserialize::Static::spatial::DenseGeoPointVector::append(points.begin(), points.end(), d);
	sserialize::Static::spatial::DenseGeoPointVector gwc(d);
	return GeoPolygon<sserialize::Static::spatial::DenseGeoPointVector>(rect, gwc);
}

template<>
GeoPolygon< sserialize::AbstractArray<sserialize::spatial::GeoPoint> >::GeoPolygon(const sserialize::UByteArrayAdapter & d) :
MyBaseClass(d)
{}


}}}//end namespaces