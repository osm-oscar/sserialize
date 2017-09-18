#ifndef SSERIALIZE_GEO_SHAPE_NONE_H
#define SSERIALIZE_GEO_SHAPE_NONE_H
#include <sserialize/spatial/GeoShape.h>

namespace sserialize {
namespace spatial {

class GeoNone: public GeoShape {
public:
	GeoNone();
	virtual ~GeoNone();
	virtual GeoShapeType type() const override;
	virtual uint32_t size() const  override;
	virtual GeoRect boundary() const  override;
	virtual void recalculateBoundary() override;
	virtual bool intersects(const GeoRect & boundary) const  override;
	virtual double distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const override;
	virtual UByteArrayAdapter & append(sserialize::UByteArrayAdapter & destination) const override;
	virtual GeoShape * copy() const override;
	virtual std::ostream & asString(std::ostream & out) const override;
};

}} //end namespace sserialize::spatial
#endif