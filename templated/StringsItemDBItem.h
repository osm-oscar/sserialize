#ifndef SSERIALIZE_STRINGS_ITEM_DB_ITEM_H
#define SSERIALIZE_STRINGS_ITEM_DB_ITEM_H
#include <stdint.h>
#include <sserialize/completers/StringCompleter.h>
#include <unordered_set>

namespace sserialize {

template<typename DataBaseType>
class StringsItem {
	uint32_t m_id;
	DataBaseType m_db;
public:
	StringsItem() : m_id(0) {}
	StringsItem(const uint32_t id, const DataBaseType & db) : m_id(id), m_db(db) {}
	virtual ~StringsItem() {}
	uint32_t id() const { return m_id; }
	const DataBaseType & db() const { return m_db; }
	DataBaseType & db() { return m_db; }
	uint32_t strCount() const { return m_db.strCount(m_id); }
	uint32_t strIdAt(uint32_t pos) const { return m_db.strIdAt(m_id, pos);}
	std::string strAt(uint32_t pos) const { return m_db.toString(strIdAt(pos)); }
	bool match(const std::string & str, sserialize::StringCompleter::QuerryType qt) const {
		return m_db.match(m_id, str, qt);
	}
	bool hasAnyStrIdOf(const std::unordered_set<uint32_t> & testSet) const {
		uint32_t sc = strCount();
		for(size_t i = 0; i < sc; ++i) {
			if (testSet.count(strIdAt(i)) > 0)
				return true;
		}
		return false;
	}
	bool hasAnyStrIdOf(const ItemIndex & testSet) const {
		uint32_t sc = strCount();
		for(size_t i = 0; i < sc; ++i) {
			if (testSet.count(strIdAt(i)) > 0)
				return true;
		}
		return false;
	}

};



}//end namespace

#endif