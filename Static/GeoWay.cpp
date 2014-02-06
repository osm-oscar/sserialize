#include <sserialize/Static/GeoWay.h>


namespace sserialize {
namespace spatial {
namespace detail {

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