#include <sserialize/Static/GeoMultiPolygon.h>

namespace sserialize {
namespace spatial {

namespace detail {


template<>
GeoMultiPolygon< sserialize::Static::Deque<sserialize::Static::spatial::GeoPolygon>, sserialize::Static::spatial::GeoPolygon >::GeoMultiPolygon(const sserialize::UByteArrayAdapter & d) {
	UByteArrayAdapter data(d);
	data.resetGetPtr();
	m_size = data.getVlPackedUint32();
	data >> m_outerBoundary >> m_innerBoundary >> m_outerPolygons >> m_innerPolygons;
}


template<>
sserialize::UByteArrayAdapter &
GeoMultiPolygon< sserialize::Static::Deque<sserialize::Static::spatial::GeoPolygon>, sserialize::Static::spatial::GeoPolygon >::append(sserialize::UByteArrayAdapter & /*destination*/) const {
	throw sserialize::UnimplementedFunctionException("sserialize::Static::spatial::GeoMultiPolygon::append");
}

}}}