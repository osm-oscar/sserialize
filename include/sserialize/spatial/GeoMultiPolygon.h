#ifndef SSERIALIZE_GEO_AREA_H
#define SSERIALIZE_GEO_AREA_H
#include <sserialize/spatial/GeoPolygon.h>

namespace sserialize {
namespace spatial {

///Call recalculateBoundaries() after construction
class GeoMultiPolygon: public GeoShape {
public:
	typedef std::vector<GeoPolygon> PolygonList;
	typedef GeoPolygon::Point Point;
private:
	//inner polygons define what's inside the poylgons
	PolygonList m_innerPolygons;
	//outer polygons define what's outside of the polygons
	PolygonList m_outerPolygons;
	GeoRect m_innerBoundary;
	GeoRect m_outerBoundary;
	uint32_t m_size;
public:
	GeoMultiPolygon();
	virtual ~GeoMultiPolygon();
	virtual GeoShapeType type() const;
	virtual uint32_t size() const;
	virtual GeoRect boundary() const;
	virtual bool intersects(const GeoRect & boundary) const;
	bool collidesWithPolygon(const GeoPolygon & poly) const;
	
	
	void recalculateBoundaries();
	
	///boundary of polygons that define what's inside
	const GeoRect & innerBoundary() const;
	///boundary of polygons that define what's outside
	const GeoRect & outerBoundary() const;
	
	///polygons that define what's inside the polygons
	const PolygonList & innerPolygons() const;
	///polygons that define what's inside the polygons
	PolygonList & innerPolygons();
	
	///polygons that define what's outside the polygons
	const PolygonList & outerPolygons() const;
	///polygons that define what's outside the polygons
	PolygonList & outerPolygons();
	
	bool test(const Point & p) const;
	bool test(const std::deque<Point> & ps) const;
	bool test(const std::vector<Point> & ps) const;
	
	virtual UByteArrayAdapter & serializeWithTypeInfo(sserialize::UByteArrayAdapter & destination) const;
	
};

}}

#endif