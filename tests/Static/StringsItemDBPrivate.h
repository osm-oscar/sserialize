#ifndef SSERIALIZE_STATIC_STRINGS_ITEM_DB_PRIVATE_H
#define SSERIALIZE_STATIC_STRINGS_ITEM_DB_PRIVATE_H
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/Static/StringTable.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexIteratorDB.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/Static/Array.h>
#include <sserialize/utility/refcounting.h>
#include "StringsItemDBItem.h"
#include <iostream>

#define SSERIALIZE_STATIC_STRINGS_ITEM_DB_VERSION 3

/** Data Layout: v3
 *
 * DataBase
 * ---------------------------------------------------
 * VERSION|Array<<ItemStrings>|Array<ItemPayload>
 *----------------------------------------------------
 *  1     |    ItemStrings    |     *
 *
 *
 * ItemStrings
 * -----------------------
 * SIZE|StringsIds
 * -----------------------
 *   1 | 4*
 *------------------------
 */

namespace sserialize {
namespace Static {

template<class MetaDataDeSerializable>
class StringsItemDBPrivate: public RefCountObject {
private:
	StringTable m_stringTable;
	Static::Array< UByteArrayAdapter > m_itemStrings;
	Static::Array< UByteArrayAdapter > m_payloads;
public:
	StringsItemDBPrivate();
	StringsItemDBPrivate(const UByteArrayAdapter & db, const StringTable & stable);
	virtual ~StringsItemDBPrivate();
	inline uint32_t size() const { return m_itemStrings.size();}
	inline uint32_t getSizeInBytes() const { return 1+m_itemStrings.getSizeInBytes()+m_payloads.getSizeInBytes();}
	UByteArrayAdapter itemDataAt(uint32_t itemId) const;
	MetaDataDeSerializable itemPayloadAt(uint32_t itemId) const;
	const StringTable & stringTable() const { return m_stringTable; }
	ItemIndex complete(const std::string & searchStr, sserialize::StringCompleter::QuerryType qtype) const;
	ItemIndex select(const std::unordered_set<uint32_t> & strIds) const;
	ItemIndex select(const ItemIndex & idx) const;
	bool match(uint32_t pos, const ItemIndex & idx) const;
	bool match(uint32_t pos, const std::unordered_set<uint32_t> & idx) const;
	bool match(uint32_t pos, std::pair< std::string, sserialize::StringCompleter::QuerryType > querry) const;
	sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const { return sserialize::StringCompleter::SQ_ALL;}
	std::ostream& printStats(std::ostream & out) const;
	
	//Access functions for StringsItemDBItem
	uint32_t strCount(uint32_t itemPos) const;
	uint32_t strIdAt(uint32_t itemPos, uint32_t strPos) const;
	std::string toString(uint32_t strId) const;
	bool match(uint32_t itemPos, const std::string & str, const sserialize::StringCompleter::QuerryType qt) const;
};

template<class MetaDataDeSerializable>
StringsItemDBPrivate<MetaDataDeSerializable>::StringsItemDBPrivate() : RefCountObject() {}


template<class MetaDataDeSerializable>
StringsItemDBPrivate<MetaDataDeSerializable>::StringsItemDBPrivate(const UByteArrayAdapter & db, const StringTable & stable) :
RefCountObject(),
m_stringTable(stable),
m_itemStrings(db+1),
m_payloads(db+(1+m_itemStrings.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_STRINGS_ITEM_DB_VERSION, db.at(0), "Static::StringsItemDB");
}

template<class MetaDataDeSerializable>
StringsItemDBPrivate<MetaDataDeSerializable>::~StringsItemDBPrivate() {}


template<class MetaDataDeSerializable>
UByteArrayAdapter
StringsItemDBPrivate<MetaDataDeSerializable>::itemDataAt(uint32_t itemId) const {
	if (itemId >= size())
		return  UByteArrayAdapter();
	return m_payloads.at(itemId);
}

template<class MetaDataDeSerializable>
MetaDataDeSerializable
StringsItemDBPrivate<MetaDataDeSerializable>::itemPayloadAt(uint32_t itemId) const {
	if (itemId >= size())
		return  MetaDataDeSerializable();
	return MetaDataDeSerializable( m_payloads.at(itemId) );
}


#define NEWFOM

template<class MetaDataDeSerializable>
ItemIndex
StringsItemDBPrivate<MetaDataDeSerializable>::complete(const std::string& searchStr, sserialize::StringCompleter::QuerryType qtype) const {
	std::unordered_set<uint32_t> set;
	TimeMeasurer tm;
	tm.begin();
	stringTable().find(set, searchStr, qtype);
	tm.end();
	long int stableTime = tm.elapsedTime();
	
	tm.begin();
	ItemIndex idx( select(set) );
	tm.end();
	std::cout << "StringsItemDBPrivate::complete(" << searchStr << "," << static_cast<uint32_t>(qtype) << "): Finding stringsids: ";
	std::cout << stableTime << "; Finding items: " << tm.elapsedTime() << std::endl;
	return idx;
}

template<class MetaDataDeSerializable>
ItemIndex
StringsItemDBPrivate<MetaDataDeSerializable>::select(const ItemIndex & idx) const {
	if (!idx.size())
		return ItemIndex();

	std::unordered_set<uint32_t> set;
	for(uint32_t i = 0; i < idx.size(); i++)
		set.insert(idx.at(i));
	
	return select(set);
}

template<class MetaDataDeSerializable>
ItemIndex
StringsItemDBPrivate<MetaDataDeSerializable>::select(const std::unordered_set<uint32_t> & set) const {
	if (!set.size())
		return ItemIndex();

	std::vector<uint32_t> arr;
	
	uint32_t size = this->size();
	uint32_t curOffSet = 0;
	uint8_t strCount = 0;
	bool doInsert = false;
	uint32_t curStrId;
	UByteArrayAdapter curItemStrings;
	for(uint32_t i = 0; i < size; i++) {
		curOffSet = 0;
		UByteArrayAdapter curItemStrings = m_itemStrings.at(i);
		strCount = curItemStrings.at(curOffSet);
		++curOffSet;
		doInsert = false;
		uint8_t j = 0;
		for(; j < strCount; ++j) {
			curStrId = curItemStrings.getUint32(curOffSet);
			curOffSet += 4;
			if (set.count(curStrId) > 0) {
				doInsert = true;
				break;
			}
		}
		if (doInsert) {
			arr.push_back(i);
		}
	}
	return ItemIndex::absorb(arr);
}

template<class MetaDataDeSerializable>
bool
StringsItemDBPrivate<MetaDataDeSerializable>::match(uint32_t pos, const ItemIndex & idx) const {
	if (pos >= size())
		return false;
	UByteArrayAdapter itemStrs( m_itemStrings.at(pos) );
	uint8_t count = itemStrs.at(0);
	itemStrs += 1;
	for(uint8_t i = 0; i < count; ++i) {
		if ( idx.count( itemStrs.getUint32() ) )
			return true;
		itemStrs += 4;
	}
	return false;
}


template<class MetaDataDeSerializable>
bool
StringsItemDBPrivate<MetaDataDeSerializable>::match(uint32_t pos, const std::unordered_set<uint32_t> & idx) const {
	if (pos >= size() || !idx.size())
		return false;
	UByteArrayAdapter itemStrs( m_itemStrings.at(pos) );
	uint8_t count = itemStrs.at(0);
	itemStrs += 1;
	for(uint8_t i = 0; i < count; ++i) {
		if ( idx.count( itemStrs.getUint32() ) )
			return true;
		itemStrs += 4;
	}
	return false;
}

template<class MetaDataDeSerializable>
bool
StringsItemDBPrivate<MetaDataDeSerializable>::match(uint32_t pos, std::pair< std::string, sserialize::StringCompleter::QuerryType > querry) const {
	if (pos >= size())
		return false;
	return match(pos, querry.first, querry.second);
}

template<class MetaDataDeSerializable>
std::ostream &
StringsItemDBPrivate<MetaDataDeSerializable>::printStats(std::ostream & out) const {
	out << "Static::StringsItemDBPrivate::Stats -- BEGIN" << std::endl;
	out << "size: " << size() << std::endl;
	out << "storage size: " << getSizeInBytes() << std::endl;
	m_stringTable.printStats(out);
	out << "Static::StringsItemDBPrivate::Stats -- END" << std::endl;
	return out;
}

template<class MetaDataDeSerializable>
uint32_t StringsItemDBPrivate<MetaDataDeSerializable>::strCount(uint32_t itemPos) const {
	if (itemPos >= size())
		return 0;
	return m_itemStrings.at(itemPos).getUint8(0);
}

template<class MetaDataDeSerializable>
uint32_t StringsItemDBPrivate<MetaDataDeSerializable>::strIdAt(uint32_t itemPos, uint32_t strPos ) const {
	return m_itemStrings.at(itemPos).getUint32(1+4*strPos);
}

template<class MetaDataDeSerializable>
std::string StringsItemDBPrivate<MetaDataDeSerializable>::toString(uint32_t strId) const {
	return m_stringTable.at(strId);
}
template<class MetaDataDeSerializable>
bool StringsItemDBPrivate<MetaDataDeSerializable>::match(uint32_t itemPos, const std::string& str, const sserialize::StringCompleter::QuerryType qt) const {
	if (itemPos >= size() || !str.size())
		return false;
	UByteArrayAdapter itemStrs( m_itemStrings.at(itemPos) );
	uint8_t count = itemStrs.at(0);
	itemStrs += 1;
	for(uint8_t i = 0; i < count; ++i) {
		if ( m_stringTable.match(itemStrs.getUint32(0), str, qt) )
			return true;
		itemStrs += 4;
	}
	return false;
}



}}//end namespace

#endif
