#ifndef SSERIALIZE_SPATIAL_POLYGON_H
#define SSERIALIZE_SPATIAL_POLYGON_H
#include <vector>
#include <cmath>
#include "GeoWay.h"

namespace sserialize {
namespace spatial {



class GeoPolygon: public GeoWay {
public:
	typedef GeoPoint Point;
	typedef GeoWay MyBaseClass;
protected:
	bool collidesWithPolygon(const GeoPolygon & poly) const;
	bool collidesWithWay(const GeoWay & way) const;
public:
	GeoPolygon();
	GeoPolygon(const std::vector<Point> & points);
	virtual ~GeoPolygon();
	virtual GeoShapeType type() const;
	virtual bool contains(const GeoPoint & p) const;
	virtual bool intersects(const sserialize::spatial::GeoRect & rect) const;
	///@return true if the line p1->p2 intersects this region
	virtual bool intersects(const GeoPoint & p1, const GeoPoint & p2) const;
	virtual bool intersects(const GeoRegion & other) const;
	bool encloses(const GeoPolygon & other) const;
	template<typename T_GEO_POINT_ITERATOR>
	bool contains(T_GEO_POINT_ITERATOR begin, T_GEO_POINT_ITERATOR end) const;

	virtual UByteArrayAdapter & serializeWithTypeInfo(UByteArrayAdapter & destination) const;
	UByteArrayAdapter & serialize(UByteArrayAdapter & destination) const;
	
	virtual sserialize::spatial::GeoShape * copy() const;
	
	static GeoPolygon fromRect(const GeoRect & rect);
};


template<typename T_GEO_POINT_ITERATOR>
bool GeoPolygon::contains(T_GEO_POINT_ITERATOR begin, T_GEO_POINT_ITERATOR end) const {
	for(; begin != end; ++begin) {
		if (contains(*begin))
			return true;
	}
	return false;
}

	
}}//end namespace

inline sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPolygon & p) {
	return p.serialize(destination);
}

#endif
