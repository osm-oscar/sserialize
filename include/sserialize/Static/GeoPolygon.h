#ifndef SSERIALIZEE_STATIC_SPATIAL_GEO_POLYGON_H
#define SSERIALIZEE_STATIC_SPATIAL_GEO_POLYGON_H
#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/Static/GeoWay.h>

namespace sserialize {
namespace spatial {
namespace detail {

}}}


namespace sserialize {
namespace Static {
namespace spatial {

typedef sserialize::spatial::detail::GeoPolygon< sserialize::Static::Deque< sserialize::spatial::GeoPoint >,  sserialize::spatial::GeoPoint> GeoPolygon;

}}}//end namespace

#endif