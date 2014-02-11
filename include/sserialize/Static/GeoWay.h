#ifndef SSERIALIZE_STATIC_GEO_WAY_H
#define SSERIALIZE_STATIC_GEO_WAY_H
#include <sserialize/spatial/GeoWay.h>
#include <sserialize/Static/DenseGeoPointVector.h>

namespace sserialize {

namespace Static {
namespace spatial {
namespace detail {
	typedef sserialize::Static::spatial::DenseGeoPointVector GeoWayPointsContainer;
}}}


namespace spatial {
namespace detail {
	template<>
	GeoWay< sserialize::Static::spatial::detail::GeoWayPointsContainer >::GeoWay(const sserialize::UByteArrayAdapter & d);

	template<>
	UByteArrayAdapter & GeoWay< sserialize::Static::spatial::detail::GeoWayPointsContainer >::append(UByteArrayAdapter & destination) const;
}}}

namespace sserialize {
namespace Static {
namespace spatial {

/** Layout
  *---------------------------------
  *GeoRect|DenseGeoPointVector
  *---------------------------------
  *
  */

typedef sserialize::spatial::detail::GeoWay< sserialize::Static::spatial::detail::GeoWayPointsContainer > GeoWay;

}}


}//end namespace


namespace std {
template<>
inline void swap<sserialize::Static::spatial::GeoWay>(sserialize::Static::spatial::GeoWay & a, sserialize::Static::spatial::GeoWay & b) { a.swap(b);}
}

// namespace sserialize {
// template<>
// sserialize::Static::spatial::GeoWay sserialize::UByteArrayAdapter::get<sserialize::Static::spatial::GeoWay>();
// }//end namespace
#endif