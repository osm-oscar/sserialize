#ifndef SSERIALIZE_STATIC_SPATIAL_GEO_MULTI_POLYGON_H
#define SSERIALIZE_STATIC_SPATIAL_GEO_MULTI_POLYGON_H
#include <sserialize/spatial/GeoMultiPolygon.h>
#include <sserialize/Static/Deque.h>
#include <sserialize/Static/GeoPolygon.h>

namespace sserialize {
namespace spatial {

namespace detail {

template<>
sserialize::UByteArrayAdapter &
GeoMultiPolygon< sserialize::Static::Deque<sserialize::Static::spatial::GeoPolygon>, sserialize::Static::spatial::GeoPolygon >::append(sserialize::UByteArrayAdapter & destination) const;

}}

namespace Static {
namespace spatial {

typedef sserialize::spatial::detail::GeoMultiPolygon< sserialize::Static::Deque<sserialize::Static::spatial::GeoPolygon>, sserialize::Static::spatial::GeoPolygon > GeoMultiPolygon;

}} //end namespace Static::spatial 

}//end namespace sserialize

#endif