#ifndef SSERIALIZEE_STATIC_SPATIAL_GEO_POLYGON_H
#define SSERIALIZEE_STATIC_SPATIAL_GEO_POLYGON_H
#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/Static/GeoWay.h>

namespace sserialize {
namespace spatial {
namespace detail {

template<>
GeoPolygon< sserialize::Static::spatial::DenseGeoPointVector >::GeoPolygon(const sserialize::UByteArrayAdapter & d);

template<>
GeoPolygon<sserialize::Static::spatial::DenseGeoPointVector>
GeoPolygon<sserialize::Static::spatial::DenseGeoPointVector>::fromRect(const GeoRect & rect);

//specializations for AbstractArray
template<>
GeoPolygon< sserialize::AbstractArray<sserialize::spatial::GeoPoint> >::GeoPolygon(const sserialize::UByteArrayAdapter & d);


}}}


namespace sserialize {
namespace Static {
namespace spatial {

typedef sserialize::spatial::detail::GeoPolygon< sserialize::Static::spatial::detail::GeoWayPointsContainer > GeoPolygon;

}}}//end namespace

#endif