#include <sserialize/Static/GeoShape.h>
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/Static/GeoWay.h>
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/Static/GeoMultiPolygon.h>
#include <sserialize/Static/Deque.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {
namespace spatial {

GeoShape::GeoShape(UByteArrayAdapter data) {
	data.resetGetPtr();
	sserialize::spatial::GeoShapeType type = static_cast<sserialize::spatial::GeoShapeType>(data.getUint8());
	data.shrinkToGetPtr();
	switch (type) {
	case sserialize::spatial::GS_POINT:
		m_priv.reset( new sserialize::spatial::GeoPoint(data) );
		break;
	case sserialize::spatial::GS_WAY:
		{
			sserialize::spatial::GeoRect b(data);
			data += SerializationInfo<sserialize::spatial::GeoRect>::length;
			m_priv.reset( new sserialize::Static::spatial::GeoWay(b, sserialize::Static::spatial::detail::GeoWayPointsContainer(data)) );
		}
		break;
	case sserialize::spatial::GS_POLYGON:
		{
			sserialize::spatial::GeoRect b(data);
			data += SerializationInfo<sserialize::spatial::GeoRect>::length;
			m_priv.reset( new sserialize::Static::spatial::GeoPolygon(b, sserialize::Static::spatial::detail::GeoWayPointsContainer(data)) );
			break;
		}
	case sserialize::spatial::GS_MULTI_POLYGON:
		{
			uint32_t size = data.getVlPackedUint32();
			sserialize::spatial::GeoRect bo, bi;
			sserialize::Static::Deque<sserialize::Static::spatial::GeoPolygon> op, ip;
			data >> bo >> bi >> op >> ip;
			m_priv.reset( new sserialize::Static::spatial::GeoMultiPolygon(size, op, ip, bo, bi) );
			break;
		}
	default:
		throw sserialize::TypeMissMatchException("sserialize::Static::spatial::GeoShape::GeoShape()");
	}
}


UByteArrayAdapter::OffsetType GeoShape::getSizeInBytes() const {
	switch (type()) {
		case sserialize::spatial::GS_POINT:
			return 1+SerializationInfo<sserialize::spatial::GeoPoint>::length;
		case sserialize::spatial::GS_WAY:
			return 1+SerializationInfo<sserialize::spatial::GeoRect>::length + get<sserialize::Static::spatial::GeoWay>()->points().getSizeInBytes();
		case sserialize::spatial::GS_POLYGON:
			return 1+SerializationInfo<sserialize::spatial::GeoRect>::length + get<sserialize::Static::spatial::GeoPolygon>()->points().getSizeInBytes();
		case sserialize::spatial::GS_MULTI_POLYGON:
			{
				OffsetType s = 1 + 2* SerializationInfo<sserialize::spatial::GeoRect>::length;
				const sserialize::Static::spatial::GeoMultiPolygon * gmpo = get<sserialize::Static::spatial::GeoMultiPolygon>();
				s += gmpo->outerPolygons().getSizeInBytes();
				s += gmpo->innerPolygons().getSizeInBytes();
				s += psize_vu32( gmpo->size() );
				return s;
			}
		default:
			throw sserialize::UnimplementedFunctionException("sserialize::Static::GeoShape::getSizeInBytes for ");
			return 0;
			break;
	};
}

sserialize::spatial::GeoPoint GeoShape::first() const {
	switch (type()) {
		case sserialize::spatial::GS_POINT:
			return *get<sserialize::spatial::GeoPoint>();
		case sserialize::spatial::GS_WAY:
			return * (get<sserialize::spatial::GeoWay>()->points().cbegin());
		case sserialize::spatial::GS_POLYGON:
			return * (get<sserialize::spatial::GeoPolygon>()->points().cbegin());
		case sserialize::spatial::GS_MULTI_POLYGON:
		{
			const sserialize::Static::spatial::GeoMultiPolygon * gmpo = get<sserialize::Static::spatial::GeoMultiPolygon>();
			if (gmpo->size()) {
				return gmpo->outerPolygons().front().points().front();
			}
		}
		default:
			return sserialize::spatial::GeoPoint();
			break;
	};
}

}}}//end namespace

namespace sserialize {
template<>
sserialize::Static::spatial::GeoShape sserialize::UByteArrayAdapter::get<sserialize::Static::spatial::GeoShape>() {
	sserialize::Static::spatial::GeoShape shape(*this+this->tellGetPtr());
	this->incGetPtr(shape.getSizeInBytes());
	return shape;
}

}//end namespace