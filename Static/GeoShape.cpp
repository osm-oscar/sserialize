#include <sserialize/Static/GeoShape.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {
namespace spatial {

GeoShape::GeoShape(const UByteArrayAdapter & data) : m_type(static_cast<sserialize::spatial::GeoShapeType>(data.at(0)) ), m_size(0), m_data(data) {
	m_data += 1;
	if (m_type == sserialize::spatial::GS_WAY || m_type == sserialize::spatial::GS_POLYGON) {
		int len;
		m_size = m_data.getVlPackedUint32(0, &len);
		if (len > 0)
			m_data += len;
		else
			m_type = sserialize::spatial::GS_NONE;
	}
	else if (m_type == sserialize::spatial::GS_POINT) {
		m_size = 1;
	}
}

UByteArrayAdapter::OffsetType GeoShape::getSizeInBytes() const {
	if (m_type == sserialize::spatial::GS_WAY || sserialize::spatial::GS_POLYGON) {
		return 1 +  psize_vu32(m_size) +  SerializationInfo<sserialize::spatial::GeoRect>::length + SerializationInfo<sserialize::Static::spatial::GeoPoint>::length*m_size;
	}
	else if (m_data.size() || m_type == sserialize::spatial::GS_POINT) {
		return 1;
	}
	return 0;
}
	
	
sserialize::Static::spatial::GeoPoint GeoShape::at(uint32_t pos) const {
	if (type() == sserialize::spatial::GS_POINT)
		return sserialize::Static::spatial::GeoPoint(m_data);
	else
		return sserialize::Static::spatial::GeoPoint(m_data + (sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length*(2+pos)));
}
	
sserialize::spatial::GeoRect GeoShape::boundary() const {
	if (m_type == sserialize::spatial::GS_POINT) {
		sserialize::Static::spatial::GeoPoint p(at(0));
		return sserialize::spatial::GeoRect(p.latD(), p.latD(), p.lonD(), p.lonD());
	}
	else {
		sserialize::Static::spatial::GeoPoint bL(m_data);
		sserialize::Static::spatial::GeoPoint tR(m_data+sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length);
		return sserialize::spatial::GeoRect(bL.latD(), tR.latD(), bL.lonD(), tR.lonD());
	}
}
	
bool GeoShape::intersects(const sserialize::spatial::GeoRect & boundary) const {
	if (type() == sserialize::spatial::GS_POINT) {
		sserialize::Static::spatial::GeoPoint p(at(0));
		return boundary.contains(p.latF(), p.lonF());
	}
	
	if (!boundary.overlap( this->boundary()) )
		return false;
	uint32_t s = size();
	UByteArrayAdapter tmp(m_data+2*sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length); tmp.resetGetPtr();
	sserialize::Static::spatial::GeoPoint p;
	for(size_t i = 0; i < s; ++i) {
		tmp >> p;
		if (boundary.contains(p.latF(), p.lonF()))
			return true;
	}
	return false;
}
	
sserialize::spatial::GeoRect GeoShape::rectFromData(const UByteArrayAdapter &  data) {
	sserialize::Static::spatial::GeoPoint bL(data);
	sserialize::Static::spatial::GeoPoint tR(data+sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length);
	return sserialize::spatial::GeoRect(bL.latD(), tR.latD(), bL.lonD(), tR.lonD());
}


UByteArrayAdapter & GeoShape::append(sserialize::UByteArrayAdapter & destination) const {
	throw sserialize::UnimplementedFunctionException("GeoShape::appendWithTypeInfo");
	return destination;
}

sserialize::spatial::GeoShape * GeoShape::copy() const {
	return new sserialize::Static::spatial::GeoShape(*this);
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