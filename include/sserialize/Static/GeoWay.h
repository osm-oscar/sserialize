#ifndef SSERIALIZE_STATIC_GEO_WAY_H
#define SSERIALIZE_STATIC_GEO_WAY_H
#include <sserialize/spatial/GeoWay.h>
#include <sserialize/Static/Deque.h>

namespace sserialize {
namespace spatial {
namespace detail {
	template<>
	UByteArrayAdapter & GeoWay< sserialize::Static::Deque< sserialize::spatial::GeoPoint > >::append(UByteArrayAdapter & destination) const;
}}}

namespace sserialize {
namespace Static {
namespace spatial {

typedef sserialize::spatial::detail::GeoWay< sserialize::Static::Deque< sserialize::spatial::GeoPoint > > GeoWay;

}}


}//end namespace


namespace std {
template<>
inline void swap<sserialize::Static::spatial::GeoWay>(sserialize::Static::spatial::GeoWay & a, sserialize::Static::spatial::GeoWay & b) { a.swap(b);}
}

namespace sserialize {
template<>
sserialize::Static::spatial::GeoWay sserialize::UByteArrayAdapter::get<sserialize::Static::spatial::GeoWay>();

}//end namespace
#endif