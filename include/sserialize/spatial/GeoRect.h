#ifndef SSERIALIZE_SPATIAL_GEO_RECT_H
#define SSERIALIZE_SPATIAL_GEO_RECT_H
#include <ostream>

namespace sserialize {
namespace spatial {


class GeoRect {
	double m_lat[2];
	double m_lon[2];
public:
	GeoRect();
	GeoRect(const GeoRect & other);
	GeoRect(double latLeft, double latRight, double lonLeft, double lonRight);
	///@param str a string holding the definition in the same order as above @GeoRect(double latLeft, double latRight, double lonLeft, double lonRight)
	GeoRect(const std::string & str); 
	virtual ~GeoRect();
	double* lat();
	const double* lat() const;
	double* lon();
	const double* lon() const;
	
	double minLat() const;
	double maxLat() const;
	
	double minLon() const;
	double maxLon() const;
	
	bool overlap(const GeoRect & other) const;
	bool contains(double lat, double lon) const;
	
	bool contains(const GeoRect & other) const;
	
	bool operator==(const GeoRect & other) const;
	/** clip this rect at other
	  * @return: returns true if clipping worked => rects overlapped, false if they did not overlap */
	bool clip(const GeoRect & other);
	
	/** Enlarge this rect so that other will fit into it */
	void enlarge(const GeoRect & other);
};

}}//end namespace

std::ostream & operator<<(std::ostream & out, const sserialize::spatial::GeoRect & rect);

#endif