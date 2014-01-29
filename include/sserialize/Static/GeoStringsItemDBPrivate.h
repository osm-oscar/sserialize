#ifndef SSERIALIZE_STATIC_GEO_STRINGS_ITEM_DB_PRIVATE_H
#define SSERIALIZE_STATIC_GEO_STRINGS_ITEM_DB_PRIVATE_H
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/Static/Deque.h>
#include <sserialize/Static/StringsItemDBPrivate.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/Static/GeoShape.h>
#include <sserialize/Static/GeoWay.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateSimple.h>

#define SSERIALIZE_STATIC_GEO_STRINGS_ITEM_DB_VERSION 0

namespace sserialize {
namespace Static {

/** File format
  *
  *--------------------------------------------
  *VERSION|StringsItemDB|Deque<GeoShapes>
  *--------------------------------------------
  *
  *
  */

template<class MetaDataDeSerializable>
class GeoStringsItemDBPrivate: public StringsItemDBPrivate<MetaDataDeSerializable> {
protected:
	typedef sserialize::Static::StringsItemDBPrivate<MetaDataDeSerializable> MyParentClass;
private:
	Static::Deque< sserialize::Static::spatial::GeoShape > m_shapes;
public:
	GeoStringsItemDBPrivate() : MyParentClass() {}
	GeoStringsItemDBPrivate(const UByteArrayAdapter & db, const StringTable & stable) :
	MyParentClass(db+1, stable),
	m_shapes(db+(1+MyParentClass::getSizeInBytes()))
	{
		SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_GEO_STRINGS_ITEM_DB_VERSION, db.at(0), "Static::GeoStringsItemDB");
	}
	virtual ~GeoStringsItemDBPrivate() {}
	
	uint32_t getSizeInBytes() const {
		return 1 + MyParentClass::getSizeInBytes() + m_shapes.getSizeInBytes();
	}

	/** checks if any point of the item lies within boundary */
	bool match(uint32_t itemPos, const sserialize::spatial::GeoRect & boundary) const {
		if (itemPos >= MyParentClass::size())
			return false;
		return m_shapes.at(itemPos).intersects(boundary);
	}
	
	ItemIndex complete(const sserialize::spatial::GeoRect & rect) const {
		size_t size = MyParentClass::size();
		UByteArrayAdapter cache( UByteArrayAdapter::createCache(1, false) );
		ItemIndexPrivateSimpleCreator creator(0, size, size, cache);
		for(size_t i = 0; i < size; ++i) {
			if (match(i, rect))
				creator.push_back(i);
		}
		creator.flush();
		return creator.getIndex();
	}

	sserialize::spatial::GeoShapeType geoShapeType(uint32_t itemPos) const {
		return m_shapes.at(itemPos).type();
	}
	uint32_t geoPointCount(uint32_t itemPos) const {
		return m_shapes.at(itemPos).size();
	}
	sserialize::Static::spatial::GeoPoint geoPointAt(uint32_t itemPos, uint32_t pos) const {
		return dynamic_cast<sserialize::Static::spatial::GeoWay*>( m_shapes.at(itemPos).priv().get())->points().at(pos);
	}
	
	sserialize::Static::spatial::GeoShape geoShapeAt(uint32_t itemPos) const {
		return m_shapes.at(itemPos);
	}
};


}}//end namespace



#endif