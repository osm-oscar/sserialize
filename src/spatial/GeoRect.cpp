#include <sserialize/spatial/GeoRect.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/spatial/GeoPoint.h>
#include <sstream>
#include <algorithm>
#include <sserialize/algorithm/utilmath.h>

namespace sserialize {
namespace spatial {


GeoRect::GeoRect() {
	m_lat[0] = 0;
	m_lat[1] = 0;
	m_lon[0] = 0;
	m_lon[1] = 0;
}

GeoRect::GeoRect(const GeoRect & other) :
m_lat{other.m_lat[0], other.m_lat[1]},
m_lon{other.m_lon[0], other.m_lon[1]}
{}

GeoRect::GeoRect(double lat1, double lat2, double lon1, double lon2) :
m_lat{lat1, lat2},
m_lon{lon1, lon2}
{
	using std::swap;
	if (m_lat[0] > m_lat[1])
		swap(m_lat[0], m_lat[1]);
	if (m_lon[0] > m_lon[1])
		swap(m_lon[0], m_lon[1]);
}

GeoRect::GeoRect(const std::string & str, bool fromLeafletBBox) {
	if (fromLeafletBBox) {
		std::string tempStr = str;
		std::replace(tempStr.begin(), tempStr.end(), ',', ' ');
		std::stringstream ss(tempStr);
		ss >> m_lon[0] >> m_lat[0] >> m_lon[1] >> m_lat[1];
	}
	else {
		std::stringstream ss(str);
		ss >> m_lat[0] >> m_lat[1] >> m_lon[0] >> m_lon[1];
	}
}

GeoRect::GeoRect(const UByteArrayAdapter & data) {
	sserialize::Static::spatial::GeoPoint bL(data);
	sserialize::Static::spatial::GeoPoint tR(data+sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length);
	minLat() = bL.lat();
	minLon() = bL.lon();
	maxLat() = tR.lat();
	maxLon() = tR.lon();
}

GeoRect::~GeoRect() {}

bool GeoRect::valid() const {
	return !( m_lat[0] == 0 && m_lat[1] == 0 && m_lon[0] == 0 && m_lon[1] == 0 );
}

double* GeoRect::lat() { return m_lat; }
const double* GeoRect::lat() const { return m_lat; }
double* GeoRect::lon() { return m_lon; }
const double* GeoRect::lon() const { return m_lon; }

double GeoRect::minLat() const { return m_lat[0]; }
double GeoRect::maxLat() const { return m_lat[1]; }

double GeoRect::minLon() const { return m_lon[0]; }
double GeoRect::maxLon() const { return m_lon[1]; }

double & GeoRect::minLat() { return m_lat[0]; }
double & GeoRect::maxLat() { return m_lat[1]; }

double & GeoRect::minLon() { return m_lon[0]; }
double & GeoRect::maxLon() { return m_lon[1]; }

///TODO:check how fast it would be to calculate the real length
double GeoRect::length() const {
	return 2*(m_lat[0]-m_lat[1])+2*(m_lon[0]-m_lon[1]);
}

bool GeoRect::overlap(const GeoRect & other) const {
	if ((m_lat[0] > other.m_lat[1]) || // this is left of other
		(m_lat[1] < other.m_lat[0]) || // this is right of other
		(m_lon[0] > other.m_lon[1]) ||
		(m_lon[1] < other.m_lon[0])) {
			return false;
	}
	return true;
}

bool GeoRect::contains(double lat, double lon) const {
	return (m_lat[0] <= lat && lat <= m_lat[1] && m_lon[0] <= lon && lon <= m_lon[1]);
}

bool GeoRect::contains(const GeoRect & other) const {
	return contains(other.m_lat[0], other.m_lon[0]) && contains(other.m_lat[1], other.m_lon[1]);
}

/** clip this rect at other
	* @return: returns true if clipping worked => rects overlapped, false if they did not overlap */
bool GeoRect::clip(const GeoRect & other) {
	if (!overlap(other))
		return false;
	m_lat[0] = std::max(m_lat[0], other.m_lat[0]);
	m_lat[1] = std::min(m_lat[1], other.m_lat[1]);
	
	m_lon[0] = std::max(m_lon[0], other.m_lon[0]);
	m_lon[1] = std::min(m_lon[1], other.m_lon[1]);
	
	return true;
}

/** Enlarge this rect so that other will fit into it */
void GeoRect::enlarge(const GeoRect & other) {
	if (!other.valid()) {
		return;
	}
	if (!valid()) {
		this->operator=(other);
	}
	else {
		m_lat[0] = std::min(m_lat[0], other.m_lat[0]);
		m_lat[1] = std::max(m_lat[1], other.m_lat[1]);
		
		m_lon[0] = std::min(m_lon[0], other.m_lon[0]);
		m_lon[1] = std::max(m_lon[1], other.m_lon[1]);
	}
}

void GeoRect::resize(double latFactor, double lonFactor) {
	double latLen = (maxLat()-minLat())/2.0;
	double lonLen = (maxLon()-minLon())/2.0;
	
	double latMid  = minLat() + latLen;
	double lonMid = minLon() + lonLen;
	
	minLat() = latMid - latLen*latFactor;
	maxLat() = latMid + latLen*latFactor;
	
	minLon() = lonMid - lonLen*lonFactor;
	maxLon() = lonMid + lonLen*lonFactor;
}


GeoRect GeoRect::operator/(const GeoRect & other) const {
	double myMinLat = std::max(minLat(), other.minLat());
	double myMaxLat = std::min(maxLat(), other.maxLat());
	if (myMinLat >= myMaxLat)
		return GeoRect();
	double myMinLon = std::max(minLon(), other.minLon());
	double myMaxLon = std::min(maxLon(), other.maxLon());
	if (myMinLon >= myMaxLon)
		return GeoRect();
	return GeoRect(myMinLat, myMaxLat, myMinLon, myMaxLon);
}


sserialize::UByteArrayAdapter & operator<<(::sserialize::UByteArrayAdapter & destination, const GeoRect & rect) {
	return destination << sserialize::spatial::GeoPoint(rect.minLat(), rect.minLon())
						<< sserialize::spatial::GeoPoint(rect.maxLat(), rect.maxLon());
}

sserialize::UByteArrayAdapter & operator>>(::sserialize::UByteArrayAdapter & src, GeoRect & rect) {
	sserialize::Static::spatial::GeoPoint bL, tR;
	src >> bL >> tR;
	rect.minLat() = bL.lat();
	rect.minLon() = bL.lon();
	rect.maxLat() = tR.lat();
	rect.maxLon() = tR.lon();
	return src;
}

bool operator==(const GeoRect & a, const GeoRect & b) {
	return sserialize::geoEq(a.minLat(), b.minLat()) && sserialize::geoEq(a.maxLat(), b.maxLat())
				&& sserialize::geoEq(a.minLon(), b.minLon()) && sserialize::geoEq(a.maxLon(), b.maxLon());
}

bool operator!=(const GeoRect & a, const GeoRect & b) {
	return sserialize::geoNeq(a.minLat(), b.minLat()) || sserialize::geoNeq(a.maxLat(), b.maxLat())
				|| sserialize::geoNeq(a.minLon(), b.minLon()) || sserialize::geoNeq(a.maxLon(), b.maxLon());
}

std::ostream & operator<<(std::ostream & out, const GeoRect & rect) {
	return out << "GeoRect[(" << rect.minLat() << ", " << rect.minLon() << "); (" << rect.maxLat() << ", " << rect.maxLon()  << ")]";
}

}}//end namespace