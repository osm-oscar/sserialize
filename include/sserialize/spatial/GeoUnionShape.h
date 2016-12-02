#ifndef SSERIALIZE_GEO_UNION_SHAPE_H
#define SSERIALIZE_GEO_UNION_SHAPE_H
#pragma once
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/spatial/GeoShapeFactory.h>
#include <sserialize/containers/AbstractArray.h>
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {
namespace spatial {

class GeoUnionShape: public GeoShape {
public:
	typedef AbstractArray<uint32_t> ShapeIdStore;
public:
	GeoUnionShape(const ShapeIdStore & shapes, const sserialize::RCPtrWrapper<GeoShapeFactory> & factory);
	virtual ~GeoUnionShape();
public:
	virtual GeoShapeType type() const override;
	virtual uint32_t size() const override;
	///This one is not thread-safe
	virtual GeoRect boundary() const override;
	virtual void recalculateBoundary() override;
	virtual bool intersects(const GeoRect & boundary) const override;
	virtual double distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const  override;
	///This function is unsupported
	virtual UByteArrayAdapter & append(sserialize::UByteArrayAdapter & destination) const override;
	virtual GeoShape * copy() const override;
public:
	///Caller is owner!
	uint32_t shapeCount() const;
	GeoShape * shape(uint32_t pos) const;
private:
	ShapeIdStore m_shapes;
	sserialize::RCPtrWrapper<GeoShapeFactory> m_factory;
	//this is only calculated on demand
	sserialize::spatial::GeoRect m_bounds;
	//this one is mutable due to calculation in size().
	mutable uint32_t m_pointCount;
};

}}//end namespace

#endif