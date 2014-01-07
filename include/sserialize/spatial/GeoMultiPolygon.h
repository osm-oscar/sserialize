#ifndef SSERIALIZE_GEO_AREA_H
#define SSERIALIZE_GEO_AREA_H
#include <sserialize/spatial/GeoPolygon.h>

namespace sserialize {
namespace spatial {

///Call recalculateBoundaries() after construction
class GeoMultiPolygon: public GeoRegion {
public:
	typedef std::vector<GeoPolygon> PolygonList;
	typedef GeoPolygon::Point Point;
private:
	//Inner polygons are within an enclosing polygon. The space inside ist defined as beeing outside of the enclosing polygon
	PolygonList m_innerPolygons;
	//outer polygons define the region of space
	PolygonList m_outerPolygons;
	//The inner boundary is the merged boundary of all inner polygons 
	GeoRect m_innerBoundary;
	//The outer boundary is the boundary enclosing the whole MultiPolygon
	GeoRect m_outerBoundary;
	uint32_t m_size;
private:
	bool collidesWithMultiPolygon(const GeoMultiPolygon & multiPoly) const;
	bool collidesWithPolygon(const GeoPolygon & poly) const;
	bool collidesWithWay(const sserialize::spatial::GeoWay& way) const;
public:
	GeoMultiPolygon();
	GeoMultiPolygon(GeoMultiPolygon && other);
	GeoMultiPolygon(const GeoMultiPolygon & other);
	virtual ~GeoMultiPolygon();
	GeoMultiPolygon & operator=(GeoMultiPolygon && other);
	GeoMultiPolygon & operator=(const GeoMultiPolygon & other);
	virtual GeoShapeType type() const;
	virtual uint32_t size() const;
	virtual GeoRect boundary() const;
	virtual bool intersects(const GeoRect & boundary) const;
	virtual bool contains(const GeoPoint & p) const;
	///@return true if the line p1->p2 intersects this region
	virtual bool intersects(const GeoPoint & p1, const GeoPoint & p2) const;
	virtual bool intersects(const GeoRegion & other) const;
	
	
	void recalculateBoundaries();
	
	///boundary of the inner polygons that define holes
	const GeoRect & innerPolygonsBoundary() const;
	///boundary of polygons that define the region
	const GeoRect & outerPolygonsBoundary() const;
	
	///polygons that lie within a outer polygon and their space defined whats outside
	const PolygonList & innerPolygons() const;
	///polygons that lie within a outer polygon and their space defined whats outside
	PolygonList & innerPolygons();
	
	///polygons that define what's inside
	const PolygonList & outerPolygons() const;
	///polygons that define what's inside
	PolygonList & outerPolygons();

	virtual UByteArrayAdapter & append(sserialize::UByteArrayAdapter & destination) const;
	
	virtual sserialize::spatial::GeoShape * copy() const;
	
};

}}

#endif