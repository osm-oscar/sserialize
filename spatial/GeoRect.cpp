#include <sserialize/spatial/GeoRect.h>
#include <sstream>

namespace sserialize {
namespace spatial {


GeoRect::GeoRect() {
	m_lat[0] = 0;
	m_lat[1] = 0;
	m_lon[0] = 0;
	m_lon[1] = 0;
}

GeoRect::GeoRect(const GeoRect & other) : m_lat({other.m_lat[0], other.m_lat[1]}), m_lon({other.m_lon[0], other.m_lon[1]}) {}

GeoRect::GeoRect(double latLeft, double latRight, double lonLeft, double lonRight) {
	if (latLeft > latRight)
		std::swap(latLeft, latRight);
	if (lonLeft > lonRight)
		std::swap(lonLeft, lonRight);
	m_lat[0] = latLeft;
	m_lat[1] = latRight;
	m_lon[0] = lonLeft;
	m_lon[1] = lonRight;
}

GeoRect::GeoRect(const std::string & str) {
	std::stringstream ss(str);
	ss >> m_lat[0] >> m_lat[1] >> m_lon[0] >> m_lon[1];
}

GeoRect::~GeoRect() {}
double* GeoRect::lat() { return m_lat; }
const double* GeoRect::lat() const { return m_lat; }
double* GeoRect::lon() { return m_lon; }
const double* GeoRect::lon() const { return m_lon; }

double GeoRect::minLat() const { return m_lat[0]; }
double GeoRect::maxLat() const { return m_lat[1]; }

double GeoRect::minLon() const { return m_lon[0]; }
double GeoRect::maxLon() const { return m_lon[1]; }

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

bool GeoRect::operator==(const GeoRect & other) const {
	for(size_t i = 0; i < 2; i++) {
		if (m_lat[i] != other.m_lat[i] || m_lon[i] != other.m_lon[i])
			return false;
	}
	return true;
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
	m_lat[0] = std::min(m_lat[0], other.m_lat[0]);
	m_lat[1] = std::max(m_lat[1], other.m_lat[1]);
	
	m_lon[0] = std::min(m_lon[0], other.m_lon[0]);
	m_lon[1] = std::max(m_lon[1], other.m_lon[1]);
}

}}//end namespace

std::ostream & operator<<(std::ostream & out, const sserialize::spatial::GeoRect & rect) {
	return out << "GeoRect[(" << rect.lat()[0] << ", " << rect.lon()[0] << "); (" << rect.lat()[1] << ", " << rect.lon()[1]  << ")]";
}