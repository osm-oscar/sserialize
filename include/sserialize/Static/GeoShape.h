#ifndef SSERIALIZE_STATIC_GEO_SHAPE_H
#define SSERIALIZE_STATIC_GEO_SHAPE_H
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/Static/GeoWay.h>
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/Static/GeoMultiPolygon.h>
#include <memory>

namespace sserialize {
namespace Static {
namespace spatial {
namespace detail {
	template<sserialize::spatial::GeoShapeType GST>
	struct GeoShapeFromType;
	
	template<>
	struct GeoShapeFromType<sserialize::spatial::GS_POINT> {
		using type = sserialize::Static::spatial::GeoPoint;
	};
	
	template<>
	struct GeoShapeFromType<sserialize::spatial::GS_WAY> {
		using type = sserialize::Static::spatial::GeoWay;
	};
	
	template<>
	struct GeoShapeFromType<sserialize::spatial::GS_POLYGON> {
		using type = sserialize::Static::spatial::GeoPolygon;
	};
	
	template<>
	struct GeoShapeFromType<sserialize::spatial::GS_MULTI_POLYGON> {
		using type = sserialize::Static::spatial::GeoMultiPolygon;
	};
	
	template<>
	struct GeoShapeFromType<sserialize::spatial::GS_SHAPE> {
		using type = sserialize::spatial::GeoShape;
	};
	
	template<>
	struct GeoShapeFromType<sserialize::spatial::GS_REGION> {
		using type = sserialize::spatial::GeoRegion;
	};
	
}

class GeoShape {
	std::shared_ptr<sserialize::spatial::GeoShape> m_priv;
public:
	GeoShape() : m_priv(0) {}
	GeoShape(sserialize::UByteArrayAdapter data);
	virtual ~GeoShape() {}
	inline bool valid() const { return m_priv.get() && type() != sserialize::spatial::GS_INVALID;}
	inline uint32_t size() const { return m_priv->size(); }
	
	template<sserialize::spatial::GeoShapeType GST = sserialize::spatial::GS_SHAPE>
	auto get() const {
		using return_type = typename detail::GeoShapeFromType<GST>::type;
		return std::dynamic_pointer_cast<return_type>(priv());
	}
	
	template<typename TVisitor>
	void visitPoints(TVisitor pv) const;
	
	sserialize::spatial::GeoPoint first() const;
	sserialize::spatial::GeoPoint at(uint32_t pos) const;
	
	inline sserialize::spatial::GeoRect boundary() const { return m_priv->boundary(); }
	
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	
	inline sserialize::spatial::GeoShapeType type() const { return m_priv->type(); }
	
	bool intersects(const sserialize::spatial::GeoRect & boundary) const { return m_priv->intersects(boundary); }
	
	bool intersects(const GeoShape & other) const;
protected:
	inline const std::shared_ptr<sserialize::spatial::GeoShape> & priv() const { return m_priv; }
private:
	struct PointVisitor {
		virtual void visit(const sserialize::spatial::GeoPoint & point) = 0;
	};
	template<typename T>
	struct TPointVisitor: PointVisitor {
		TPointVisitor(T * t) : t(t) {}
		virtual void visit(const sserialize::spatial::GeoPoint & point) override {
			(*t)(point);
		}
		T * t;
	};
private:
	void doVisitPoints(PointVisitor & pv) const;
};


template<typename TVisitor>
void GeoShape::visitPoints(TVisitor pv) const {
	TPointVisitor<TVisitor> tpv(&pv);
	doVisitPoints(tpv);
}

}}}//end namespace

namespace sserialize {
template<>
sserialize::Static::spatial::GeoShape sserialize::UByteArrayAdapter::get<sserialize::Static::spatial::GeoShape>();

}//end namespace
#endif
