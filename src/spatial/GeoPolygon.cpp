#include <sserialize/spatial/GeoPolygon.h>

namespace sserialize {
namespace spatial {
namespace detail {

template<>
GeoPolygon< std::vector<sserialize::spatial::GeoPoint> >::GeoPolygon(const std::vector<sserialize::spatial::GeoPoint> & points) :
MyBaseClass()
{
	if (points.size() < 3) {
		throw sserialize::CorruptDataException("GeoPolygon");
	}
	this->points() = points;
	if (!spatial::equal(points.front(), points.back(), 0.0)) {
		this->points().push_back(points.front());
	}
	recalculateBoundary();
}

template<>
GeoPolygon< std::vector<sserialize::spatial::GeoPoint> >::GeoPolygon(std::vector<sserialize::spatial::GeoPoint> && points) :
MyBaseClass()
{
	if (points.size() < 3) {
		throw sserialize::CorruptDataException("GeoPolygon");
	}
	if (!spatial::equal(points.front(), points.back(), 0.0)) {
		points.push_back(points.front());
	}
	this->points() = std::move(points);
	recalculateBoundary();
}

template<>
int
GeoPolygon<std::vector<sserialize::spatial::GeoPoint> >::orientation() const {
// Idea based on http://cs.smith.edu/~orourke/Code/polyorient.C
	if (size() < 4) {
		return 0;
	}
	//lat is y, lon is x
	
	const_iterator it = cbegin();
	const_iterator end = cend()-1;
	const_iterator prev = it;
	const_iterator lowest = it;
	sserialize::spatial::GeoPoint lp = *lowest;
	for(; it != end; ++it) {
		const sserialize::spatial::GeoPoint & gp = *it;
		if ((gp.lat() < lp.lat()) || (gp.lat() == lp.lat() && gp.lon() > lp.lon())) {
			lp = gp;
			lowest = it;
		}
	}
	
	//prev is at the end
	if (prev == cbegin()) {
		prev = cend()-2;
	}
	const_iterator next = it+1;
// 
// 	a[lon/lat] = prev, b[lon/lat] = it, c[lon/lat] = next with lon=0, lat=1
//                a[0] * b[1] - a[1] * b[0] +
//                 a[1] * c[0] - a[0] * c[1] +
//                 b[0] * c[1] - c[0] * b[1];
	double area = prev->lon() * it->lat() - prev->lat() * it->lon() +
			prev->lat() * next->lon() - prev->lon() * next->lat() +
			it->lon() * next->lat() - next->lon() * prev->lat();
	return (area > 0 ? 1 : (area < 0 ? -1 : 0));
}

template<>
void GeoPolygon< std::vector<sserialize::spatial::GeoPoint> >::orient(int ot) {
	if (orientation() == -ot) {
		using std::reverse;
		reverse(points().begin(), points().end());
	}
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