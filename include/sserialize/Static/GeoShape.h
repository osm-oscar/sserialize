#ifndef SSERIALIZE_STATIC_GEO_SHAPE_H
#define SSERIALIZE_STATIC_GEO_SHAPE_H
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/Static/GeoPoint.h>

namespace sserialize {
namespace Static {
namespace spatial {

/** NOTE: corner-cases like point on line are not especially handeled and aer so undefined behavior */

class GeoShape {
	sserialize::spatial::GeoShapeType m_type;
	uint32_t m_size;
	UByteArrayAdapter m_data;
public:
	GeoShape() : m_type(sserialize::spatial::GS_NONE), m_size(0)  {}
	GeoShape(const UByteArrayAdapter & data);
	virtual ~GeoShape() {}
	
	inline uint32_t size() const { return m_size; }
	
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	
	inline sserialize::spatial::GeoShapeType type() const { return m_type; }
	
	GeoPoint at(uint32_t pos) const;
	sserialize::spatial::GeoRect boundary() const;
	
	bool intersects(const sserialize::spatial::GeoRect & boundary) const;
	
	static sserialize::spatial::GeoRect rectFromData(const UByteArrayAdapter &  data);
};

}}}//end namespace

sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & in, sserialize::spatial::GeoRect & out);

namespace sserialize {
template<>
sserialize::Static::spatial::GeoShape sserialize::UByteArrayAdapter::get<sserialize::Static::spatial::GeoShape>();

}//end namespace
#endif