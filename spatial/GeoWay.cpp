#include <sserialize/spatial/GeoWay.h>

namespace sserialize {
namespace spatial {
namespace detail {

template<>
UByteArrayAdapter & GeoWay< std::vector<GeoPoint> >::append(UByteArrayAdapter & destination) const {
	destination.putVlPackedUint32(points().size());
	destination << myBoundary();
	for(std::size_t i(0), s(points().size()); i < s; ++i) {
		destination << points().at(i);
	}
	return destination;
}

}}}//end namespace