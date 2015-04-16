#ifndef STRINGS_ITEM_DB_PRIVATE_H
#define STRINGS_ITEM_DB_PRIVATE_H
#include <deque>
#include <algorithm>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/ProgressInfo.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRegLine.h>
#include <sserialize/Static/StringTable.h>
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/utilfuncs.h>
#include "ItemDB.h"

namespace sserialize {


template<typename ItemType>
class StringsItemDBPrivate: public RefCountObject {
public:
	typedef typename std::vector< ItemType > ItemContainer;
	typedef typename ItemContainer::iterator ItemIterator;
	typedef typename ItemContainer::const_iterator ConstItemIterator;
	
	typedef std::vector< std::vector<uint32_t> > ItemStringsContainer;
	typedef std::map<std::string, std::pair<unsigned int, unsigned int> >::iterator StrToStrIdCountIterator;
	typedef std::map<std::string, std::pair<unsigned int, unsigned int> >::const_iterator ConstStrToStrIdCountIterator;
	typedef std::vector<std::string>::iterator StrIdToStrIterator;


private:
	std::map<std::string, std::pair<unsigned int, unsigned int> > m_strToStrIdCount;
	std::vector<std::string> m_strIdToStr;
	ItemStringsContainer m_itemStrings;
	ItemContainer m_items;
private:
	unsigned int insert(const std::string& str, unsigned int addCount);
	
public:
	StringsItemDBPrivate() {}
	virtual ~StringsItemDBPrivate() {}
	virtual void clear();
	size_t size() const { return m_items.size();}
	
	
	
	std::unordered_set<unsigned int> match(std::string str, sserialize::StringCompleter::QuerryType qt) const;
	ItemIndex complete(const std::string& str, sserialize::StringCompleter::QuerryType qt) const;
	ItemIndex select(const std::unordered_set<uint32_t> & strIds) const;
	bool match(uint32_t pos, const ItemIndex & idx) const;
	bool match(uint32_t pos, const std::unordered_set<uint32_t> & idx) const;

	unsigned int push_back(const std::deque<std::string> & strs, const ItemType & item);
	unsigned int push_back(const std::vector<std::string> & strs, const ItemType & item);
	unsigned int push_back(const std::string & str, const ItemType & item);
	bool addStringsToItem(uint32_t itemPos, std::deque<std::string> insertStrs);

	ItemContainer & items() { return m_items;};
	const ItemContainer & items() const { return m_items;};
	ItemStringsContainer & itemStrings() { return m_itemStrings; }
	const ItemStringsContainer & itemStrings() const { return m_itemStrings;}

	const std::vector<std::string> & strIdToStr() const { return m_strIdToStr; }

	std::string toString(uint32_t strId);

	/** Assigns string ids according to usage-frequency */
	void reAssignStringIds();
	
	/** Assigns item ids **/
	void reAssignItemIds(const std::map< uint32_t, uint32_t >& oldToNew);

	/**create a static stringtable, you should call reAssignStringIds before **/
	void createStaticStringTable(UByteArrayAdapter & destination) {
		Static::StringTable::create(m_strIdToStr.cbegin(), m_strIdToStr.cend(), destination);
	}
	
	void absorb(StringsItemDBPrivate<ItemType> & db);
	
	void reserve(size_t count) {
		m_items.reserve(count);
	}
	
	//Access functions for StringsItemDBItem
	uint32_t strCount(uint32_t itemPos) const { return itemStrings().at(itemPos).size();}
	uint32_t strIdAt(uint32_t itemPos, uint32_t strPos) const { return itemStrings().at(itemPos).at(strPos);}
	bool match(uint32_t /*itemPos*/, const std::string & /*str*/, const sserialize::StringCompleter::QuerryType /*qt*/) const {
		return false;
	}

};

template<typename ItemType>
unsigned int
StringsItemDBPrivate<ItemType>::insert(const std::string & str, unsigned int addCount) {
	if (m_strToStrIdCount.count(str) > 0) {
		StrToStrIdCountIterator it = m_strToStrIdCount.find(str);
		it->second.second += addCount;
		return it->second.first;
	}
	else {
		unsigned int newStrId = m_strIdToStr.size();
		m_strIdToStr.push_back(str);
		m_strToStrIdCount[str] = std::pair<unsigned int, unsigned int>(newStrId, addCount);
		return newStrId;
	}
}



template<typename ItemType>
void
StringsItemDBPrivate<ItemType>::clear() {
	m_strToStrIdCount.clear();
	m_strIdToStr.clear();
	m_items = ItemContainer();
}

template<typename ItemType>
std::unordered_set<unsigned int>
StringsItemDBPrivate<ItemType>::match(std::string str, sserialize::StringCompleter::QuerryType qt) const {
	std::unordered_set<unsigned int> strIds;
	if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
		str = unicode_to_lower(str);
	}
	for(ConstStrToStrIdCountIterator it = m_strToStrIdCount.begin(); it != m_strToStrIdCount.end(); it++) {
		std::string testString;
		if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
			testString = unicode_to_lower(it->first);
		}
		else {
			testString = it->first;
		}
		if (qt & sserialize::StringCompleter::QT_EXACT || qt & sserialize::StringCompleter::QT_PREFIX) {
			size_t firstPos = testString.find(str);
			if (firstPos == 0) {
				if (qt & sserialize::StringCompleter::QT_EXACT && testString.size() == str.size()) {
					strIds.insert(it->second.first);
				}
				if (qt & sserialize::StringCompleter::QT_PREFIX) {
					strIds.insert(it->second.first);
				}
			}
		}
		else {
			size_t lastPos = testString.rfind(str);
			if (lastPos < std::string::npos) {
				if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
					strIds.insert(it->second.first);
				}
				if (qt & sserialize::StringCompleter::QT_SUFFIX && str.size() + lastPos == testString.size()) {
					strIds.insert(it->second.first);
				}
			}
		}
	}
	return strIds;
}

template<typename ItemType>
ItemIndex
StringsItemDBPrivate<ItemType>::select(const std::unordered_set<uint32_t> & strIds) const {
	std::deque<unsigned int> itemIds;
	for(size_t i = 0; i < m_itemStrings.size(); i++) {
		if (haveCommonValue(m_itemStrings[i], strIds) ) {
			itemIds.push_back(i);
		}
	}
	return ItemIndex(itemIds);
}


template<typename ItemType>
bool
StringsItemDBPrivate<ItemType>::match(uint32_t pos, const ItemIndex & strIds) const {
	if (pos >= size())
		return false;
	return haveCommonValue(m_itemStrings[pos], strIds);
}

template<typename ItemType>
bool
StringsItemDBPrivate<ItemType>::match(uint32_t pos, const std::unordered_set<uint32_t> & strIds) const {
	if (pos >= size())
		return false;
	return haveCommonValue(m_itemStrings[pos], strIds);
}

template<typename ItemType>
ItemIndex
StringsItemDBPrivate<ItemType>::complete(const std::string & str, sserialize::StringCompleter::QuerryType qt) const {
	return select( match(str, qt) );
}

template<typename ItemType>
std::string
StringsItemDBPrivate<ItemType>::toString(uint32_t strId) {
	if (strId < m_strIdToStr.size()) {
		return m_strIdToStr.at(strId);
	}
	return std::string();
}

template<typename ItemType>
unsigned int
StringsItemDBPrivate<ItemType>::push_back(const std::deque<std::string> & strs, const ItemType & item) {
	return push_back(std::vector<std::string>(strs.begin(), strs.end()), item);
}

template<typename ItemType>
unsigned int
StringsItemDBPrivate<ItemType>::push_back(const std::vector<std::string> & strs, const ItemType & item) {
	unsigned int itemId = m_items.size();
	std::vector<uint32_t> addedStrings;
	for(std::vector<std::string>::const_iterator it = strs.begin(); it != strs.end(); it++) {
		unsigned int strId;
		if (m_strToStrIdCount.count(*it) == 0) {
			strId = m_strIdToStr.size();
			m_strIdToStr.push_back(*it);
			m_strToStrIdCount[*it] = std::pair<unsigned int, unsigned int>(strId, 0);
		}
		else {
			strId = m_strToStrIdCount.at(*it).first;
			m_strToStrIdCount.at(*it).second += 1;
		}
		addedStrings.push_back(strId);
	}
	m_items.push_back(item);
	m_itemStrings.push_back(addedStrings);
	return itemId;
}

template<typename ItemType>
unsigned int
StringsItemDBPrivate<ItemType>::push_back(const std::string & str, const ItemType & item) {
	std::vector<std::string> strs;
	strs.push_back(str);
	return push_back(strs, item);
}

template<class ItemType>
bool StringsItemDBPrivate<ItemType>::addStringsToItem(uint32_t itemPos, std::deque<std::string> strs) {
	if (itemPos >= m_items.size())
		return false;
	
	std::deque<uint32_t> addedStrings;
	for(std::deque<std::string>::iterator it = strs.begin(); it != strs.end(); it++) {
		unsigned int strId;
		if (m_strToStrIdCount.count(*it) == 0) {
			strId = m_strIdToStr.size();
			m_strIdToStr.push_back(*it);
			m_strToStrIdCount[*it] = std::pair<unsigned int, unsigned int>(strId, 0);
		}
		else {
			strId = m_strToStrIdCount.at(*it).first;
			m_strToStrIdCount.at(*it).second += 1;
		}
		addedStrings.push_back(strId);
	}
	m_itemStrings[itemPos].insert(m_itemStrings[itemPos].end(), addedStrings.begin(), addedStrings.end());
	return true;
}

template <typename ItemType>
void
StringsItemDBPrivate<ItemType>::reAssignStringIds() {
	std::map<uint32_t, std::deque<uint32_t> > usageToStrIds;
	for(StrToStrIdCountIterator it = m_strToStrIdCount.begin(); it != m_strToStrIdCount.end(); it++) {
		usageToStrIds[it->second.second].push_back(it->second.first);
	}
	std::map<uint32_t, uint32_t> oldStrsToNewStrs;
	uint32_t newStrCounter = 0;
	for(std::map<uint32_t, std::deque<uint32_t> >::iterator it = usageToStrIds.begin(); it != usageToStrIds.end(); it++) {
		for(std::deque<uint32_t>::iterator jt = it->second.begin(); jt != it->second.end(); jt++) {
			oldStrsToNewStrs.insert(std::pair<uint32_t, uint32_t>(*jt, newStrCounter));
			newStrCounter++;
		}
	}
	usageToStrIds.clear();
	//Let's do the remapping
	for(StrToStrIdCountIterator it = m_strToStrIdCount.begin(); it != m_strToStrIdCount.end(); it++) {
		it->second.first = oldStrsToNewStrs.at(it->second.first);
		m_strIdToStr.at(it->second.first) = it->first;
	}
	for(ItemIterator it = m_items.begin(); it != m_items.end(); it++) {
		for(std::deque<uint32_t>::iterator jt = it->strs.begin(); jt != it->strs.end(); jt++) {
			*jt = oldStrsToNewStrs.at(*jt);
		}
	}
}

template <typename ItemType>
void
StringsItemDBPrivate<ItemType>::reAssignItemIds(const std::map<uint32_t, uint32_t> & oldToNew) {
	ItemContainer items;
	ItemStringsContainer itemStrings;
	items.resize(size());
	itemStrings.resize(size());
	
	ProgressInfo info;
	info.begin(size(), "StringsItemDB::reAssignItemIds");
	for(size_t oldId = 0; oldId < size(); ++oldId) {
		uint32_t newId = oldToNew.at(oldId);
		items[ newId ] = m_items[oldId];
		itemStrings[ newId ] = m_itemStrings[oldId];
		info(oldId);
	}
	info.end("StringsItemDB::reAssignItemIds");
	
	m_items.swap(items);
	m_itemStrings.swap(itemStrings);
}

template<typename ItemType>
void
StringsItemDBPrivate<ItemType>::absorb(StringsItemDBPrivate< ItemType >& db) {
	if (size() == 0 && m_strIdToStr.size() == 0 && m_strToStrIdCount.size()) {
		m_items.swap(db.m_items);
		m_itemStrings.swap(db.m_itemStrings);
		m_strIdToStr.swap(db.m_strIdToStr);
		m_strToStrIdCount.swap(db.m_strToStrIdCount);
		return;
	}

	//merge in the string table and create the remapping table
	std::unordered_map<unsigned int, unsigned int> strIdRemapMap;
	for(StrToStrIdCountIterator it = db.m_strToStrIdCount.begin(); it != db.m_strToStrIdCount.end(); it++) {
		unsigned int newId = insert(it->first, it->second.second);
		strIdRemapMap[it->second.first] = newId;
	}
	db.m_strIdToStr.clear();
	db.m_strToStrIdCount.clear();
	
	m_items.insert(m_items.end(), db.m_items.begin(), db.m_items.end());
	db.m_items = ItemContainer();
	
	m_itemStrings.reserve(m_itemStrings.size() + db.m_itemStrings.size());
	for(ItemStringsContainer::iterator it = db.m_itemStrings.begin(); it != db.m_itemStrings.end(); ++it) {
		m_itemStrings.push_back(*it);
		remap(m_itemStrings.back(), m_itemStrings.back(), strIdRemapMap);
	}
	db.m_itemStrings = ItemStringsContainer();
}

}//end namespace

#endif