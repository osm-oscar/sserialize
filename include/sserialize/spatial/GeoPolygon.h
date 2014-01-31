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
	typedef typename MyBaseClass::const_iterator const_iterator;
protected:
	bool collidesWithPolygon(const GeoPolygon & poly) const;
	bool collidesWithWay(const MyGeoWay & way) const;
public:
	GeoPolygon();
	GeoPolygon(const sserialize::spatial::GeoRect & boundary, const TPointsContainer & points);
	GeoPolygon(const TPointsContainer & points);
	GeoPolygon(const GeoPolygon & other);
	GeoPolygon(TPointsContainer && points);
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

	if (contains(poly.points().cbegin(), poly.points().cend())) { //check if at least one vertex poly lies within us
		return true;
	}
	else if (poly.contains(MyBaseClass::points().cbegin(), MyBaseClass::points().cend())) { //check if at least one own vertex lies within poly
		return true;
	}
	else { //check if any lines intersect
		const_iterator it(MyBaseClass::cbegin());
		++it;
		for(const_iterator prev(MyBaseClass::cbegin()), end(MyBaseClass::cend()); it != end; ++it, ++prev) {
			if (intersects(*prev, *it))
				return true;
		}
	}
	return false;
}

template<typename TPointsContainer, typename TPointsConstRef>
bool GeoPolygon<TPointsContainer, TPointsConstRef>::collidesWithWay(const GeoPolygon<TPointsContainer, TPointsConstRef>::MyGeoWay & way) const {
	if (!MyBaseClass::myBoundary().overlap(way.boundary()))
		return false;

	if (contains(way.points().cbegin(), way.points().cend())) { //check if at least one vertex poly lies within us
		return true;
	}
	else if (way.size() > 1) { //check if any lines intersect
		for(typename MyGeoWay::const_iterator prev(MyBaseClass::cbegin()), it(MyBaseClass::cbegin()+1), end(MyBaseClass::cend()); it != end; ++prev, ++it) {
			for(typename MyGeoWay::const_iterator oPrev(way.cbegin()), oIt(way.cbegin()+1), oEnd(way.cend()); oIt != oEnd; ++oPrev, ++oIt) {
				if (sserialize::spatial::GeoPoint::intersect(*prev, *it, *oPrev, *oIt)) {
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
GeoPolygon<TPointsContainer, TPointsConstRef>::GeoPolygon(const sserialize::spatial::GeoRect & boundary, const TPointsContainer & points) :
MyBaseClass(boundary, points)
{}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef>::GeoPolygon(const TPointsContainer & points) :
MyBaseClass(points)
{}

template<typename TPointsContainer, typename TPointsConstRef>
GeoPolygon<TPointsContainer, TPointsConstRef>::GeoPolygon(TPointsContainer && points) :
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
GeoPolygon<TPointsContainer, TPointsConstRef> & GeoPolygon<TPointsContainer, TPointsConstRef>::operator=(GeoPolygon<TPointsContainer, TPointsConstRef> && other) {
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
	double testx = p.lat();
	double testy = p.lon();
	int c = 0;
// 	int nvert = MyBaseClass::points().size();
// 	int i, j = 0;
// 	for (i = 0, j = nvert-1; i < nvert; j = i++) {
// 		TPointsConstRef iP = MyBaseClass::points()[i];
// 		TPointsConstRef jP = MyBaseClass::points()[j];

	for(const_iterator prev(MyBaseClass::cbegin()+1), it(MyBaseClass::cbegin()), end(MyBaseClass::cend()); it != end; ++it, ++prev) {
		TPointsConstRef iP = *it;
		TPointsConstRef jP = *prev;
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

	for(const_iterator prev(MyBaseClass::cbegin()+1), it(MyBaseClass::cbegin()), end(MyBaseClass::cend()); it != end; ++it, ++prev)  {
		if (sserialize::spatial::GeoPoint::intersect(p1, p2, *prev, *it)) {
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
	throw sserialize::UnimplementedFunctionException("sserialize::spatial::GeoPolygon::fromRect");
	return GeoPolygon();
// 	std::vector<GeoPoint> points;
// 	points.push_back( Point(rect.lat()[0], rect.lon()[0]) );
// 	points.push_back( Point(rect.lat()[1], rect.lon()[0]) );
// 	points.push_back( Point(rect.lat()[1], rect.lon()[1]) );
// 	points.push_back( Point(rect.lat()[0], rect.lon()[1]) );
// 	return GeoPolygon(points);
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
	for(const_iterator oIt(other.cbegin()), oEnd(other.cend()); oIt != oEnd; ++oIt)  {
		if (!contains(*oIt)) {
			return false;
		}
	}
	//all points are within, check for cut. TODO: improve this by haveing more info about the polygon, if this would be convex, then we would be done here
	
	for(const_iterator oPrev(other.cbegin()), oIt(other.cbegin()+1), oEnd(other.cend()); oIt != oEnd; ++oIt, ++oPrev) {
		if (intersects(*oPrev, *oIt))
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
