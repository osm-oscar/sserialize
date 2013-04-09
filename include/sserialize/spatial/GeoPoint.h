#ifndef OSMFIND_CREATE_GEO_POINT_H
#define OSMFIND_CREATE_GEO_POINT_H
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include "GeoShape.h"

namespace sserialize {
namespace spatial {

class GeoPoint: public GeoShape {
public:
	double lat;
	double lon;
	GeoPoint() : lat(1337.0), lon(-1337.0) {}
	GeoPoint(double lat, double lon) : lat(lat), lon(lon) {};
	GeoPoint(std::pair<double, double> p) : lat(p.first), lon(p.second) {}
	GeoPoint(std::pair<float, float> p) : lat(p.first), lon(p.second) {}
	GeoPoint(const sserialize::Static::spatial::GeoPoint & sgeop) : lat(sgeop.latF()), lon(sgeop.lonF()) {};
	GeoPoint(const GeoPoint & other) : lat(other.lat), lon(other.lon) {}
	GeoPoint& operator=(const GeoPoint & other) {
		lat = other.lat;
		lon = other.lon;
		return *this;
	}
	virtual ~GeoPoint() {}
	bool valid() const { return lat != 1337.0; }
	virtual GeoRect boundaryRect() const {
		return GeoRect(lat, lat, lon, lon);
	}
	virtual uint32_t size() const { return 1;}
	virtual bool intersects(const GeoRect & boundary) const {
		return boundary.contains(lat, lon);
	}
	
	virtual UByteArrayAdapter & serializeWithTypeInfo(UByteArrayAdapter & destination) const {
		destination << static_cast<uint8_t>( GS_POINT );
		return serialize(destination);
	}
	
	UByteArrayAdapter & serialize(UByteArrayAdapter & destination) const {
		destination.putUint24(sserialize::Static::spatial::GeoPoint::toIntLat(lat));
		destination.putUint24(sserialize::Static::spatial::GeoPoint::toIntLon(lon));
		return destination;
	}
};

}}

inline sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPoint & point) {
	return point.serialize(destination);
}

inline sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & in, sserialize::spatial::GeoRect & out) {
	sserialize::spatial::GeoPoint(out.lat()[0], out.lon()[0]).serialize(in);
	sserialize::spatial::GeoPoint(out.lat()[1], out.lon()[1]).serialize(in);
	return in;
}

#endif