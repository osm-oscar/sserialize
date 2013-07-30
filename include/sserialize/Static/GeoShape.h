#ifndef SSERIALIZE_STATIC_GEO_SHAPE_H
#define SSERIALIZE_STATIC_GEO_SHAPE_H
#include <sserialize/spatial/GeoShape.h>
#include "GeoPoint.h"

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
	GeoShape(const UByteArrayAdapter & data) : m_type(static_cast<sserialize::spatial::GeoShapeType>(data.at(0)) ), m_size(0), m_data(data) {
		m_data += 1;
		if (m_type == sserialize::spatial::GS_WAY || m_type == sserialize::spatial::GS_POLYGON) {
			int len;
			m_size = m_data.getVlPackedUint32(0, &len);
			if (len > 0)
				m_data += len;
			else
				m_type = sserialize::spatial::GS_NONE;
		}
		else if (m_type == sserialize::spatial::GS_POINT) {
			m_size = 1;
		}
	}
	virtual ~GeoShape() {}
	
	uint32_t size() const {
		return m_size;
	
	}
	sserialize::spatial::GeoShapeType type() const {
		return m_type;
	}
	
	GeoPoint at(uint32_t pos) const {
		if (type() == sserialize::spatial::GS_POINT)
			return GeoPoint(m_data);
		else
			return GeoPoint(m_data + (sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length*(2+pos)));
	}
	
	sserialize::spatial::GeoRect boundary() const {
		if (m_type == sserialize::spatial::GS_POINT) {
			GeoPoint p(at(0));
			return sserialize::spatial::GeoRect(p.latD(), p.latD(), p.lonD(), p.lonD());
		}
		else {
			GeoPoint bL(m_data);
			GeoPoint tR(m_data+sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length);
			return sserialize::spatial::GeoRect(bL.latD(), tR.latD(), bL.lonD(), tR.lonD());
		}
	}
	
	bool intersects(const sserialize::spatial::GeoRect & boundary) const {
		if (type() == sserialize::spatial::GS_POINT) {
			GeoPoint p(at(0));
			return boundary.contains(p.latF(), p.lonF());
		}
		
		if (!boundary.overlap( this->boundary()) )
			return false;
		uint32_t s = size();
		UByteArrayAdapter tmp(m_data+2*sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length); tmp.resetGetPtr();
		GeoPoint p;
		for(size_t i = 0; i < s; ++i) {
			tmp >> p;
			if (boundary.contains(p.latF(), p.lonF()))
				return true;
		}
		return false;
	}
	
	inline static sserialize::spatial::GeoRect rectFromData(const UByteArrayAdapter &  data) {
		GeoPoint bL(data);
		GeoPoint tR(data+sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length);
		return sserialize::spatial::GeoRect(bL.latD(), tR.latD(), bL.lonD(), tR.lonD());
	}

};

}}}//end namespace

inline sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & in, sserialize::spatial::GeoRect & out) {
	sserialize::Static::spatial::GeoPoint bL(in);
	sserialize::Static::spatial::GeoPoint tR(in+sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length);
	out = sserialize::spatial::GeoRect(bL.latD(), tR.latD(), bL.lonD(), tR.lonD());
	in.incGetPtr(2*sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length);
	return in;
}

#endif