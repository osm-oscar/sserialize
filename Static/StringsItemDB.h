#ifndef SSERIALIZE_STATIC_STRINGS_ITEM_DATA_BASE_H
#define SSERIALIZE_STATIC_STRINGS_ITEM_DATA_BASE_H
#include "StringsItemDBPrivate.h"

namespace sserialize {
namespace Static {

template<class MetaDataDeSerializable>
class StringsItemDB: RCWrapper< sserialize::Static::StringsItemDBPrivate<MetaDataDeSerializable> > {
public:
	typedef sserialize::Static::StringsItemDBItem<StringsItemDB, MetaDataDeSerializable> Item;
protected:
	typedef sserialize::Static::StringsItemDBPrivate<MetaDataDeSerializable> MyPrivateClass;
	typedef RCWrapper< sserialize::Static::StringsItemDBPrivate<MetaDataDeSerializable> > MyParentClass;
	MyPrivateClass * priv() const {
		return static_cast<MyPrivateClass*>( MyParentClass::priv() );
	}
	StringsItemDB(MyPrivateClass * priv) : MyParentClass( priv ) {}
public:
	StringsItemDB() : MyParentClass( new MyPrivateClass() ) {}
	StringsItemDB(const UByteArrayAdapter & db, const StringTable & stable) :  MyParentClass( new MyPrivateClass(db, stable) ) {}
	StringsItemDB(const StringsItemDB& other) : MyParentClass( other ) {}
	virtual ~StringsItemDB() {}
	StringsItemDB & operator=(const StringsItemDB & other) {
		MyParentClass::operator=(other);
		return *this;
	}
	
	uint32_t size() const { return priv()->size(); }
	uint32_t getSizeInBytes() const { return priv()->getSizeInBytes(); }
	UByteArrayAdapter itemDataAt(uint32_t itemId) const { return priv()->itemDataAt(itemId); }
	MetaDataDeSerializable itemPayloadAt(uint32_t itemId) const { return priv()->itemPayloadAt(itemId);}
	const StringTable & stringTable() const { return priv()->stringTable(); }
	Item at(uint32_t itemId) const {
		if (itemId >= size())
			return Item();
		return Item(itemId, *this);
	}
	ItemIndex complete(const std::string & searchStr, sserialize::StringCompleter::QuerryType qtype) const {
		return priv()->complete(searchStr, qtype);
	}
	ItemIndexIterator partialComplete(const std::string & searchStr, sserialize::StringCompleter::QuerryType qtype) const {
		if (qtype & sserialize::StringCompleter::QT_CASE_INSENSITIVE)
			return ItemIndexIterator( new ItemIndexIteratorDB<StringsItemDB<MetaDataDeSerializable>, std::pair<std::string, sserialize::StringCompleter::QuerryType> >(*this, std::pair<std::string, sserialize::StringCompleter::QuerryType>(unicode_to_lower(searchStr), qtype) ) );
		else
			return ItemIndexIterator( new ItemIndexIteratorDB<StringsItemDB<MetaDataDeSerializable>, std::pair<std::string, sserialize::StringCompleter::QuerryType> >(*this, std::pair<std::string, sserialize::StringCompleter::QuerryType>(searchStr, qtype) ) );
	}
	ItemIndex select(const std::unordered_set<uint32_t> & strIds) const {
		return priv()->select(strIds);
	}
	ItemIndex select(const ItemIndex & idx) const {
		return priv()->select(idx);
	}
	bool match(uint32_t pos, const ItemIndex & idx) const {
		return priv()->match(pos, idx);
	}
	bool match(uint32_t pos, const std::unordered_set<uint32_t> & idx) const {
		return priv()->match(pos, idx);
	}
	bool match(uint32_t pos, std::pair< std::string, sserialize::StringCompleter::QuerryType > querry) const {
		return priv()->match(pos, querry);
	}
	sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const {
		return priv()->getSupportedQuerries();
	}
	std::ostream& printStats(std::ostream & out) const {
		return priv()->printStats(out);
	}
	
	//Access functions for StringsItemDBItem
	uint32_t strCount(uint32_t itemPos) const {
		return priv()->strCount(itemPos);
	}
	uint32_t strIdAt(uint32_t itemPos, uint32_t strPos) const {
		return priv()->strIdAt(itemPos, strPos);
	}
	std::string toString(uint32_t strId) const {
		return priv()->toString(strId);
	}
	bool match(uint32_t itemPos, const std::string & str, const sserialize::StringCompleter::QuerryType qt) const {
		return priv()->match(itemPos, str, qt);
	}
};

}}//end namespace

#endif
