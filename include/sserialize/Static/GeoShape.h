#ifndef SSERIALIZE_STATIC_GEO_SHAPE_H
#define SSERIALIZE_STATIC_GEO_SHAPE_H
#include <sserialize/spatial/GeoShape.h>
#include <memory>

namespace sserialize {
namespace Static {
namespace spatial {

class GeoShape {
	std::shared_ptr<sserialize::spatial::GeoShape> m_priv;
public:
	GeoShape() {}
	GeoShape(const UByteArrayAdapter & data);
	virtual ~GeoShape() {}
	inline uint32_t size() const { return m_priv->size(); }
	inline const std::shared_ptr<sserialize::spatial::GeoShape> & priv() const { return m_priv; }
	
	template<typename TGeoShapeType>
	const TGeoShapeType * get() const { return dynamic_cast<const TGeoShapeType*>(priv().get());}
	
	inline sserialize::spatial::GeoRect boundary() const { return m_priv->boundary(); }
	
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	
	inline sserialize::spatial::GeoShapeType type() const { return m_priv->type(); }
	
	bool intersects(const sserialize::spatial::GeoRect & boundary) const { return m_priv->intersects(boundary); }

};

}}}//end namespace

namespace sserialize {
template<>
sserialize::Static::spatial::GeoShape sserialize::UByteArrayAdapter::get<sserialize::Static::spatial::GeoShape>();

}//end namespace
#endif