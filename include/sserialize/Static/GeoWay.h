#ifndef SSERIALIZE_STATIC_GEO_WAY_H
#define SSERIALIZE_STATIC_GEO_WAY_H
#include <sserialize/spatial/GeoWay.h>
#include <sserialize/Static/DenseGeoPointVector.h>

namespace sserialize {

namespace Static {
namespace spatial {
namespace detail {
	///The type of the on-disk PointsContainer
	typedef sserialize::Static::spatial::DenseGeoPointVector GeoWayPointsContainer;
	typedef sserialize::Static::spatial::detail::DenseGeoPointVectorAbstractArray GeoWayAbstractArrayPointsContainer;
}}}


namespace spatial {
namespace detail {
	template<>
	GeoWay< sserialize::Static::spatial::DenseGeoPointVector >::GeoWay(const sserialize::UByteArrayAdapter & d);
	
	template<>
	GeoWay< sserialize::AbstractArray<sserialize::spatial::GeoPoint> >::GeoWay(const sserialize::UByteArrayAdapter & d);

	template<>
	UByteArrayAdapter & GeoWay< sserialize::Static::spatial::DenseGeoPointVector >::append(UByteArrayAdapter & destination) const;
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

// typedef sserialize::spatial::detail::GeoWay< sserialize::Static::spatial::detail::GeoWayPointsContainer > GeoWay;
typedef sserialize::spatial::detail::GeoWay< sserialize::AbstractArray<sserialize::spatial::GeoPoint> > GeoWay;

}}


}//end namespace

inline void swap(sserialize::Static::spatial::GeoWay & a, sserialize::Static::spatial::GeoWay & b) { a.swap(b);}
#endif