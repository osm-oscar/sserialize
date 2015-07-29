#ifndef SSERIALIZE_STRINGS_ITEMDB_WRAPPER_PRIVATE_SSIDB_H
#define SSERIALIZE_STRINGS_ITEMDB_WRAPPER_PRIVATE_SSIDB_H
#include <sserialize/containers/StringsItemDBWrapper.h>
#include <sserialize/Static/StringsItemDB.h>
#include <sserialize/templated/StringsItemDBPrivate.h>

namespace sserialize {

template<typename ItemType>
class StringsItemDBWrapperPrivateSSIDB: public StringsItemDBWrapperPrivate<ItemType> {
	Static::StringsItemDB<ItemType> m_db;
	std::vector<std::string> m_strIdToStr;
	ItemType m_dummy;
public:
	StringsItemDBWrapperPrivateSSIDB(const Static::StringsItemDB<ItemType> & db) : 
		StringsItemDBWrapperPrivate<ItemType>(), m_db(db) {
			Static::StringTable stable = m_db.stringTable();
			m_strIdToStr.resize(stable.size());
			for(size_t i = 0; i < stable.size(); i++) {
				m_strIdToStr[i] = stable.at(i);
			}
		}
	virtual ~StringsItemDBWrapperPrivateSSIDB() {}
	virtual size_t size() const {
		return m_db.size();
	}
	virtual void clear() {}
	virtual unsigned int insert(const std::deque<std::string> & /*strs*/, const ItemType & /*value*/) {
		return 0;
	}
	virtual bool addStringsToItem(unsigned int /*itemId*/, const std::deque<std::string> & /*itemStrs*/) {
		return false;
	}
	virtual std::vector<unsigned int> itemStringIDs(uint32_t itemPos) const {
		typename Static::StringsItemDB<ItemType>::Item item = m_db.at(itemPos);
		std::vector<unsigned int> strs;
		strs.reserve(item.strCount());
		for(size_t i = 0; i < item.strCount(); i++) {
			strs.push_back(item.strIdAt(i));
		}
		return strs;
	}
	virtual const std::vector<std::string> & strIdToStr() const {
		return m_strIdToStr;
	}
	
	/** @return returns a dumym value, don't use it! **/
	virtual const ItemType & at(uint32_t /*itemPos*/) const {
		return m_dummy;
	}

	ItemIndex select(const ItemIndex & idx) const {
		return m_db.select(idx);
	}
	
	ItemIndex select(const std::unordered_set<uint32_t> & idx) const {
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