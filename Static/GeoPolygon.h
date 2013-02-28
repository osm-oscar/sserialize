#ifndef SSERIALIZEE_STATIC_SPATIAL_GEO_POLYGON_H
#define SSERIALIZEE_STATIC_SPATIAL_GEO_POLYGON_H
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include "GeoPoint.h"

namespace sserialize {
namespace Static {
namespace spatial {

/** This is the static dual-class to sserialize::spatial::Polygon
  *
  * File format:
  * -------------------------------------------------------------
  *   size |GeoPoint|CachedBoundary|Value
  * --------------------------------------------------------------
  * vluin32|   *    | 2 * GeoPoint | *
  * --------------------------------------------------------------
  */

typedef double GeoPolygonCoordType;
template<typename TValue>
class GeoPolygon: public GeoShape<GeoPolygonCoordType> {
	uint32_t m_size;
	UByteArrayAdapter m_data;
public:
	GeoPolygon();
	GeoPolygon(const sserialize::UByteArrayAdapter& data);
	virtual ~GeoPolygon();
	virtual GeoRect<GeoPolygonCoordType> boundaryRect() const;
	uint32_t size() const;
	sserialize::Static::spatial::GeoPoint at(uint32_t pos) const;
	TValue value() const;
};

template<typename TValue>
GeoPolygon<TValue>::GeoPolygon() : m_size(0) {}

template<typename TValue>
GeoPolygon<TValue>::GeoPolygon(const UByteArrayAdapter & data) : m_data(data) {
	int len;
	m_size = m_data.getVlPackedUint32(0, &len);
	if (len > 0)
		m_data += len;
}

template<typename TValue>
GeoPolygon<TValue>::~GeoPolygon() {}

template<typename TValue>
uint32_t GeoPolygon<TValue>::size() const {
	return m_size;
}

template<typename TValue>
GeoPoint GeoPolygon<TValue>::at(uint32_t pos) const {
	if (pos < size())
		return GeoPoint(m_data+6*pos);
	else
		return GeoPoint();
}

template<typename TValue>
TValue GeoPolygon<TValue>::value() const {
	return TValue(m_data+6*m_size+(size() > 2 ? 12 : 0));
}

template<typename TValue>
GeoRect<GeoPolygonCoordType> GeoPolygon<TValue>::boundaryRect() const {
	switch (size()) {
	case 0:
		return GeoRect<GeoPolygonCoordType>();
	case 1:
		{
			GeoPoint p(at(0));
			return GeoRect<GeoPolygonCoordType>(p.latF(), p.latF(), p.lonF(), p.lonF());
		}
	case 2:
		{
			GeoPoint p(at(0));
			return GeoRect<GeoPolygonCoordType>(p.latF(), p.latF(), p.lonF(), p.lonF());
		}
	default:
		{
			GeoPoint p1(GeoPoint(m_data+6*size()));
			GeoPoint p2(GeoPoint(m_data+6*size()+6));
			return GeoRect<GeoPolygonCoordType>(p1.latF(), p2.latF(), p1.lonF(), p2.lonF());
		}
	}
}

}}}//end namespace

#endif