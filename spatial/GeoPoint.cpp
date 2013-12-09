#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/utility/utilmath.h>

namespace sserialize {
namespace spatial {

GeoPoint::GeoPoint() :
lat(1337.0),
lon(-1337.0)
{}

GeoPoint::GeoPoint(double lat, double lon) :
lat(lat),
lon(lon)
{}

GeoPoint::GeoPoint(std::pair<double, double> p) :
lat(p.first),
lon(p.second)
{}

GeoPoint::GeoPoint(std::pair<float, float> p) :
lat(p.first),
lon(p.second)
{}

GeoPoint::GeoPoint(const sserialize::Static::spatial::GeoPoint & sgeop) :
lat(sgeop.latD()),
lon(sgeop.lonF())
{}

GeoPoint::GeoPoint(const GeoPoint & other) :
lat(other.lat),
lon(other.lon)
{}

GeoPoint& GeoPoint::operator=(const GeoPoint & other) {
	lat = other.lat;
	lon = other.lon;
	return *this;
}

GeoPoint::~GeoPoint() {}

GeoShapeType GeoPoint::type() const {
	return GS_POINT;
}

bool GeoPoint::valid() const {
	return -90.0 <= lat && lat <= 90.0 && 0.0 <= lon && lon <= 360.0;
}

GeoRect GeoPoint::boundary() const {
	return GeoRect(lat, lat, lon, lon);
}
uint32_t GeoPoint::size() const {
	return 1;
}

bool GeoPoint::intersects(const GeoRect & boundary) const {
	return boundary.contains(lat, lon);
}

UByteArrayAdapter & GeoPoint::serializeWithTypeInfo(UByteArrayAdapter & destination) const {
	destination << static_cast<uint8_t>( GS_POINT );
	return serialize(destination);
}

UByteArrayAdapter & GeoPoint::serialize(UByteArrayAdapter & destination) const {
	destination.putUint24(sserialize::Static::spatial::GeoPoint::toIntLat(lat));
	destination.putUint24(sserialize::Static::spatial::GeoPoint::toIntLon(lon));
	return destination;
}

bool GeoPoint::intersect(const GeoPoint & p , const GeoPoint & q, const GeoPoint & r, const GeoPoint & s) {
	double tl1, tl2;
	double t1_denom = (q.lon-p.lon)*(s.lat-r.lat)+(q.lat-p.lat)*(r.lon-s.lon);
	if (std::abs(t1_denom) <= 0.000001)
		return false;
	double t1_nom = (r.lon-p.lon)*(s.lat-r.lat)+(r.lat-p.lat)*(r.lon-s.lon);
	
	if (sserialize::sgn(t1_nom)*sserialize::sgn(t1_denom) < 0)
		return false;
	tl1 = t1_nom/t1_denom;
	if (tl1 > 1)
		return false;
	tl2 = (tl1*(q.lat-p.lat)-r.lat+p.lat)/(s.lat-r.lat);
	return (0.0 <= tl2 && 1.0 >= tl2);
}

}}//end namespace