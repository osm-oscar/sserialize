#ifndef SSERIALIZE_SPATIAL_GEO_SHAPE_FACTORY_H
#define SSERIALIZE_SPATIAL_GEO_SHAPE_FACTORY_H
#pragma once
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/types.h>
#include <sserialize/spatial/GeoShape.h>

namespace sserialize {
namespace spatial {

class GeoShapeFactory: public RefCountObject {
public:
	typedef sserialize::SizeType SizeType;
public:
	GeoShapeFactory() {}
	virtual ~GeoShapeFactory() = 0;
public:
	virtual SizeType size() const = 0;
	///caller is owner!
	virtual GeoShape * shape(SizeType pos) const = 0;
};

}}

#endif