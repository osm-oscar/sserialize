#ifndef SSERIALIZE_STATIC_GEO_SHAPE_H
#define SSERIALIZE_STATIC_GEO_SHAPE_H
#include <sserialize/spatial/GeoPoint.h>
#include <memory>

namespace sserialize {
namespace Static {
namespace spatial {

class GeoShape {
	std::shared_ptr<sserialize::spatial::GeoShape> m_priv;
public:
	GeoShape() : m_priv(0) {}
	GeoShape(sserialize::UByteArrayAdapter data);
	virtual ~GeoShape() {}
	inline bool valid() const { return m_priv.get() && type() != sserialize::spatial::GS_INVALID;}
	inline uint32_t size() const { return m_priv->size(); }
	
	template<typename TGeoShapeType = sserialize::spatial::GeoShape>
	std::shared_ptr<TGeoShapeType> get() const { return std::dynamic_pointer_cast<TGeoShapeType>(priv());}
	
	sserialize::spatial::GeoPoint first() const;
	sserialize::spatial::GeoPoint at(uint32_t pos) const;
	
	inline sserialize::spatial::GeoRect boundary() const { return m_priv->boundary(); }
	
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	
	inline sserialize::spatial::GeoShapeType type() const { return m_priv->type(); }
	
	bool intersects(const sserialize::spatial::GeoRect & boundary) const { return m_priv->intersects(boundary); }
	
	bool intersects(const GeoShape & other) const;
	
protected:
	inline const std::shared_ptr<sserialize::spatial::GeoShape> & priv() const { return m_priv; }

};

}}}//end namespace

namespace sserialize {
template<>
sserialize::Static::spatial::GeoShape sserialize::UByteArrayAdapter::get<sserialize::Static::spatial::GeoShape>();

}//end namespace
#endif
