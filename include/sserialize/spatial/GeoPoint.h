#ifndef SSERIALIZE_SPATIAL_GEO_POINT_H
#define SSERIALIZE_SPATIAL_GEO_POINT_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/SerializationInfo.h>
#include <complex>
#include <sserialize/spatial/GeoShape.h>

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
	void normalize();
	inline const double & lat() const { return m_lat; }
	inline const double & lon() const {return m_lon;}
	///lat is betweern -90 and 90 degrees
	inline double & lat() { return m_lat; }
	///lon is between -180 and +180 degrees
	inline double & lon() { return m_lon; }
	
	
	virtual GeoShapeType type() const;
	virtual GeoRect boundary() const;
	virtual uint32_t size() const;
	virtual bool intersects(const GeoRect & boundary) const;
	virtual double distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const;
	
	virtual void recalculateBoundary();
	
	virtual UByteArrayAdapter & append(UByteArrayAdapter & destination) const;
	
	virtual sserialize::spatial::GeoShape * copy() const;
	virtual std::ostream & asString(std::ostream & out) const;
	
	///Check if two lines given by p->q and r->s intersect each other
	static bool intersect(const GeoPoint & p , const GeoPoint & q, const GeoPoint & r, const GeoPoint & s);
	static inline uint32_t toIntLat(double lat) { return (uint32_t)((lat+90.0)*0xFFFFFFFF/180.0);}
	static inline double toDoubleLat(uint32_t lat) { return ((double)lat*180.0/0xFFFFFFFF-90.0);}
	static inline uint32_t toIntLon(double lon) { return (uint32_t)((lon+180.0)/360.0*0xFFFFFFFF);}
	static inline double toDoubleLon(uint32_t lon) { return ((double)lon*360.0/0xFFFFFFFF-180.0);}
	static inline GeoPoint fromIntLatLon(uint32_t lat, uint32_t lon) { return GeoPoint(lat, lon); }
};

sserialize::UByteArrayAdapter & operator<<(::sserialize::UByteArrayAdapter & destination, const GeoPoint & point);
sserialize::UByteArrayAdapter & operator>>(::sserialize::UByteArrayAdapter & destination, GeoPoint & p);

///equality test with EPS 
inline bool operator==(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b) {
	return (std::abs<double>(a.lat() - b.lat()) < EPSILON && std::abs<double>(a.lon() - b.lon()) < EPSILON);
}

inline bool operator!=(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b) {
	return ! (a == b);
}

inline std::ostream & operator<<(std::ostream & out, const sserialize::spatial::GeoPoint & gp) {
	return gp.asString(out);
}

}//end namespace spatial

template<>
struct SerializationInfo<sserialize::spatial::GeoPoint> {
	static const bool is_fixed_length = true;
	static const OffsetType length = 8;
	static const OffsetType max_length = 8;
	static const OffsetType min_length = 8;
	static inline OffsetType sizeInBytes(const sserialize::spatial::GeoPoint & /*value*/) { return 8; }
};

}//end namespace sserialize

#endif