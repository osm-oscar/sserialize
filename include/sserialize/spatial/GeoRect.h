#ifndef SSERIALIZE_SPATIAL_GEO_RECT_H
#define SSERIALIZE_SPATIAL_GEO_RECT_H
#include <ostream>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/SerializationInfo.h>

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
	///separated by whitespace, if fromLeafletBBox is set then southwest_lng,southwest_lat,northeast_lng,northeast_lat
	GeoRect(const std::string & str, bool fromLeafletBBox = false);
	GeoRect(const UByteArrayAdapter & data);
	virtual ~GeoRect();
	
	double* lat();
	const double* lat() const;
	double* lon();
	const double* lon() const;
	
	double minLat() const;
	double maxLat() const;
	
	double minLon() const;
	double maxLon() const;
	
	double & minLat();
	double & maxLat();
	
	double & minLon();
	double & maxLon();
	
	double diagonalLength() const;
	
	double length() const;
	
	bool overlap(const GeoRect & other) const;
	bool contains(double lat, double lon) const;
	
	bool contains(const GeoRect & other) const;
	
	/** clip this rect at other
	  * @return: returns true if clipping worked => rects overlapped, false if they did not overlap */
	bool clip(const GeoRect & other);
	
	/** Enlarge this rect so that other will fit into it */
	void enlarge(const GeoRect & other);
	
	///Resize this Rect by lat in latitude and lon in longitude
	void resize(double latFactor, double lonFactor);
	
	GeoRect operator/(const GeoRect & other) const;
};

bool operator==(const sserialize::spatial::GeoRect & a, const GeoRect & b);
bool operator!=(const sserialize::spatial::GeoRect & a, const GeoRect & b);
std::ostream & operator<<(std::ostream & out, const GeoRect & rect);

sserialize::UByteArrayAdapter & operator<<(::sserialize::UByteArrayAdapter & destination, const GeoRect & rect);
sserialize::UByteArrayAdapter & operator>>(::sserialize::UByteArrayAdapter & src, GeoRect & rect);

}}//end namespacec

namespace sserialize {

	template<>
	struct SerializationInfo<sserialize::spatial::GeoRect> {
		static const bool is_fixed_length = true;
		static const OffsetType length = 16; //equals 2 GeoPoint
		static const OffsetType max_length = 16;
		static const OffsetType min_length = 16;
		static inline OffsetType sizeInBytes(const sserialize::spatial::GeoRect & /*value*/) { return 16; }
	};
}

#endif