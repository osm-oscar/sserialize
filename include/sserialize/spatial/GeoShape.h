#ifndef SSERIALIZE_SPATIAL_GEOSHAPE_H
#define SSERIALIZE_SPATIAL_GEOSHAPE_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/spatial/DistanceCalculator.h>

namespace sserialize {
namespace spatial {

///GeoShapes ordered with increasing complexity. GS_END is one beyond the last one => for(int i(GS_BEGIN); i != GS_END; ++i)
typedef enum {
	GS_UNDEFINED=0, GS_NONE=0, GS_INVALID=0,
	GS_BEGIN=0,
	GS_FIRST_SPATIAL_OBJECT=1, GS_POINT=1, GS_WAY=2, GS_POLYGON=3, GS_MULTI_POLYGON=4, GS_UNION_SHAPE=5, GS_LAST_SPATIAL_OBJECT=5,
	GS_FIRST_STATIC_SPATIAL_OBJECT=6, GS_STATIC_POINT=6, GS_STATIC_WAY=7, GS_STATIC_POLYGON=8, GS_STATIC_MULTI_POLYGON=9,
	GS_LAST_STATIC_SPATIAL_OBJECT=9,
	GS_END=10} GeoShapeType;

class GeoShape {
public:
	GeoShape() {};
	virtual ~GeoShape() {};
	virtual GeoShapeType type() const = 0;
	///number of points this shape is made-up of
	virtual uint32_t size() const = 0;
	virtual GeoRect boundary() const = 0;
	virtual void recalculateBoundary() = 0;
	virtual bool intersects(const GeoRect & boundary) const = 0;
	///return the distance to each other (only consider the distance to the border of the object, use intersects for collision testing)
	virtual double distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const  = 0;
	virtual UByteArrayAdapter & append(sserialize::UByteArrayAdapter & destination) const = 0;
	virtual GeoShape * copy() const = 0;
	virtual std::ostream & asString(std::ostream & out) const { return out; }
	std::string toString() const;
	void dump() const;

// 	virtual double realDistance(const sserialize::spatial::GeoShape & other) const;
	
	///ptr is allwoed to be null
	static inline UByteArrayAdapter & appendWithTypeInfo(const GeoShape * ptr, sserialize::UByteArrayAdapter & destination) {
		if (ptr) {
			destination << static_cast<uint8_t>( ptr->type() );
			return ptr->append(destination);
		}
		else {
			return destination << static_cast<uint8_t>(GS_NONE);
		}
	}
	template<typename T_GEO_SHAPE_ITERATOR>
	static GeoRect bounds(T_GEO_SHAPE_ITERATOR begin, T_GEO_SHAPE_ITERATOR end);
	
};

template<typename T_GEO_SHAPE_ITERATOR>
GeoRect GeoShape::bounds(T_GEO_SHAPE_ITERATOR begin, T_GEO_SHAPE_ITERATOR end) {
	if (begin != end) {
		GeoRect r((*begin)->boundary());
		for(++begin; begin != end; ++begin) {
			r.enlarge((*begin)->boundary());
		}
		return r;
	}
	return GeoRect();
}


}}//end namespace

#endif