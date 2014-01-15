#ifndef SSERIALIZE_SPATIAL_GEO_POINT_H
#define SSERIALIZE_SPATIAL_GEO_POINT_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/SerializationInfo.h>
#include "GeoShape.h"

namespace sserialize {
namespace spatial {


class GeoPoint: public GeoShape {
private:
	double m_lat;
	double m_lon;
private:
	explicit GeoPoint(uint32_t lat, uint32_t lon);
public:
	GeoPoint();
	GeoPoint(double lat, double lon);
	GeoPoint(std::pair<double, double> p);
	GeoPoint(std::pair<float, float> p);
	GeoPoint(const UByteArrayAdapter & data);
	GeoPoint(const GeoPoint & other);
	virtual ~GeoPoint();
	GeoPoint& operator=(const GeoPoint & other);
	bool valid() const;
	inline const double & lat() const { return m_lat; }
	inline const double & lon() const {return m_lon;}
	inline double & lat() { return m_lat; }
	inline double & lon() { return m_lon; }
	
	
	virtual GeoShapeType type() const;
	virtual GeoRect boundary() const;
	virtual uint32_t size() const;
	virtual bool intersects(const GeoRect & boundary) const;
	
	virtual void recalculateBoundary();
	
	virtual UByteArrayAdapter & append(UByteArrayAdapter & destination) const;
	
	virtual sserialize::spatial::GeoShape * copy() const;
	
	///Check if two lines given by p->q and r->s intersect each other
	static bool intersect(const GeoPoint & p , const GeoPoint & q, const GeoPoint & r, const GeoPoint & s);
	static inline uint32_t toIntLat(double lat) { return (uint32_t)((lat+90.0)*0xFFFFFF/180.0);}
	static inline double toDoubleLat(uint32_t lat) { return ((double)lat*180.0/0xFFFFFF-90.0);}
	static inline uint32_t toIntLon(double lon) { return (uint32_t)((lon+180.0)/360.0*0xFFFFFF);}
	static inline double toDoubleLon(uint32_t lon) { return ((double)lon*360.0/0xFFFFFF-180.0);}
	static inline GeoPoint fromIntLatLon(uint32_t lat, uint32_t lon) { return GeoPoint(lat, lon); }
};

}//end namespace spatial

template<>
struct SerializationInfo<sserialize::spatial::GeoPoint> {
	static const bool is_fixed_length = true;
	static const OffsetType length = 6;
	static const OffsetType max_length = 6;
	static const OffsetType min_length = 6;
	static inline OffsetType sizeInBytes(const sserialize::spatial::GeoPoint & value) { return 6; }
};

}//end namespace sserialize


sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPoint & point);
sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & destination, sserialize::spatial::GeoPoint & p);

inline bool operator==(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b) {
	return (a.lat() == b.lat() && a.lon() == b.lon());
}

inline bool operator!=(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b) {
	return (a.lat() != b.lat() || a.lon() != b.lon());
}

#endif