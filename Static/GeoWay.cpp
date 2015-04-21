#include <sserialize/Static/GeoWay.h>


namespace sserialize {
namespace spatial {
namespace detail {

template<>
GeoWay< sserialize::Static::spatial::DenseGeoPointVector >::GeoWay(const sserialize::UByteArrayAdapter & d) :
m_boundary(d),
m_points(d+SerializationInfo<sserialize::spatial::GeoRect>::length)
{}

template<>
GeoWay< sserialize::AbstractArray<sserialize::spatial::GeoPoint> >::GeoWay(const sserialize::UByteArrayAdapter & d) :
m_boundary(d),
m_points( sserialize::AbstractArray<sserialize::spatial::GeoPoint>(
				new sserialize::Static::spatial::detail::GeoWayAbstractArrayPointsContainer(
					d+SerializationInfo<sserialize::spatial::GeoRect>::length
				)
			)
)
{}


}}}//end namespace