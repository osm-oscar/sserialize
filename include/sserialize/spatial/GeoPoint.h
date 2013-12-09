#ifndef SSERIALIZE_SPATIAL_GEO_POINT_H
#define SSERIALIZE_SPATIAL_GEO_POINT_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include "GeoShape.h"

namespace sserialize {

namespace Static {
namespace spatial {
	class GeoPoint;
}}

namespace spatial {


class GeoPoint: public GeoShape {
public:
	double lat;
	double lon;
	GeoPoint();
	GeoPoint(double lat, double lon);
	GeoPoint(std::pair<double, double> p);
	GeoPoint(std::pair<float, float> p);
	GeoPoint(const sserialize::Static::spatial::GeoPoint & sgeop);
	GeoPoint(const GeoPoint & other);
	virtual ~GeoPoint();
	GeoPoint& operator=(const GeoPoint & other);
	bool valid() const;
	
	virtual GeoShapeType type() const;
	virtual GeoRect boundary() const;
	virtual uint32_t size() const;
	virtual bool intersects(const GeoRect & boundary) const;
	
	virtual UByteArrayAdapter & serializeWithTypeInfo(UByteArrayAdapter & destination) const;
	UByteArrayAdapter & serialize(UByteArrayAdapter & destination) const;
	
	///Check if two lines given by p->q and r->s intersect each other
	static bool intersect(const GeoPoint & p , const GeoPoint & q, const GeoPoint & r, const GeoPoint & s);
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

inline bool operator==(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b) {
	return (a.lat == b.lat && a.lon == b.lon);
}

inline bool operator!=(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b) {
	return (a.lat != b.lat || a.lon != b.lon);
}

#endif