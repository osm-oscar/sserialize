#ifndef SSERIALIZE_SPATIAL_GEO_POINT_H
#define SSERIALIZE_SPATIAL_GEO_POINT_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/storage/SerializationInfo.h>
#include <complex>
#include <sserialize/spatial/GeoShape.h>

namespace sserialize {
namespace spatial {


class GeoPoint: public GeoShape {
public:
	typedef enum {NT_CLIP, NT_WRAP} NormalizationType;
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
	
	operator std::pair<double, double>() const { return std::pair<double, double>(m_lat, m_lon);}
	
	bool valid() const;
	void normalize(NormalizationType nt = NT_WRAP);
	///snap point to the precision provided by the serialization
	void snap();
	bool isSnapped() const;
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
	static uint32_t toIntLat(double lat);
	static double toDoubleLat(uint32_t lat);
	static uint32_t toIntLon(double lon);
	static double toDoubleLon(uint32_t lon);
	static GeoPoint fromIntLatLon(uint32_t lat, uint32_t lon);
	
	///set acc to 0 to get usualy equality
	static bool equal(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b, double acc = EPSILON);
	
	bool equal(const sserialize::spatial::GeoPoint & b, double acc = EPSILON) const;
	
	bool operator==(const GeoPoint & other) const = delete;
};

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const GeoPoint & point);
sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & destination, GeoPoint & p);

bool equal(const sserialize::spatial::GeoPoint & a, const sserialize::spatial::GeoPoint & b, double acc = EPSILON);

std::ostream & operator<<(std::ostream & out, const sserialize::spatial::GeoPoint & gp);

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