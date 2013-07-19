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
	
	template<typename T_OLD_TO_NEW_MAP>
	void sort(T_OLD_TO_NEW_MAP & oldToNew) {
		std::sort(m_strings.begin(), m_strings.end());
		uint32_t s = m_strings.size();
		for(uint32_t i = 0; i < s; ++i) {
			uint32_t & old = m_map[ m_strings[i] ];
			oldToNew[old] = i;
			old = i;
		}
	}
	
	inline bool count(const std::string & str) const { return m_map.count(str) > 0; }
	inline std::string at(uint32_t id) const {
		return (id < m_strings.size() ? m_strings[id] : std::string());
	}
	
	///This remapds the string ids, sort has to be called before and you should update your string ids afterwards
	inline UByteArrayAdapter serialize(UByteArrayAdapter & dest) const {
		if (!std::is_sorted(m_strings.begin(), m_strings.end())) {
			throw sserialize::CreationException("StringTable is not sorted");
		}
		return dest << m_strings;
	};
};

}//end namespace

#endif