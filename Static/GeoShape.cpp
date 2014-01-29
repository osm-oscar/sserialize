#include <sserialize/Static/GeoShape.h>
#include <sserialize/Static/GeoWay.h>
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
		m_priv.reset( new sserialize::Static::spatial::GeoWay(data+1) );
		break;
	case sserialize::spatial::GS_POLYGON:
	case sserialize::spatial::GS_MULTI_POLYGON:
	default:
		break;
	}
}

UByteArrayAdapter::OffsetType GeoShape::getSizeInBytes() const {
	throw sserialize::UnimplementedFunctionException("sserialize::Static::GeoShape::getSizeInBytes");
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