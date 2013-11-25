#ifndef SSERIALIZE_SPATIAL_POLYGON_H
#define SSERIALIZE_SPATIAL_POLYGON_H
#include <vector>
#include <cmath>
#include "GeoWay.h"

namespace sserialize {
namespace spatial {



class GeoPolygon: public GeoWay<GeoPoint, std::vector<GeoPoint> > {
public:
	typedef GeoPoint Point;
	typedef GeoWay<GeoPoint, std::vector<GeoPoint> > MyBaseClass;
public:
	GeoPolygon();
	GeoPolygon(const std::vector<Point> & points);
	virtual ~GeoPolygon();
	virtual bool intersects(const GeoRect & boundary) const;
	virtual UByteArrayAdapter & serializeWithTypeInfo(UByteArrayAdapter & destination) const;
	UByteArrayAdapter & serialize(UByteArrayAdapter & destination) const;
	//http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
	bool test(const Point & p) const;
	bool test(const std::deque<Point> & ps) const;
	bool test(const std::vector<Point> & ps) const;
	bool collidesWithRect(const GeoRect & rect) const;
	bool collidesWithPolygon(const GeoPolygon & poly) const;
	bool intersectsWithLineSegment(const Point & p1, const Point & p2) const;
	bool intersectLineSegments(const Point & p , const Point & q, const Point & r, const Point & s) const;
	bool contains(const GeoPolygon & other) const;
	static GeoPolygon fromRect(const GeoRect & rect);
};


	
}}//end namespace

inline sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPolygon & p) {
	return p.serialize(destination);
}

#endif
