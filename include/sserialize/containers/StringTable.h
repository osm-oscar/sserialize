#ifndef SSERIALIZE_STRING_TABLE_H
#define SSERIALIZE_STRING_TABLE_H
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <sserialize/Static/StringTable.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {

class StringTable {
	std::vector<std::string> m_strings;
	std::unordered_map<std::string, uint32_t> m_map;
public:
	StringTable() {}
	~StringTable() {}
	inline std::size_t size() const { return m_strings.size(); }
	inline uint32_t insert(const std::string & str) {
		if (m_map.count(str))
			return m_map[str];
		else {
			uint32_t id = m_strings.size();
			m_map[str] = id;
			m_strings.push_back(str);
			return id;
		}
	}
	
	///@param oldToNew an object with operator[] = ;
	template<typename T_OLD_TO_NEW_MAP>
	void sort(T_OLD_TO_NEW_MAP & oldToNew) {
		std::sort(m_strings.begin(), m_strings.end());
		for(uint32_t i = 0, s = m_strings.size(); i < s; ++i) {
			uint32_t & old = m_map[ m_strings[i] ];
			oldToNew[old] = i;
			old = i;
		}
	}

	void sort() {
		std::sort(m_strings.begin(), m_strings.end());
		for(uint32_t i = 0, s = m_strings.size(); i < s; ++i) {
			m_map[ m_strings[i] ] = i;
		}
	}
	
	inline bool count(const std::string & str) const { return m_map.count(str) > 0; }
	inline std::string at(uint32_t id) const {
		return (id < m_strings.size() ? m_strings[id] : std::string());
	}
	
	uint32_t at(const std::string & str) const {
		if (count(str))
			return m_map.at(str);
		throw sserialize::OutOfBoundsException("sserialize::StringTable::at");
	}
	
	///This remapds the string ids, sort has to be called before and you should update your string ids afterwards
	inline UByteArrayAdapter serialize(UByteArrayAdapter & dest) const {
		if (!std::is_sorted(m_strings.begin(), m_strings.end())) {
			throw sserialize::CreationException("StringTable is not sorted");
		}
		return dest << m_strings;
	};
	
	void swap(StringTable & other) {
		using std::swap;
		swap(m_map, other.m_map);
		swap(m_strings, other.m_strings);
	}
};

}//end namespace

inline void swap(sserialize::StringTable & a, sserialize::StringTable & b) {
	a.swap(b);

}
#endif