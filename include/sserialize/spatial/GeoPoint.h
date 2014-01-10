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
	
	virtual void recalculateBoundary();
	
	virtual UByteArrayAdapter & append(UByteArrayAdapter & destination) const;
	
	///Check if two lines given by p->q and r->s intersect each other
	static bool intersect(const GeoPoint & p , const GeoPoint & q, const GeoPoint & r, const GeoPoint & s);
	
	virtual sserialize::spatial::GeoShape * copy() const;
};

}}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPoint & point);

inline bool operator==(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b) {
	return (a.lat == b.lat && a.lon == b.lon);
}

inline bool operator!=(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b) {
	return (a.lat != b.lat || a.lon != b.lon);
}

#endif