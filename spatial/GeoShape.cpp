#include <sserialize/spatial/GeoShape.h>

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoShape & shape) {
	if (&shape)
		return shape.serializeWithTypeInfo(destination);
	else
		return destination << static_cast<uint8_t>(sserialize::spatial::GS_NONE);
}
