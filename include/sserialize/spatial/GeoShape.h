#ifndef SSERIALIZE_SPATIAL_GEOSHAPE_H
#define SSERIALIZE_SPATIAL_GEOSHAPE_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include "GeoRect.h"

namespace sserialize {
namespace spatial {


typedef enum { GS_NONE=0, GS_POINT=1, GS_WAY=2, GS_POLYGON=3 } GeoShapeType;

class GeoShape {
public:
	GeoShape() {};
	virtual ~GeoShape() {};
	virtual uint32_t size() const = 0;
	virtual GeoRect boundaryRect() const = 0;
	virtual bool intersects(const GeoRect & boundary) const = 0;
	virtual UByteArrayAdapter & serializeWithTypeInfo(sserialize::UByteArrayAdapter & destination) const = 0;
};

}}//end namespace

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoShape & shape);
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoRect & rect);

#endif