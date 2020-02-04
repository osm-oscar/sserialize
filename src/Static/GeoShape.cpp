#include <sserialize/Static/GeoShape.h>
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/spatial/GeoNone.h>
#include <sserialize/Static/GeoWay.h>
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/Static/GeoMultiPolygon.h>
#include <sserialize/Static/Array.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {
namespace spatial {

GeoShape::GeoShape(UByteArrayAdapter data) {
	data.resetGetPtr();
	sserialize::spatial::GeoShapeType type = static_cast<sserialize::spatial::GeoShapeType>(data.getUint8());
	data.shrinkToGetPtr();
	switch (type) {
	case sserialize::spatial::GS_NONE:
		m_priv.reset( new sserialize::spatial::GeoNone() );
		break;
	case sserialize::spatial::GS_POINT:
		m_priv.reset( new sserialize::spatial::GeoPoint(data) );
		break;
	case sserialize::spatial::GS_WAY:
		{
			m_priv.reset( new sserialize::Static::spatial::GeoWay(data) );
		}
		break;
	case sserialize::spatial::GS_POLYGON:
		{
			m_priv.reset( new sserialize::Static::spatial::GeoPolygon(data) );
			break;
		}
	case sserialize::spatial::GS_MULTI_POLYGON:
		{
			m_priv.reset( new sserialize::Static::spatial::GeoMultiPolygon(data) );
			break;
		}
	default:
		throw sserialize::TypeMissMatchException("sserialize::Static::spatial::GeoShape::GeoShape(UByteArrayAdapter) invalid type: ");
	}
}

template<typename T_POINTS_CONTAINER>
struct PointsArraySizeGetter {
	static sserialize::UByteArrayAdapter::OffsetType getSizeInBytes(const T_POINTS_CONTAINER & c);
};

template<>
sserialize::UByteArrayAdapter::OffsetType
PointsArraySizeGetter< sserialize::Static::spatial::DenseGeoPointVector >
::getSizeInBytes(const sserialize::Static::spatial::DenseGeoPointVector & c) {
	return c.getSizeInBytes();
}

template<>
sserialize::UByteArrayAdapter::OffsetType
PointsArraySizeGetter< sserialize::AbstractArray<sserialize::spatial::GeoPoint> >
::getSizeInBytes(const sserialize::AbstractArray<sserialize::spatial::GeoPoint> & c) {
	typedef sserialize::Static::spatial::detail::DenseGeoPointVectorAbstractArray MyArrayType;
	MyArrayType * tmp = c.get< MyArrayType >();
	if (tmp)
		return tmp->container().getSizeInBytes();
	return 0;
}

UByteArrayAdapter::OffsetType GeoShape::getSizeInBytes() const {
	switch (type()) {
		case sserialize::spatial::GS_POINT:
			return 1+SerializationInfo<sserialize::spatial::GeoPoint>::length;
		case sserialize::spatial::GS_WAY:
			return 1+SerializationInfo<sserialize::spatial::GeoRect>::length +
					PointsArraySizeGetter<sserialize::Static::spatial::GeoPolygon::PointsContainer>::getSizeInBytes( get<sserialize::spatial::GS_WAY>()->points());
		case sserialize::spatial::GS_POLYGON:
			return 1+SerializationInfo<sserialize::spatial::GeoRect>::length +
					PointsArraySizeGetter<sserialize::Static::spatial::GeoPolygon::PointsContainer>::getSizeInBytes( get<sserialize::spatial::GS_POLYGON>()->points());
		case sserialize::spatial::GS_MULTI_POLYGON:
			{
				OffsetType s = 1 + 2* SerializationInfo<sserialize::spatial::GeoRect>::length;
				auto gmpo = get<sserialize::spatial::GS_MULTI_POLYGON>();
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
			return *get<sserialize::spatial::GS_POINT>();
		case sserialize::spatial::GS_WAY:
			return * (get<sserialize::spatial::GS_WAY>()->points().cbegin());
		case sserialize::spatial::GS_POLYGON:
			return * (get<sserialize::spatial::GS_POLYGON>()->points().cbegin());
		case sserialize::spatial::GS_MULTI_POLYGON:
		{
			auto gmpo = get<sserialize::spatial::GS_MULTI_POLYGON>();
			if (gmpo->size()) {
				return gmpo->outerPolygons().front().points().front();
			}
			return sserialize::spatial::GeoPoint();
		}
		default:
			return sserialize::spatial::GeoPoint();
			break;
	};
}

sserialize::spatial::GeoPoint GeoShape::at(uint32_t pos) const {
	switch (type()) {
		case sserialize::spatial::GS_POINT:
			return *get<sserialize::spatial::GS_POINT>();
		case sserialize::spatial::GS_WAY:
			return get<sserialize::spatial::GS_WAY>()->points().at(pos);
		case sserialize::spatial::GS_POLYGON:
			return get<sserialize::spatial::GS_POLYGON>()->points().at(pos);
		case sserialize::spatial::GS_MULTI_POLYGON:
			return get<sserialize::spatial::GS_MULTI_POLYGON>()->at(pos);
		default:
			return sserialize::spatial::GeoPoint();
	};
}

bool GeoShape::intersects(const GeoShape & other) const {
	if (type() == sserialize::spatial::GS_POINT) {
		if (other.type() == sserialize::spatial::GS_POINT) {
			return this->first().equal(other.first(), 0);
		}
		else {
			return this->get<sserialize::spatial::GS_REGION>()->contains(other.first());
		}
	}
	else if (other.type() == sserialize::spatial::GS_POINT) {
		//we know that this->type() != GS_POINT
		return other.get<sserialize::spatial::GS_REGION>()->contains(other.first());
	}
	else { //both are regions
		return this->get<sserialize::spatial::GS_REGION>()->intersects(* other.get<sserialize::spatial::GS_REGION>() );
	}
	return false;
}


void GeoShape::doVisitPoints(PointVisitor & pv) const {
	switch (type()) {
		case sserialize::spatial::GS_POINT:
			pv.visit(*get<sserialize::spatial::GS_POINT>());
			break;
		case sserialize::spatial::GS_WAY:
			{
				auto shape = this->get<sserialize::spatial::GS_WAY>();
				for(auto p : shape->points()) {
					pv.visit(p);
				}
			}
			break;
		case sserialize::spatial::GS_POLYGON:
			{
				auto shape = get<sserialize::spatial::GS_POLYGON>();
				for(auto p : shape->points()) {
					pv.visit(p);
				}
			}
			break;
		case sserialize::spatial::GS_MULTI_POLYGON:
			{
				auto shape = get<sserialize::spatial::GS_MULTI_POLYGON>();
				for(auto poly : shape->outerPolygons()) {
					for(auto p : poly.points()) {
						pv.visit(p);
					}
				}
				for(auto poly : shape->innerPolygons()) {
					for(auto p : poly.points()) {
						pv.visit(p);
					}
				}
			}
			break;
		default:
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
