#ifndef SSERIALIZE_STATIC_SPATIAL_GEOPOINT_H
#define SSERIALIZE_STATIC_SPATIAL_GEOPOINT_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <stdint.h>

namespace sserialize {
namespace Static {
namespace spatial {

class GeoPoint {
public:
	uint32_t lat;
	uint32_t lon;
	GeoPoint(const UByteArrayAdapter & data) : lat(data.getUint24(0)), lon(data.getUint24(3)) {}
	GeoPoint(uint32_t lat, uint32_t lon) : lat(lat), lon(lon) {}
	GeoPoint() : lat(0), lon(0) {}
	virtual ~GeoPoint() {}
	inline double latF() const { return toDoubleLat(lat); }
	inline double lonF() const { return toDoubleLon(lon); }
	inline double latD() const { return toDoubleLat(lat); }
	inline double lonD() const { return toDoubleLon(lon); }

	inline double distance(const GeoPoint & p) const;
	
	//static conversion functions
	static inline uint32_t toIntLat(double lat) {
		return (uint32_t)((lat+90.0)*0xFFFFFF/180.0);
	}

	static inline double toDoubleLat(uint32_t lat) {
		return ((double)lat*180.0/0xFFFFFF-90.0);
	}

	static inline uint32_t toIntLon(double lon) {
		return (uint32_t)((lon+180.0)/360.0*0xFFFFFF);
	}

	static inline double toDoubleLon(uint32_t lon) {
		return ((double)lon*360.0/0xFFFFFF-180.0);
	}
	
	//simulate constexpr
	enum {SERIALIZED_FIXED_SIZE=0};
	enum {SERIALIZED_SIZE=6};
};


}}}//end namespace

inline sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & destination, sserialize::Static::spatial::GeoPoint & p) {
	p.lat = destination.getUint24();
	p.lon = destination.getUint24();
	return destination;
}

#endif