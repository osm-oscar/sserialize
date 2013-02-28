#ifndef SSERIALIZE_GEO_STRINGS_ITEM_H
#define SSERIALIZE_GEO_STRINGS__ITEM_H
#include <stdint.h>
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/spatial/GeoShape.h>
#include "StringsItemDBItem.h"

namespace sserialize {

template<typename DataBaseType>
class GeoStringsItem: public StringsItem<DataBaseType> {
public:
	GeoStringsItem() : StringsItem<DataBaseType>() {}
	GeoStringsItem(const uint32_t id, const DataBaseType & db) : StringsItem<DataBaseType>(id, db) {}
	virtual ~GeoStringsItem() {}
	
	/** This is only valid while the underlying DB and its content is valid */
	const spatial::GeoShape * geoShape() const {
		uint32_t id = StringsItem<DataBaseType>::id();
		return StringsItem<DataBaseType>::db().geoShapeAt(id);
	}
	
	bool match(const sserialize::spatial::GeoRect & boundary) const {
		return StringsItem<DataBaseType>::db().match(StringsItem<DataBaseType>::id(), boundary);
	}
};



}//end namespace

#endif