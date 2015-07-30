#ifndef SSERIALIZE_STATIC_GEO_STRINGS_ITEM_DB_H
#define SSERIALIZE_STATIC_GEO_STRINGS_ITEM_DB_H
#include <sserialize/search/ItemIndexIteratorGeoDB.h>
#include "GeoStringsItemDBPrivate.h"
#include "StringsItemDB.h"
#include "GeoStringsItemDBItem.h"

namespace sserialize {
namespace Static {

template<class MetaDataDeSerializable>
class GeoStringsItemDB: public StringsItemDB<MetaDataDeSerializable> {
public:
	typedef sserialize::Static::GeoStringsItemDBItem<GeoStringsItemDB, MetaDataDeSerializable> Item;
protected:
	typedef sserialize::Static::GeoStringsItemDBPrivate<MetaDataDeSerializable> MyPrivateClass;
	typedef sserialize::Static::StringsItemDB<MetaDataDeSerializable> MyParentClass;
	MyPrivateClass * priv() const {
		return static_cast<MyPrivateClass*>( MyParentClass::priv() );
	}
	GeoStringsItemDB(MyPrivateClass * priv) : MyParentClass( priv ) {}
public:
	GeoStringsItemDB() : MyParentClass( new MyPrivateClass() ) {}
	GeoStringsItemDB(const UByteArrayAdapter & db, const StringTable & stable) :
	MyParentClass( new MyPrivateClass(db, stable) ) {}
	GeoStringsItemDB(const GeoStringsItemDB & other) : MyParentClass(other) {}
	virtual ~GeoStringsItemDB() {}
	GeoStringsItemDB & operator=(const GeoStringsItemDB & other) {
		MyParentClass::operator=(other);
		return *this;
	}
	
	uint32_t getSizeInBytes() const {
		return priv()->getSizeInBytes();
	}
	
	Item at(uint32_t itemPos) const {
		if (itemPos >= MyParentClass::size())
			return Item();
		return Item(itemPos, *this);
	}
	
	/** checks if any point of the item lies within boundary */
	bool match(uint32_t itemPos, const sserialize::spatial::GeoRect & boundary) const {
		return priv()->match(itemPos, boundary);
	}
	
	using MyParentClass::match; //import match(*) functions from base, as those are hidden by the one above
	
	ItemIndex complete(const sserialize::spatial::GeoRect & rect, bool) const {
		return priv()->complete(rect);
	}
	
	
	using MyParentClass::complete; //import complete(*) functions from base, as those are hidden by the one above
	
	ItemIndexIterator partialComplete(const sserialize::spatial::GeoRect & rect, bool) const {
		ItemIndexIteratorPrivate * rangeIt = new ItemIndexIteratorPrivateFixedRange(0, MyParentClass::size());
		ItemIndexIteratorPrivate * dbIt = new ItemIndexIteratorGeoDB< GeoStringsItemDB<MetaDataDeSerializable> >(*this, rect, ItemIndexIterator(rangeIt));
		return ItemIndexIterator( dbIt );
	}

	using MyParentClass::partialComplete; //import partialComplete(*) functions from base, as those are hidden by the one above
	
	ItemIndex filter(const sserialize::spatial::GeoRect & rect, bool /*approximate*/, const ItemIndex & partner) const {
		if (!partner.size())
			return partner;
		UByteArrayAdapter cache( UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY) );
		ItemIndexPrivateSimpleCreator creator(partner.front(), partner.back(), partner.size(), cache);
		for(size_t i = 0; i < partner.size(); i++) {
			uint32_t itemId = partner.at(i);
			if (match(itemId, rect))
				creator.push_back(itemId);
		}
		creator.flush();
		return creator.getIndex();
	}
	
	ItemIndexIterator filter(const sserialize::spatial::GeoRect & rect, bool /*approximate*/, const ItemIndexIterator & partner) const {
		ItemIndexIteratorGeoDB< GeoStringsItemDB<MetaDataDeSerializable> > * dbIt = new ItemIndexIteratorGeoDB< GeoStringsItemDB<MetaDataDeSerializable> >(*this, rect, partner);
		return ItemIndexIterator( dbIt );
	}

	
	sserialize::spatial::GeoShapeType geoShapeType(uint32_t itemPos) const {
		return priv()->geoShapeType(itemPos);
	}
	uint32_t geoPointCount(uint32_t itemPos) const {
		return priv()->geoPointCount(itemPos);
	}
	sserialize::Static::spatial::GeoPoint geoPointAt(uint32_t itemPos, uint32_t pos) const {
		return priv()->geoPointAt(itemPos, pos);
	}
	sserialize::Static::spatial::GeoShape geoShape(uint32_t itemPos) const {
		return priv()->geoShapeAt(itemPos);
	}
	
	std::string getName() const { return "sserialize::Static::GeoStringsItemDB"; }
};


}}//end namespace



#endif