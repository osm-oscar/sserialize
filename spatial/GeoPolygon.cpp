#include <sserialize/spatial/GeoPolygon.h>

namespace sserialize {
namespace spatial {
namespace detail {

template<>
GeoPolygon< std::vector<sserialize::spatial::GeoPoint> >::GeoPolygon(const std::vector<sserialize::spatial::GeoPoint> & points) :
MyBaseClass()
{
	this->points() = points;
	if (points.front() != points.back()) {
		this->points().push_back(points.front());
	}
	recalculateBoundary();
}

template<>
GeoPolygon<std::vector<sserialize::spatial::GeoPoint> > GeoPolygon<std::vector<sserialize::spatial::GeoPoint> >::fromRect(const GeoRect & rect) {
	std::vector<GeoPoint> points;
	points.reserve(5);
	points.push_back( GeoPoint(rect.minLat(), rect.minLon()) );
	points.push_back( GeoPoint(rect.minLat(), rect.maxLon()) );
	points.push_back( GeoPoint(rect.maxLat(), rect.maxLon()) );
	points.push_back( GeoPoint(rect.maxLat(), rect.minLon()) );
	points.push_back( points.front() );
	return GeoPolygon(rect, points);
}

template<>
GeoPolygon<sserialize::AbstractArray<sserialize::spatial::GeoPoint> > GeoPolygon<sserialize::AbstractArray<sserialize::spatial::GeoPoint> >::fromRect(const GeoRect & rect) {
	std::vector<GeoPoint> points;
	points.reserve(5);
	points.push_back( GeoPoint(rect.minLat(), rect.minLon()) );
	points.push_back( GeoPoint(rect.minLat(), rect.maxLon()) );
	points.push_back( GeoPoint(rect.maxLat(), rect.maxLon()) );
	points.push_back( GeoPoint(rect.maxLat(), rect.minLon()) );
	points.push_back( points.front() );
	
	typedef sserialize::detail::AbstractArrayDefaultImp<std::vector<sserialize::spatial::GeoPoint>, sserialize::spatial::GeoPoint > AAPType;
	
	AbstractArray<sserialize::spatial::GeoPoint> d( new AAPType(points) );
	return GeoPolygon< sserialize::AbstractArray<sserialize::spatial::GeoPoint> >(d);
}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const GeoPolygon< std::vector<sserialize::spatial::GeoPoint> > & p) {
	return p.append(destination);
}

}}}