#ifndef SSERIALIZE_SPATIAL_POLYGON_H
#define SSERIALIZE_SPATIAL_POLYGON_H
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/types.h>
#include <sserialize/spatial/GeoWay.h>
#include <sserialize/templated/AbstractArray.h>
#include <vector>
#include <cmath>

namespace sserialize {
namespace spatial {

namespace detail {

///GeoPolygon is just a closed GeoWay where the last node equals the first

template<typename TPointsContainer>
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
	GeoPolygon(const sserialize::UByteArrayAdapter & d);
	GeoPolygon(const sserialize::spatial::GeoRect & boundary, const TPointsContainer & points);
	GeoPolygon(const TPointsContainer & points);
	GeoPolygon(const GeoPolygon & other);
	GeoPolygon(TPointsContainer && points);
	GeoPolygon(GeoPolygon && other);
	virtual ~GeoPolygon();
	GeoPolygon & operator=(GeoPolygon && other);
	GeoPolygon & operator=(const GeoPolygon & other);
	void swap(GeoPolygon & other);
	using MyBaseClass::cbegin;
	using MyBaseClass::cend;
	using MyBaseClass::begin;
	using MyBaseClass::end;
	using MyBaseClass::points;
	
	virtual GeoShapeType type() const;
	virtual bool contains(const GeoPoint & p) const;
	virtual bool intersects(const sserialize::spatial::GeoRect & rect) const;
	///@return true if the line p1->p2 intersects this region
	virtual bool intersects(const GeoPoint & p1, const GeoPoint & p2) const;
	virtual bool intersects(const GeoRegion & other) const;
	virtual double distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const;
	bool encloses(const GeoPolygon & other) const;
	bool encloses(const MyGeoWay & other) const;
	template<typename T_GEO_POINT_ITERATOR>
	bool contains(T_GEO_POINT_ITERATOR begin, T_GEO_POINT_ITERATOR end) const;

	virtual UByteArrayAdapter & append(UByteArrayAdapter & destination) const;
	
	virtual sserialize::spatial::GeoShape * copy() const;
	virtual std::ostream & asString(std::ostream & out) const;
	
	static GeoPolygon fromRect(const GeoRect & rect);
};

template<typename TPointsContainer>
bool GeoPolygon<TPointsContainer>::collidesWithPolygon(const GeoPolygon & poly) const {
	if (! MyBaseClass::myBoundary().overlap(poly.myBoundary()))
		return false;

	if (contains(poly.points().cbegin(), poly.points().cend())) { //check if at least one vertex poly lies within us
		return true;
	}
	else if (poly.contains(MyBaseClass::points().cbegin(), MyBaseClass::points().cend())) { //check if at least one own vertex lies within poly
		return true;
	}
	else { //check if any lines intersect
		const_iterator oIt(poly.cbegin());
		++oIt;
		for(const_iterator oPrev(poly.cbegin()), oEnd(poly.cend()); oIt != oEnd; ++oIt, ++oPrev) {
			const_iterator it(cbegin());
			++it;
			for(const_iterator prev(cbegin()), end(cend()); it != end; ++prev, ++it) {
				if (sserialize::spatial::GeoPoint::intersect(*prev, *it, *oPrev, *oIt)) {
					return true;
				}
			}
		}
	}
	return false;
}

template<typename TPointsContainer>
bool GeoPolygon<TPointsContainer>::collidesWithWay(const GeoPolygon<TPointsContainer>::MyGeoWay & way) const {
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

template<typename TPointsContainer>
GeoPolygon<TPointsContainer>::GeoPolygon() :
MyBaseClass()
{}

template<typename TPointsContainer>
GeoPolygon<TPointsContainer>::GeoPolygon(const sserialize::spatial::GeoRect & boundary, const TPointsContainer & points) :
MyBaseClass(boundary, points)
{}

template<typename TPointsContainer>
GeoPolygon<TPointsContainer>::GeoPolygon(const TPointsContainer & points) :
MyBaseClass(points)
{
	if (points.front() != points.back()) {
		throw sserialize::CorruptDataException("GeoPolygon");
	}
}

template<typename TPointsContainer>
GeoPolygon<TPointsContainer>::GeoPolygon(TPointsContainer && points) :
MyBaseClass(points)
{}

template<typename TPointsContainer>
GeoPolygon<TPointsContainer>::GeoPolygon(GeoPolygon && other) :
MyBaseClass(other)
{}

template<typename TPointsContainer>
GeoPolygon<TPointsContainer>::GeoPolygon(const GeoPolygon & other) :
MyBaseClass(other)
{}

template<typename TPointsContainer>
GeoPolygon<TPointsContainer>::~GeoPolygon()
{}

template<typename TPointsContainer>
GeoPolygon<TPointsContainer> & GeoPolygon<TPointsContainer>::operator=(GeoPolygon<TPointsContainer> && other) {
	swap(other);
	return *this;
}

template<typename TPointsContainer>
GeoPolygon<TPointsContainer> & GeoPolygon<TPointsContainer>::operator=(const GeoPolygon<TPointsContainer> & other) {
	MyGeoWay::operator=(other);
	return *this;
}

template<typename TPointsContainer>
void GeoPolygon<TPointsContainer>::swap(GeoPolygon & other) {
	MyGeoWay::swap(other);
}

template<typename TPointsContainer>
GeoShapeType GeoPolygon<TPointsContainer>::type() const {
	return GS_POLYGON;
}

//http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
template<typename TPointsContainer>
bool GeoPolygon<TPointsContainer>::contains(const GeoPoint & p) const {
	if (!MyBaseClass::points().size() || !MyBaseClass::myBoundary().contains(p.lat(), p.lon()))
		return false;
	double testx = p.lat();
	double testy = p.lon();
	int c = 0;
// 	int nvert = MyBaseClass::points().size();
// 	for (int i = 0, j = nvert-1; i < nvert; j = i++) {
// 		typename TPointsContainer::const_reference iP = MyBaseClass::points().at(i);
// 		typename TPointsContainer::const_reference jP = MyBaseClass::points().at(j);

	for(const_iterator prev(MyBaseClass::cbegin()), it(MyBaseClass::cbegin()+1), end(MyBaseClass::cend()); it != end; ++it, ++prev) {
		typename TPointsContainer::const_reference iP = *it;
		typename TPointsContainer::const_reference jP = *prev;
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

template<typename TPointsContainer>
bool GeoPolygon<TPointsContainer>::intersects(const sserialize::spatial::GeoRect & rect) const {
	if (!MyBaseClass::myBoundary().overlap(rect))
		return false;

	//now check if any of the rect points fall within ourself
	std::vector<sserialize::spatial::GeoPoint> poly;
	poly.push_back( GeoPoint(rect.lat()[0], rect.lon()[0]) );
	poly.push_back( GeoPoint(rect.lat()[1], rect.lon()[0]) );
	poly.push_back( GeoPoint(rect.lat()[1], rect.lon()[1]) );
	poly.push_back( GeoPoint(rect.lat()[0], rect.lon()[1]) );
	if (contains(poly.cbegin(), poly.cend())) { //check if at least one vertex poly lies within us
		return true;
	}
	
	//check if any of our own lie within the poly
	const_iterator it(cbegin());
	++it; //skip the first since first == last
	for(const_iterator end(cend()); it != end; ++it) {
		typename TPointsContainer::const_reference gp = *it;
		if (rect.contains(gp.lat(), gp.lon())) {
			return true;
		}
	}

	//check if any lines intersect
	poly.push_back(poly.back());//put the last back in
	it = cbegin();
	++it;
	for(const_iterator prev(cbegin()), end(cend()); it != end; ++prev, ++it) {
		std::vector<sserialize::spatial::GeoPoint>::const_iterator oIt(poly.cbegin());
		++oIt;
		for(std::vector<sserialize::spatial::GeoPoint>::const_iterator oPrev(poly.cbegin()), oEnd(poly.cend()); oIt != oEnd; ++oIt, ++oPrev) {
			if (sserialize::spatial::GeoPoint::intersect(*prev, *it, *oPrev, *oIt)) {
				return true;
			}
		}
	}
	//nothing intersected
	return false;
}

template<typename TPointsContainer>
bool GeoPolygon<TPointsContainer>::intersects(const GeoPoint & p1, const GeoPoint & p2) const {
	if (!MyBaseClass::myBoundary().overlap( sserialize::spatial::GeoRect(p1.lat(), p2.lat(), p1.lon(), p2.lon()) ) ) {
		return false;
	}

	if (contains(p1) || contains(p2)) {
		return true;
	}

	for(const_iterator prev(MyBaseClass::cbegin()), it(MyBaseClass::cbegin()+1), end(MyBaseClass::cend()); it != end; ++it, ++prev)  {
		if (sserialize::spatial::GeoPoint::intersect(p1, p2, *prev, *it)) {
			return true;
		}
	}
	return false;
}

template<typename TPointsContainer>
bool GeoPolygon<TPointsContainer>::intersects(const GeoRegion & other) const {
	if (other.type() == sserialize::spatial::GS_POLYGON) {
		return collidesWithPolygon( *static_cast<const GeoPolygon *>(&other) );
	}
	else if (other.type() == sserialize::spatial::GS_WAY) {
		return collidesWithWay( *static_cast<const MyGeoWay*>(&other) );
	}
	else if (other.type() > type()) {
		return other.intersects(*this);
	}
	return false;
}

template<typename TPointsContainer>
double GeoPolygon<TPointsContainer>::distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const {
	GeoShapeType gt = type();
	if (gt <= GS_POLYGON) {
		if (gt == GS_POINT) {
			const sserialize::spatial::GeoPoint & op = *static_cast<const sserialize::spatial::GeoPoint*>(&other);
			double d = std::numeric_limits<double>::max();
			for(const_iterator it(this->cbegin()), end(this->cend()); it != end; ++it) {
				typename TPointsContainer::const_reference itP = *it;
				d = std::min<double>(d, distanceCalculator.calc(itP.lat(), itP.lon(), op.lat(), op.lon()));
			}
			return d;
		}
		else if (gt == GS_WAY || gt == GS_POLYGON) {
			const MyBaseClass & ow = *static_cast<const MyBaseClass*>(&other);
			double d = std::numeric_limits<double>::max();
			for(const_iterator it(cbegin()), end(cend()); it != end; ++it) {
				typename TPointsContainer::const_reference itP = *it;
				for (const_iterator oIt(ow.cbegin()), oEnd(ow.cend()); oIt != oEnd; ++oIt) {
					typename TPointsContainer::const_reference oItP = *oIt; 
					d = std::min<double>(d, distanceCalculator.calc(itP.lat(), itP.lon(), oItP.lat(), oItP.lon()));
				}
			}
			return d;
		}
	}
	else {
		return other.distance(*this, distanceCalculator);
	}
	return std::numeric_limits<double>::quiet_NaN();
}

template<typename TPointsContainer>
UByteArrayAdapter & GeoPolygon<TPointsContainer>::append(UByteArrayAdapter & destination) const {
	return MyBaseClass::append(destination);
}

template<typename TPointsContainer>
sserialize::spatial::GeoShape * GeoPolygon<TPointsContainer>::copy() const {
	return new GeoPolygon(*this);
}

template<typename TPointsContainer>
std::ostream & GeoPolygon<TPointsContainer>::asString(std::ostream & out) const {
	out << "GeoPolygon[" << MyBaseClass::myBoundary() << "{";
	for(typename TPointsContainer::const_iterator it(cbegin()), end(cend()); it != end; ++it) {
		typename TPointsContainer::const_reference gp = *it;
		out << "(" << gp.lat() << ", " << gp.lon() << ")";
	}
	out << "}]";
	return out;
}

template<typename TPointsContainer>
bool GeoPolygon<TPointsContainer>::encloses(const GeoPolygon<TPointsContainer> & other) const {
	return encloses(*static_cast<const MyGeoWay*>(&other) );
}

template<typename TPointsContainer>
bool GeoPolygon<TPointsContainer>::encloses(const GeoPolygon<TPointsContainer>::MyGeoWay & other) const {
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


template<typename TPointsContainer>
template<typename T_GEO_POINT_ITERATOR>
bool GeoPolygon<TPointsContainer>::contains(T_GEO_POINT_ITERATOR begin, T_GEO_POINT_ITERATOR end) const {
	for(; begin != end; ++begin) {
		if (contains(*begin))
			return true;
	}
	return false;
}

//specializations for std::vector
template<>
GeoPolygon< std::vector<sserialize::spatial::GeoPoint> >::GeoPolygon(const std::vector<sserialize::spatial::GeoPoint> & points);

template<>
GeoPolygon<std::vector<sserialize::spatial::GeoPoint> > GeoPolygon< std::vector<sserialize::spatial::GeoPoint> >::fromRect(const GeoRect & rect);

//specializations for AbstractArray
template<>
GeoPolygon< sserialize::AbstractArray<sserialize::spatial::GeoPoint> > GeoPolygon< sserialize::AbstractArray<sserialize::spatial::GeoPoint> >::fromRect(const GeoRect & rect);


}//end namespace detail

typedef detail::GeoPolygon< std::vector<sserialize::spatial::GeoPoint> > GeoPolygon;


}}//end namespace

namespace std {
template<>
inline void swap<sserialize::spatial::GeoPolygon>(sserialize::spatial::GeoPolygon & a, sserialize::spatial::GeoPolygon & b) { a.swap(b);}
}

///serializes without type info
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPolygon & p);


#endif
