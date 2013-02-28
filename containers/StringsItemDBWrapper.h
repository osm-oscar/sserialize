#ifndef SSERIALIZE_STRINGS_ITEM_DB_WRAPPER_H
#define SSERIALIZE_STRINGS_ITEM_DB_WRAPPER_H
#include <deque>
#include <map>
#include <string>
#include <sserialize/utility/refcounting.h>
#include "ItemIndex.h"


namespace sserialize {

template<typename ItemType>
class StringsItemDBWrapperPrivate: public RefCountObject {
public:
	StringsItemDBWrapperPrivate() {}
	virtual ~StringsItemDBWrapperPrivate() {}
	virtual size_t size() const = 0;
	virtual void clear() = 0;
	virtual unsigned int insert(const std::deque<std::string> & strs, const ItemType & value) = 0;
	virtual bool addStringsToItem(unsigned int itemId, const std::deque<std::string> & itemStrs) = 0;
	virtual std::vector<unsigned int> itemStringIDs(uint32_t itemPos) const = 0;
	virtual const ItemType & at(uint32_t itemPos) const = 0;
	virtual const std::map<unsigned int, std::string> & strIdToStr() const = 0;
	virtual ItemIndex select(const ItemIndex & idx) const = 0;
	virtual ItemIndex select(const std::unordered_set<uint32_t> & strIds) const = 0;
	virtual bool match(uint32_t pos, const ItemIndex & idx) const = 0;
	virtual bool match(uint32_t pos, const std::unordered_set<uint32_t> & idx) const = 0;
};

template<typename ItemType>
class StringsItemDBWrapper: public RCWrapper< StringsItemDBWrapperPrivate< ItemType > > {
public:
	StringsItemDBWrapper() : RCWrapper< StringsItemDBWrapperPrivate< ItemType > >(0) {}
	StringsItemDBWrapper(StringsItemDBWrapperPrivate<ItemType> * data) :
		RCWrapper< StringsItemDBWrapperPrivate< ItemType > >(data) {}
	virtual ~StringsItemDBWrapper() {}
	StringsItemDBWrapper & operator=(const StringsItemDBWrapper<ItemType> & other) {
		RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::operator=(other);
		return *this;
	}
	size_t size() const {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->size();
	}
	void clear() {
		RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->clear();
	}
	unsigned int insert(const std::deque<std::string> & strs, const ItemType & value) {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->insert(strs, value);
	}
	bool addStringsToItem(unsigned int itemId, const std::deque<std::string> & itemStrs) {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->addStringsToItem(itemId, itemStrs);
	}
	std::vector<unsigned int> itemStringIDs(uint32_t itemPos) const {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->itemStringIDs(itemPos);
	}
	
	const ItemType & at(uint32_t itemPos) const {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->at(itemPos);
	}
	
	const std::map<unsigned int, std::string> & strIdToStr() const {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->strIdToStr();
	}
	StringsItemDBWrapperPrivate<ItemType> * getPrivate() const {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv();
	}
	
	ItemIndex select(const ItemIndex & idx) const {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->select(idx);
	}
	
	ItemIndex select(const std::unordered_set<uint32_t> & strIds) const {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->select(strIds);
	}
	
	bool match(uint32_t pos, const ItemIndex & idx) const {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->match(pos, idx);
	}

	bool match(uint32_t pos, const std::unordered_set<uint32_t> & idx) const {
		return RCWrapper< StringsItemDBWrapperPrivate< ItemType > >::priv()->match(pos, idx);
	}
	
};


};

#endif