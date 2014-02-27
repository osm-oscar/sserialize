#ifndef STRINGS_ITEM_DB_H
#define STRINGS_ITEM_DB_H
#include "StringsItemDBPrivate.h"
#include "StringsItemDBItem.h"
#include <sserialize/Static/Deque.h>

namespace sserialize {

template<typename ItemType>
class StringsItemDB: public RCWrapper< StringsItemDBPrivate<ItemType> > {
public:
	typedef StringsItem<StringsItemDB> Item;

	typedef typename StringsItemDBPrivate<ItemType>::ItemContainer ItemContainer;
	typedef typename StringsItemDBPrivate<ItemType>::ItemIterator ItemIterator;
	
	typedef typename StringsItemDBPrivate<ItemType>::ItemStringsContainer ItemStringsContainer;
	
	typedef typename StringsItemDBPrivate<ItemType>::ConstItemIterator ConstItemIterator;
	typedef typename StringsItemDBPrivate<ItemType>::StrToStrIdCountIterator StrToStrIdCountIterator;
	typedef typename StringsItemDBPrivate<ItemType>::StrIdToStrIterator StrIdToStrIterator;
protected:
	typedef RCWrapper< StringsItemDBPrivate<ItemType> > MyParentClass;
	typedef StringsItemDBPrivate<ItemType> MyPrivateClass;
	StringsItemDBPrivate<ItemType> * priv() const {
		return RCWrapper< StringsItemDBPrivate<ItemType> >::priv();
	}
	StringsItemDB(StringsItemDBPrivate<ItemType> * priv) : MyParentClass(priv) {}
public:
	StringsItemDB() : RCWrapper< StringsItemDBPrivate<ItemType> >(new StringsItemDBPrivate<ItemType>() )  {}
	StringsItemDB(const StringsItemDB & other) : RCWrapper< StringsItemDBPrivate<ItemType> >(other) {}
	virtual ~StringsItemDB() {}
	StringsItemDB & operator=(const StringsItemDB & other) {
		RCWrapper< StringsItemDBPrivate<ItemType> >::operator=(other);
		return *this;
	}
	size_t size() const { return priv()->size(); }
	virtual void clear() { priv()->clear();}
	std::set<unsigned int> match(const std::string & str, sserialize::StringCompleter::QuerryType qt) const {
		return priv()->match(str, qt);
	}

	ItemIndex complete(const std::string& str, sserialize::StringCompleter::QuerryType qt) {
		return priv()->complete(str, qt);
	}

	ItemIndex select(const ItemIndex & idx) const {
		std::unordered_set<uint32_t> strIds;
		for(uint32_t i = 0; i < idx.size(); i++)
			strIds.insert(idx.at(i));
		return priv()->select(strIds);
	}

	ItemIndex select(const std::unordered_set<uint32_t> & strIds) const {
		return priv()->select(strIds);
	}
	
	bool match(uint32_t pos, const ItemIndex & idx) const {
		return priv()->match(pos, idx);
	}
	
	bool match(uint32_t pos, const std::unordered_set<uint32_t> & idx) const {
		return priv()->match(pos, idx);
	}
	
	unsigned int insert(const std::deque<std::string> & strs, const ItemType & item) {
		return priv()->push_back(strs, item);
	}

	unsigned int insert(const std::vector<std::string> & strs, const ItemType & item) {
		return priv()->push_back(strs, item);
	}

	unsigned int insert(const std::string & str, const ItemType & item) {
		return priv()->insert(str, item);
	}
	/** This is NOT Thread-sage */
	bool addStringsToItem(uint32_t itemPos, std::deque<std::string> insertStrs) {
		return priv()->addStringsToItem(itemPos, insertStrs);
	}

	ItemContainer & items() {
		return priv()->items();
	}
	
	const ItemContainer & items() const {
		return priv()->items();
	}
	
	ItemStringsContainer & itemStrings() {
		return priv()->itemStrings();
	}

	const ItemStringsContainer & itemStrings() const {
		return priv()->itemStrings();
	}

	
	const std::vector<std::string> & strIdToStr() const {
		return priv()->strIdToStr();
	}

	Item at(size_t pos) {
		return StringsItem<StringsItemDB>(pos, *this);
	}
	
	std::string toString(uint32_t strId) {
		return priv()->toString(strId);
	}

	/** Assigns string ids according to usage-frequency */
	void reAssignStringIds() {
		priv()->reAssignStringIds();
	}
	
	/** Assigns item ids **/
	void reAssignItemIds(std::map< uint32_t, uint32_t > & oldToNew) {
		priv()->reAssignItemIds(oldToNew);
	}

	/**create a static stringtable, you should call reAssignStringIds before **/
	void createStaticStringTable(UByteArrayAdapter & destination) {
		priv()->createStaticStringTable(destination);
	}
	
	void absorb(StringsItemDB<ItemType> & db) {
		priv()->absorb(*(db.priv()));
	}

	/** @throws std::bad_alloc or anything that vector::reserve() may throw */
	void reserve(size_t count) {
		priv()->reserve(count);
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

}//end namespace

template<typename ItemType>
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::StringsItemDB<ItemType> & db) {


	sserialize::UByteArrayAdapter::OffsetType dO = destination.tellPutPtr();
	destination.putUint8(3); //version
	
	sserialize::ProgressInfo progressInfo;
	progressInfo.begin(db.items().size());
	uint32_t count = 0;
	sserialize::Static::DequeCreator<sserialize::UByteArrayAdapter> creator(destination);
	for(typename sserialize::StringsItemDB<ItemType>::ItemStringsContainer::const_iterator it = db.itemStrings().begin(); it != db.itemStrings().end(); ++it) {
		creator.beginRawPut();
		creator.rawPut().putUint8(it->size());
		for(size_t i = 0; i < it->size(); i++) {
			creator.rawPut().putUint32(it->at(i));
		}
		creator.endRawPut();
		progressInfo(++count, "StringsItemDB<ItemType>::serialize::ItemStrings");
	}
	creator.flush();
	progressInfo.end("StringsItemDB<ItemType>::serialize::ItemStrings");

	//Now put the payload
	destination << db.items();
	
	std::cout << "StringsItemDB<ItemType>::getSizeInBytes()=" << destination.tellPutPtr()-dO << std::endl;
	
	return destination;
}


#endif
