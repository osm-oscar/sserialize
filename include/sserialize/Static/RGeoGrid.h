#ifndef SSERIALIZE_STATIC_RGEO_GRID_H
#define SSERIALIZE_STATIC_RGEO_GRID_H
#include <sserialize/utility/exceptions.h>
#include <sserialize/spatial/RWGeoGrid.h>
#include <sserialize/Static/Array.h>

namespace sserialize {
namespace Static{
namespace spatial {

/**
  * Layout:
  * { 
  *  BaseClass   sserialize::spatial::GeoGrid
  *  Values      sserialize::Static::Array<TValue>
  * }
  *
  */


template<typename TValue>
class RGeoGrid: public sserialize::spatial::RGeoGrid<TValue, sserialize::Static::Array<TValue> > {
public:
	typedef sserialize::spatial::RGeoGrid<TValue, sserialize::Static::Array<TValue> > MyParentClass;
public:
	RGeoGrid() : MyParentClass() {}
	RGeoGrid(const UByteArrayAdapter & d) : MyParentClass(sserialize::spatial::GeoGrid(d)) {
		sserialize::UByteArrayAdapter::OffsetType baseGridSize = sserialize::SerializationInfo<sserialize::spatial::GeoGrid>::sizeInBytes(*this);
		MyParentClass::storage() = sserialize::Static::Array<TValue>(d+baseGridSize);
		SSERIALIZE_EQUAL_LENGTH_CHECK(MyParentClass::MyParentClass::tileCount(), MyParentClass::storage().size(), "sserialize::Static::spatial::RGeoGrid");
	}
	uint32_t getSizeInBytes() const {
		return MyParentClass::storage().getSizeInBytes() + sserialize::SerializationInfo<sserialize::spatial::GeoGrid>::sizeInBytes(*this);
	}
};


}}}//end namespace




#endif