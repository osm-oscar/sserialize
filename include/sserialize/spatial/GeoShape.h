#ifndef SSERIALIZE_SPATIAL_GEOSHAPE_H
#define SSERIALIZE_SPATIAL_GEOSHAPE_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include "GeoRect.h"

namespace sserialize {
namespace spatial {

///GeoShapes ordered with increasing complexity. GS_END is one beyond the last one => for(int i(GS_BEGIN); i != GS_END; ++i)
typedef enum {
	GS_UNDEFINED=0, GS_NONE=0,
	GS_BEGIN=0,
	GS_FIRST_SPATIAL_OBJECT=1, GS_POINT=1, GS_WAY=2, GS_POLYGON=3, GS_MULTI_POLYGON=4, GS_LAST_SPATIAL_OBJECT=4,
	GS_FIRST_STATIC_SPATIAL_OBJECT=5, GS_STATIC_POINT=5, GS_STATIC_WAY=6, GS_STATIC_POLYGON=7, GS_STATIC_MULTI_POLYGON=8,
	GS_LAST_STATIC_SPATIAL_OBJECT=8,
	GS_END=9} GeoShapeType;

class GeoShape {
public:
	GeoShape() {};
	virtual ~GeoShape() {};
	virtual GeoShapeType type() const = 0;
	virtual uint32_t size() const = 0;
	virtual GeoRect boundary() const = 0;
	virtual void recalculateBoundary() = 0;
	virtual bool intersects(const GeoRect & boundary) const = 0;
	virtual UByteArrayAdapter & append(sserialize::UByteArrayAdapter & destination) const = 0;
	virtual GeoShape * copy() const = 0;
	virtual std::ostream & asString(std::ostream & out) const { return out; }
	
	///this can be Null
	inline UByteArrayAdapter & appendWithTypeInfo(sserialize::UByteArrayAdapter & destination) const {
		if (this) {
			destination << static_cast<uint8_t>( type() );
			return append(destination);
		}
		else {
			return destination << static_cast<uint8_t>(GS_NONE);
		}
	}
};

}}//end namespace

#endif