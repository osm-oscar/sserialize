#ifndef SSERIALIZE_SPATIAL_POLYGON_H
#define SSERIALIZE_SPATIAL_POLYGON_H
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/types.h>
#include <vector>
#include <cmath>
#include "GeoWay.h"

namespace sserialize {
namespace spatial {

namespace detail {

///GeoPolygon is just GeoWay where the last node equals the first

template<typename TPointsContainer, typename TPointsConstRef>
class GeoPolygon: public sserialize::spatial::detail::GeoWay<TPointsContainer> {
public:
	typedef GeoPoint Point;
	typedef sserialize::spatial::detail::GeoWay<TPointsContainer> MyBaseClass;
	typedef MyBaseClass MyGeoWay;
protected:
	bool collidesWithPolygon(const GeoPolygon & poly) const;
	bool collidesWithWay(const MyGeoWay & way) const;
public:
	GeoPolygon();
	GeoPolygon(const std::vector<Point> & points);
	GeoPolygon(const GeoPolygon & other);
	GeoPolygon(std::vector<Point> && points);
	GeoPolygon(GeoPolygon && other);
	virtual ~GeoPolygon();
	GeoPolygon & operator=(GeoPolygon && other);
	GeoPolygon & operator=(const GeoPolygon & other);
	void swap(GeoPolygon & other);
	virtual GeoShapeType type() const;
	virtual bool contains(const GeoPoint & p) const;
	virtual bool intersects(const sserialize::spatial::GeoRect & rect) const;
	///@return true if the line p1->p2 intersects this region
	virtual bool intersects(const GeoPoint & p1, const GeoPoint & p2) const;
	virtual bool intersects(const GeoRegion & other) const;
	bool encloses(const GeoPolygon & other) const;
	bool encloses(const MyGeoWay & other) const;
	template<typename T_GEO_POINT_ITERATOR>
	bool contains(T_GEO_POINT_ITERATOR begin, T_GEO_POINT_ITERATOR end) const;

	virtual UByteArrayAdapter & append(UByteArrayAdapter & destination) const;
	
	virtual sserialize::spatial::GeoShape * copy() const;
	
	static GeoPolygon fromRect(const GeoRect & rect);
};

template<typename TPointsContainer, typename TPointsConstRef>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::collidesWithPolygon(const GeoPolygon & poly) const {
	if (! MyBaseClass::myBoundary().overlap(poly.myBoundary()))
		return false;

	if (contains(poly.points().begin(), poly.points().end())) { //check if at least one vertex poly lies within us
		return true;
	}
	else if (poly.contains(MyBaseClass::points().begin(), MyBaseClass::points().end())) { //check if at least one own vertex lies within poly
		return true;
	}
	else { //check if any lines intersect
		for(std::size_t i=0, s = poly.points().size(); i < s; i++) {
			if (intersects(poly.points()[i], poly.points()[(i+1)%s]))
				return true;
		}
	}
	return false;
}

template<typename TPointsContainer, typename TPointsConstRef>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::collidesWithWay(const GeoPolygon<TPointsContainer, TPointsConstRef>::MyGeoWay & way) const {
	if (!MyBaseClass::myBoundary().overlap(way.boundary()))
		return false;

	if (contains(way.points().begin(), way.points().end())) { //check if at least one vertex poly lies within us
		return true;
	}
	else if (way.size() > 1) { //check if any lines intersect
		for(typename MyGeoWay::ConstPointsIterator it(way.cbegin()), jt(way.cbegin()+1), end(way.cend()); jt != end; ++it, ++jt) {
			for(std::size_t i = 0, s = MyBaseClass::points().size(); i < s; ++i) {
				if (sserialize::spatial::GeoPoint::intersect(*it, *jt, MyBaseClass::points()[i], MyBaseClass::points()[(i+1)%s])) {
					return true;
				}
			}
		}
	}
	return false;
}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef>::GeoPolygon() :
MyBaseClass()
{}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef>::GeoPolygon(const std::vector<Point> & points) :
MyBaseClass(points)
{}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef>::GeoPolygon(std::vector<Point> && points) :
MyBaseClass(points)
{}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef>::GeoPolygon(GeoPolygon && other) :
MyBaseClass(other)
{}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef>::GeoPolygon(const GeoPolygon & other) :
MyBaseClass(other)
{}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef>::~GeoPolygon()
{}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef> & GeoPolygon<TPointsContainer, TPointsConstRef>::operator=(GeoPolygon && other) {
	swap(other);
	return *this;
}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef> & GeoPolygon<TPointsContainer, TPointsConstRef>::operator=(const GeoPolygon<TPointsContainer, TPointsConstRef> & other) {
	MyGeoWay::operator=(other);
	return *this;
}

template<typename TPointsContainer, typename TPointsConstRef>
void GeoPolygon<TPointsContainer, TPointsConstRef>::swap(GeoPolygon & other) {
	MyGeoWay::swap(other);
}

template<typename TPointsContainer, typename TPointsConstRef>
GeoShapeType GeoPolygon<TPointsContainer, TPointsConstRef>::type() const {
	return GS_POLYGON;
}

//http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
template<typename TPointsContainer, typename TPointsConstRef>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::contains(const GeoPoint & p) const {
	if (!MyBaseClass::points().size() || !MyBaseClass::myBoundary().contains(p.lat(), p.lon()))
		return false;
	int nvert = MyBaseClass::points().size();
	double testx = p.lat();
	double testy = p.lon();
	int i, j, c = 0;
	for (i = 0, j = nvert-1; i < nvert; j = i++) {
		TPointsConstRef iP = MyBaseClass::points()[i];
		TPointsConstRef jP = MyBaseClass::points()[j];
		double vertx_i = iP.lat();
		double verty_i = iP.lon();
		double vertx_j = jP.lat();
		double verty_j = jP.lon();
		
		if ( ((verty_i>testy) != (verty_j>testy)) &&
			(testx < (vertx_j-vertx_i) * (testy-verty_i) / (verty_j-verty_i) + vertx_i) ) {
			c = !c;
		}
	}
	return c;
}

template<typename TPointsContainer, typename TPointsConstRef>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::intersects(const sserialize::spatial::GeoRect & rect) const {
	if (!MyBaseClass::myBoundary().overlap(rect))
		return false;
		
	return collidesWithPolygon( fromRect(rect) );
}

template<typename TPointsContainer, typename TPointsConstRef>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::intersects(const GeoPoint & p1, const GeoPoint & p2) const {
	if (!MyBaseClass::myBoundary().overlap( sserialize::spatial::GeoRect(p1.lat(), p2.lat(), p1.lon(), p2.lon()) ) ) {
		return false;
	}

	if (contains(p1) || contains(p2)) {
		return true;
	}

	for(std::size_t i = 0, s = MyBaseClass::points().size(); i < s; i++) {
		if (sserialize::spatial::GeoPoint::intersect(p1, p2, MyBaseClass::points()[i], MyBaseClass::points()[(i+1)%s])) {
			return true;
		}
	}
	return false;
}

template<typename TPointsContainer, typename TPointsConstRef>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::intersects(const GeoRegion & other) const {
	if (other.type() == sserialize::spatial::GS_POLYGON) {
		return collidesWithPolygon( *static_cast<const GeoPolygon *>(&other) );
	}
	else if (other.type() == sserialize::spatial::GS_WAY) {
		return collidesWithWay( *static_cast<const MyGeoWay*>(&other) );
	}
	else {
		return other.intersects(*this);
	}
}

template<typename TPointsContainer, typename TPointsConstRef>
UByteArrayAdapter & GeoPolygon<TPointsContainer, TPointsConstRef>::append(UByteArrayAdapter & destination) const {
	return MyBaseClass::append(destination);
}

template<typename TPointsContainer, typename TPointsConstRef>
sserialize::spatial::GeoShape * GeoPolygon<TPointsContainer, TPointsConstRef>::copy() const {
	return new GeoPolygon(*this);
}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef> GeoPolygon<TPointsContainer, TPointsConstRef>::fromRect(const GeoRect & rect) {
	std::vector<GeoPoint> points;
	points.push_back( Point(rect.lat()[0], rect.lon()[0]) );
	points.push_back( Point(rect.lat()[1], rect.lon()[0]) );
	points.push_back( Point(rect.lat()[1], rect.lon()[1]) );
	points.push_back( Point(rect.lat()[0], rect.lon()[1]) );
	return GeoPolygon(points);
}

template<typename TPointsContainer, typename TPointsConstRef>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::encloses(const GeoPolygon<TPointsContainer, TPointsConstRef> & other) const {
	return encloses(*static_cast<const MyGeoWay*>(&other) );
}

template<typename TPointsContainer, typename TPointsConstRef>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::encloses(const GeoPolygon<TPointsContainer, TPointsConstRef>::MyGeoWay & other) const {
	if (!other.boundary().overlap(this->myBoundary())) {
		return false;
	}
	//if all points are within my self and nothing intersects, then the poly is fully contained
	for (std::size_t i = 0, s = other.points().size(); i < s; ++i) {
		if (!contains(other.points()[i])) {
			return false;
		}
	}
	//all points are within, check for cut. TODO: improve this by haveing more info about the polygon, if this would be convex, then we would be done here
	
	for(typename MyGeoWay::PointsContainer::const_iterator prev(other.points().begin()), it(other.points().begin()+1), end(other.points().end()); it != end; ++it) {
		if (intersects(*prev, *it))
			return false;
	}
	return true;
}


template<typename TPointsContainer, typename TPointsConstRef>
template<typename T_GEO_POINT_ITERATOR>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::contains(T_GEO_POINT_ITERATOR begin, T_GEO_POINT_ITERATOR end) const {
	for(; begin != end; ++begin) {
		if (contains(*begin))
			return true;
	}
	return false;
}

}

typedef detail::GeoPolygon< std::vector<sserialize::spatial::GeoPoint>, const sserialize::spatial::GeoPoint & > GeoPolygon;

}}//end namespace

namespace std {
template<>
inline void swap<sserialize::spatial::GeoPolygon>(sserialize::spatial::GeoPolygon & a, sserialize::spatial::GeoPolygon & b) { a.swap(b);}
}

///serializes without type info
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPolygon & p);


#endif
