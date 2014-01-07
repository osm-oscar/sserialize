#ifndef SSERIALIZE_SPATIAL_POLYGON_H
#define SSERIALIZE_SPATIAL_POLYGON_H
#include <vector>
#include <cmath>
#include "GeoWay.h"

namespace sserialize {
namespace spatial {


///GeoPolygon is just GeoWay where the last node equals the first
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
	bool encloses(const GeoWay & other) const;
	template<typename T_GEO_POINT_ITERATOR>
	bool contains(T_GEO_POINT_ITERATOR begin, T_GEO_POINT_ITERATOR end) const;

	virtual UByteArrayAdapter & append(UByteArrayAdapter & destination) const;
	
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

namespace std {
template<>
inline void swap<sserialize::spatial::GeoPolygon>(sserialize::spatial::GeoPolygon & a, sserialize::spatial::GeoPolygon & b) { a.swap(b);}
}

///serializes without type info
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPolygon & p);


#endif
