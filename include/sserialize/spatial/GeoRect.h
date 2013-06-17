#ifndef SSERIALIZE_SPATIAL_GEO_RECT_H
#define SSERIALIZE_SPATIAL_GEO_RECT_H
#include <ostream>

namespace sserialize {
namespace spatial {


class GeoRect {
	double m_lat[2];
	double m_lon[2];
public:
	GeoRect() {
		m_lat[0] = 0;
		m_lat[1] = 0;
		m_lon[0] = 0;
		m_lon[1] = 0;
	}
	GeoRect(double latLeft, double latRight, double lonLeft, double lonRight) {
		if (latLeft > latRight)
			std::swap(latLeft, latRight);
		if (lonLeft > lonRight)
			std::swap(lonLeft, lonRight);
		m_lat[0] = latLeft;
		m_lat[1] = latRight;
		m_lon[0] = lonLeft;
		m_lon[1] = lonRight;
	}
	virtual ~GeoRect() {}
	double* lat() { return m_lat; }
	const double* lat() const { return m_lat; }
	double* lon() { return m_lon; }
	const double* lon() const { return m_lon; }
	
	double minLat() const { return m_lat[0]; }
	double maxLat() const { return m_lat[1]; }
	
	double minLon() const { return m_lon[0]; }
	double maxLon() const { return m_lon[1]; }
	
	bool overlap(const GeoRect & other) const {
		if ((m_lat[0] > other.m_lat[1]) || // this is left of other
			(m_lat[1] < other.m_lat[0]) || // this is right of other
			(m_lon[0] > other.m_lon[1]) ||
			(m_lon[1] < other.m_lon[0])) {
				return false;
		}
		return true;
	}
	bool contains(double lat, double lon) const {
		return (m_lat[0] <= lat && lat <= m_lat[1] && m_lon[0] <= lon && lon <= m_lon[1]);
	}
	
	bool contains(const GeoRect & other) const {
		return contains(other.m_lat[0], other.m_lon[0]) && contains(other.m_lat[1], other.m_lon[1]);
	}
	
	bool operator==(const GeoRect & other) const {
		for(size_t i = 0; i < 2; i++) {
			if (m_lat[i] != other.m_lat[i] || m_lon[i] != other.m_lon[i])
				return false;
		}
		return true;
	}
	
	/** clip this rect at other
	  * @return: returns true if clipping worked => rects overlapped, false if they did not overlap */
	bool clip(const GeoRect & other) {
		if (!overlap(other))
			return false;
		m_lat[0] = std::max(m_lat[0], other.m_lat[0]);
		m_lat[1] = std::min(m_lat[1], other.m_lat[1]);
		
		m_lon[0] = std::max(m_lon[0], other.m_lon[0]);
		m_lon[1] = std::min(m_lon[1], other.m_lon[1]);
		
		return true;
	}
	
	/** Enlarge this rect so that other will fit into it */
	void enlarge(const GeoRect & other) {
		m_lat[0] = std::min(m_lat[0], other.m_lat[0]);
		m_lat[1] = std::max(m_lat[1], other.m_lat[1]);
		
		m_lon[0] = std::min(m_lon[0], other.m_lon[0]);
		m_lon[1] = std::max(m_lon[1], other.m_lon[1]);
	}
};

}}//end namespace

inline std::ostream & operator<<(std::ostream & out, const sserialize::spatial::GeoRect & rect) {
	return out << "GeoRect[(" << rect.lat()[0] << ", " << rect.lon()[0] << "); (" << rect.lat()[1] << ", " << rect.lon()[1]  << ")]";
}

#endif