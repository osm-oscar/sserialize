#include <sserialize/Static/GeoMultiPolygon.h>

namespace sserialize {
namespace spatial {

namespace detail {

template<>
sserialize::UByteArrayAdapter &
GeoMultiPolygon< sserialize::Static::Deque<sserialize::Static::spatial::GeoPolygon>, sserialize::Static::spatial::GeoPolygon >::append(sserialize::UByteArrayAdapter & destination) const {
	throw sserialize::UnimplementedFunctionException("sserialize::Static::spatial::GeoMultiPolygon::append");
}

}}}