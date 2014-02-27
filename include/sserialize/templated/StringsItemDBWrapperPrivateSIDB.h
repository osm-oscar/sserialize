#ifndef SSERIALIZE_STRINGS_ITEMDB_WRAPPER_PRIVATE_SIDB_H
#define SSERIALIZE_STRINGS_ITEMDB_WRAPPER_PRIVATE_SIDB_H
#include <sserialize/containers/StringsItemDBWrapper.h>
#include <sserialize/templated/StringsItemDB.h>

namespace sserialize {

template<typename ItemType>
class StringsItemDBWrapperPrivateSIDB: public StringsItemDBWrapperPrivate<ItemType> {
	StringsItemDB<ItemType> m_db;
public:
	StringsItemDBWrapperPrivateSIDB() {}
	StringsItemDBWrapperPrivateSIDB(const StringsItemDB<ItemType> & db) : 
		StringsItemDBWrapperPrivate<ItemType>(), m_db(db) {}
	virtual ~StringsItemDBWrapperPrivateSIDB() {}
	virtual size_t size() const {
		return m_db.items().size();
	}
	virtual void clear() {
		m_db.clear();
	}
	virtual unsigned int insert(const std::deque<std::string> & strs, const ItemType & value) {
		return m_db.insert(strs, value);
	}
	virtual bool addStringsToItem(unsigned int itemId, const std::deque<std::string> & itemStrs) {
		return m_db.addStringsToItem(itemId, itemStrs);
	}
	virtual std::vector<unsigned int> itemStringIDs(uint32_t itemPos) const {
		return m_db.itemStrings().at(itemPos);
	}
	virtual const ItemType & at(uint32_t itemPos) const {
		return m_db.items().at(itemPos);
	}
	
	virtual const std::vector<std::string> & strIdToStr() const {
		return m_db.strIdToStr();
	}
	
	virtual ItemIndex select(const ItemIndex & idx) const {
		return m_db.select(idx);
	}

	virtual ItemIndex select(const std::unordered_set<uint32_t> & idx) const {
		return m_db.select(idx);
	}
	
	virtual bool match(uint32_t pos, const ItemIndex & idx) const {
		return m_db.match(pos, idx);
	}

	virtual bool match(uint32_t pos, const std::unordered_set<uint32_t> & idx) const {
		return m_db.match(pos, idx);
	}
};

}//end namespace


#endif