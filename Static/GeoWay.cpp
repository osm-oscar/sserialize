#include <sserialize/Static/GeoWay.h>


namespace sserialize {
namespace spatial {
namespace detail {

template<>
GeoWay< sserialize::Static::spatial::detail::GeoWayPointsContainer >::GeoWay(const sserialize::UByteArrayAdapter & d) :
m_boundary(d),
m_points(d+SerializationInfo<sserialize::spatial::GeoRect>::length)
{}

template<>
UByteArrayAdapter & GeoWay< sserialize::Static::spatial::DenseGeoPointVector >::append(UByteArrayAdapter & destination) const {
	destination << myBoundary();
	return sserialize::Static::spatial::DenseGeoPointVector::append(cbegin(), cend(), destination);
// 	destination.putVlPackedUint32(points().size());
// 	destination << myBoundary();
// 	for(const_iterator) {
// 		destination << points().at(i);
// 	}
// 	return destination;
}

}}}//end namespace