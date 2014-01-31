#include <sserialize/Static/GeoShape.h>
#include <sserialize/Static/GeoWay.h>
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {
namespace spatial {

GeoShape::GeoShape(const UByteArrayAdapter & data) {
	sserialize::spatial::GeoShapeType type = static_cast<sserialize::spatial::GeoShapeType>(data.at(0));
	switch (type) {
	case sserialize::spatial::GS_POINT:
		m_priv.reset( new sserialize::spatial::GeoPoint(data+1) );
		break;
	case sserialize::spatial::GS_WAY:
		m_priv.reset( new sserialize::Static::spatial::GeoWay( sserialize::Static::Deque<sserialize::spatial::GeoPoint>(data+1) ) );
		break;
	case sserialize::spatial::GS_POLYGON:
		m_priv.reset( new sserialize::Static::spatial::GeoPolygon( sserialize::Static::Deque<sserialize::spatial::GeoPoint>(data+1)) );
		break;
	case sserialize::spatial::GS_MULTI_POLYGON:
	default:
		break;
	}
}

UByteArrayAdapter::OffsetType GeoShape::getSizeInBytes() const {
	throw sserialize::UnimplementedFunctionException("sserialize::Static::GeoShape::getSizeInBytes");
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