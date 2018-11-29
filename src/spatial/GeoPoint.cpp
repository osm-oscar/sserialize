#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/algorithm/utilmath.h>
#include <cmath>
#include <geographiclib/legacy/C/geodesic.h>

namespace sserialize {
namespace spatial {

GeoPoint::GeoPoint() :
m_lat(1337.0),
m_lon(-1337.0)
{}

GeoPoint::GeoPoint(double lat, double lon) :
m_lat(lat),
m_lon(lon)
{}

GeoPoint::GeoPoint(std::pair<double, double> p) :
m_lat(p.first),
m_lon(p.second)
{}

GeoPoint::GeoPoint(std::pair<float, float> p) :
m_lat(p.first),
m_lon(p.second)
{}


GeoPoint::GeoPoint(const UByteArrayAdapter & data) :
m_lat(toDoubleLat(data.getUint32(0))),
m_lon(toDoubleLon(data.getUint32(4)))
{}

GeoPoint::GeoPoint(uint32_t lat, uint32_t lon) :
m_lat(toDoubleLat(lat)),
m_lon(toDoubleLon(lon))
{}

GeoPoint::GeoPoint(const std::string & str) {
	std::sscanf("%*[(] %lf %*[;,] %lf %*[)]", str.c_str(), m_lat, m_lon);
}

GeoPoint::GeoPoint(const GeoPoint & other) :
m_lat(other.m_lat),
m_lon(other.m_lon)
{}


GeoPoint& GeoPoint::operator=(const GeoPoint & other) {
	m_lat = other.m_lat;
	m_lon = other.m_lon;
	return *this;
}

GeoPoint::~GeoPoint() {}

GeoShapeType GeoPoint::type() const {
	return GS_POINT;
}

bool GeoPoint::valid() const {
	return -90.0 <= lat() && lat() <= 90.0 && -180.0 <= lon() && lon() <= 180.0;
}

void GeoPoint::normalize(sserialize::spatial::GeoPoint::NormalizationType nt) {
	if (nt == NT_CLIP) {
		if (lat() > 90.0) {
			lat() = 90.0;
		}
		if (lat() < -90.0) {
			lat() = -90.0;
		}
		if (lon() > 180.0) {
			lon() = 180.0;
		}
		if (lon() < -180.0) {
			lon() = -180.0;
		}
	}
	else {
		if (lat() < -90.0 || lat() > 90.0) {
			lat() = fmod(lat()+90.0, 180.0)-90.0;
		}
		if (lon() < -180.0 || lon() > 180.0) {
			lon() = fmod(lon()+180.0, 360.0)-180.0;
		}
	}
}

void GeoPoint::snap() {
	m_lat = snapLat(m_lat);
	m_lon = snapLon(m_lon);
	SSERIALIZE_CHEAP_ASSERT(isSnapped());
}

bool GeoPoint::isSnapped() const {
	return m_lat == toDoubleLat(toIntLat(m_lat)) && m_lon == toDoubleLon(toIntLon(m_lon));
}

GeoRect GeoPoint::boundary() const {
	return GeoRect(lat(), lat(), lon(), lon());
}
uint32_t GeoPoint::size() const {
	return 1;
}

bool GeoPoint::intersects(const GeoRect & boundary) const {
	return boundary.contains(lat(), lon());
}

void GeoPoint::recalculateBoundary() {}

UByteArrayAdapter & GeoPoint::append(UByteArrayAdapter & destination) const {
	destination.putUint32(sserialize::spatial::GeoPoint::toIntLat(lat()));
	destination.putUint32(sserialize::spatial::GeoPoint::toIntLon( lon()));
	return destination;
}

bool GeoPoint::intersect(const GeoPoint & p , const GeoPoint & q, const GeoPoint & r, const GeoPoint & s) {
	double tl1, tl2;
	double t1_denom = (q.lon()-p.lon())*(s.lat()-r.lat())+(q.lat()-p.lat())*(r.lon()-s.lon());
	if (std::abs(t1_denom) <= SSERIALIZE_EPSILON)
		return false;
	double t1_nom = (r.lon()-p.lon())*(s.lat()-r.lat())+(r.lat()-p.lat())*(r.lon()-s.lon());
	
	if (sserialize::sgn(t1_nom)*sserialize::sgn(t1_denom) < 0)
		return false;
	tl1 = t1_nom/t1_denom;
	if (tl1 > 1)
		return false;
	tl2 = (tl1*(q.lat()-p.lat())-r.lat()+p.lat())/(s.lat()-r.lat());
	return (0.0 <= tl2 && 1.0 >= tl2);
}

double GeoPoint::distance(const sserialize::spatial::GeoShape& other, const sserialize::spatial::DistanceCalculator& distanceCalculator) const {
	if (other.type() == sserialize::spatial::GS_POINT) {
		const sserialize::spatial::GeoPoint op = *static_cast<const sserialize::spatial::GeoPoint*>(&other);
		return distanceCalculator.calc(m_lat, m_lon, op.m_lat, op.m_lon);
	}
	else if (other.type() > GS_POINT) {
		return other.distance(*this, distanceCalculator);
	}
	return std::numeric_limits<double>::quiet_NaN();
}

sserialize::spatial::GeoShape * GeoPoint::copy() const {
	return new sserialize::spatial::GeoPoint(*this);
}

std::ostream & GeoPoint::asString(std::ostream & out) const {
	auto p = out.precision();
	out.precision(std::numeric_limits<double>::digits10+1);
	out << "GeoPoint(" << lat() << ", " << lon() << ")";
	out.precision(p);
	return out;
}

uint32_t GeoPoint::toIntLat(double lat) {
	SSERIALIZE_CHEAP_ASSERT(lat >= -90.0 && lat <= 90.0);
	return ::floor((lat+90.0) * (1 << 24) );
}

double GeoPoint::toDoubleLat(uint32_t lat) {
	return (static_cast<double>(lat) / (1 << 24)) - 90.0;
}

uint32_t GeoPoint::toIntLon(double lon) {
	SSERIALIZE_CHEAP_ASSERT(lon >= -180.0 && lon <= 180.0);
	return ::floor((lon+180.0) * (1 << 23));
}

double GeoPoint::toDoubleLon(uint32_t lon) {
	return (static_cast<double>(lon) / (1 << 23)) - 180.0;
}

double GeoPoint::snapLat(double v) {
	return toDoubleLat(toIntLat(v));
}

double GeoPoint::snapLon(double v) {
	return toDoubleLon(toIntLon(v));
}

GeoPoint GeoPoint::fromIntLatLon(uint32_t lat, uint32_t lon) { return GeoPoint(lat, lon); }

bool GeoPoint::equal(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b, double acc) {
	return (std::abs<double>(a.lat() - b.lat()) <= acc && std::abs<double>(a.lon() - b.lon()) <= acc);
}

bool GeoPoint::equal(const sserialize::spatial::GeoPoint & b, double acc) const {
	return equal(*this, b, acc);
}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPoint & point) {
	return point.append(destination);
}

sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & destination, sserialize::spatial::GeoPoint & p) {
	p.lat() = sserialize::spatial::GeoPoint::toDoubleLat(destination.getUint32());
	p.lon() = sserialize::spatial::GeoPoint::toDoubleLon(destination.getUint32());
	return destination;
}


bool equal(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b, double acc) {
	return sserialize::spatial::GeoPoint::equal (a, b, acc);
}

std::ostream & operator<<(std::ostream & out, const sserialize::spatial::GeoPoint & gp) {
	return gp.asString(out);
}

}}//end namespace
