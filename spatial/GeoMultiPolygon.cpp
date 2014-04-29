#include <sserialize/spatial/GeoMultiPolygon.h>
#include <numeric>
#include <sserialize/Static/Array.h>
#include <sserialize/spatial/GeoRect.h>


namespace sserialize {
namespace spatial {
namespace detail {

template<>
sserialize::UByteArrayAdapter & GeoMultiPolygon< std::vector<sserialize::spatial::GeoPolygon>, sserialize::spatial::GeoPolygon >::append(sserialize::UByteArrayAdapter & destination) const {
	destination.putVlPackedUint32(m_size);
	destination << m_outerBoundary;
	destination << m_innerBoundary;
	destination << m_outerPolygons;
	destination << m_innerPolygons;
	return destination;
}

}//end namespace detail
// 
}}