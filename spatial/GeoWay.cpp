#include <sserialize/spatial/GeoWay.h>
#include <sserialize/Static/DenseGeoPointVector.h>

namespace sserialize {
namespace spatial {
namespace detail {

template<>
UByteArrayAdapter & GeoWay< std::vector<GeoPoint> >::append(UByteArrayAdapter & destination) const {
	destination << myBoundary();
	return sserialize::Static::spatial::DenseGeoPointVector::append(cbegin(), cend(), destination);
}

}}}//end namespace