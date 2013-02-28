#ifndef SSERIALIZE_STATIC_GEO_STRINGS_ITEM_DB_ITEM_H
#define SSERIALIZE_STATIC_GEO_STRINGS_ITEM_DB_ITEM_H
#include "StringsItemDBItem.h"

namespace sserialize {
namespace Static {

template<typename DataBaseType, typename MetaDataDeSerializable>
class GeoStringsItemDBItem: public StringsItemDBItem<DataBaseType, MetaDataDeSerializable> {
public:
	typedef StringsItemDBItem<DataBaseType, MetaDataDeSerializable> MyBaseClass;
public:
	GeoStringsItemDBItem() : StringsItemDBItem<DataBaseType, MetaDataDeSerializable>() {}
	GeoStringsItemDBItem(uint32_t id, const DataBaseType & db)  : StringsItemDBItem<DataBaseType, MetaDataDeSerializable>(id, db) {}

	bool match(const sserialize::spatial::GeoRect & boundary) const {
		return MyBaseClass::db().match(MyBaseClass::id(), boundary);
	}
	
	sserialize::spatial::GeoShapeType geoShapeType() const {
		return MyBaseClass::db().geoShapeType(MyBaseClass::id());
	}
	
	sserialize::Static::spatial::GeoShape geoShape() const {
		return MyBaseClass::db().geoShape(MyBaseClass::id());
	}
	
	uint32_t geoPointCount() const {
		return MyBaseClass::db().geoPointCount(MyBaseClass::id());
	}
	
	sserialize::Static::spatial::GeoPoint geoPointAt(uint32_t pos) const {
		return MyBaseClass::db().geoPointAt(MyBaseClass::id(), pos);
	}
};

}}//end namespace


#endif