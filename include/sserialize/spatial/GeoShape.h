#ifndef SSERIALIZE_SPATIAL_GEOSHAPE_H
#define SSERIALIZE_SPATIAL_GEOSHAPE_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include "GeoRect.h"

namespace sserialize {
namespace spatial {

///GeoShapes ordered with increasing complexity. GS_END is one beyond the last one => for(int i(GS_BEGIN); i != GS_END; ++i)
typedef enum { GS_UNDEFINED=0, GS_NONE=0, GS_BEGIN=0, GS_FIRST_SPATIAL_OBJECT=1, GS_POINT=1, GS_WAY=2, GS_POLYGON=3, GS_MULTI_POLYGON=4, GS_LAST_SPATIAL_OBJECT=4, GS_END=5} GeoShapeType;

class GeoShape {
public:
	GeoShape() {};
	virtual ~GeoShape() {};
	virtual GeoShapeType type() const = 0;
	virtual uint32_t size() const = 0;
	virtual GeoRect boundary() const = 0;
	virtual bool intersects(const GeoRect & boundary) const = 0;
	virtual UByteArrayAdapter & serializeWithTypeInfo(sserialize::UByteArrayAdapter & destination) const = 0;
	virtual GeoShape * copy() const = 0;
};

}}//end namespace

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoShape & shape);

#endif