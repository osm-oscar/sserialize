#ifndef SSERIALIZE_STRING_TABLE_H
#define SSERIALIZE_STRING_TABLE_H
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <string.h>
#include <sserialize/Static/StringTable.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/Static/Array.h>
#include <sserialize/algorithm/hashspecializations.h>

namespace sserialize {
class StringTable;
namespace detail {
namespace StringTable {
	class StaticString {
		friend class sserialize::StringTable;
	public:
		typedef const char * const_iterator;
	private:
		char * m_begin;
		uint32_t m_len;
		bool m_external;
		///if external==true, the begin is an offset into an external char* array
		StaticString(char * begin, uint32_t len, bool external) : m_begin(begin), m_len(len), m_external(external) {}
	public:
		StaticString() : m_begin(0), m_len(0), m_external(0) {}
		~StaticString() {}
		inline uint32_t size() const { return m_len; }
		inline bool external() const  { return m_external; }
		inline const_iterator begin() const { return m_begin; }
		inline const_iterator end() const { return m_begin+m_len; }
	};
}}}

namespace std {

	template<>
	struct hash< sserialize::detail::StringTable::StaticString > {
		inline size_t operator()(const char * begin, const char * end) const {
			size_t seed = 0;
			for(; begin != end; ++begin) {
				::hash_combine(seed, *begin);
			}
			return seed;
		}
	};
	
}

namespace sserialize {

class StringTable {
public:
	typedef uint32_t SizeType;
	typedef uint32_t StringSizeType;
	typedef uint64_t DataSizeType;
private:
	typedef detail::StringTable::StaticString StaticString;
private:

	struct MyHTFuncs {
		MyHTFuncs(const std::vector<char> * strData) : strData(strData) {}
		const std::vector<char> * strData;
		inline const char * cbegin(const StaticString & str) const {
			if (str.external()) {
				return str.begin();
			}
			else {
				return &((*strData)[0])+reinterpret_cast<DataSizeType>(str.begin());
			}
		}
		inline const char * cend(const StaticString & str) const {
			if (str.external()) {
				return str.end();
			}
			else {
				return &((*strData)[0])+reinterpret_cast<DataSizeType>(str.end());
			}
		}
	};

	struct MyHasher: public MyHTFuncs {
		MyHasher(const std::vector<char> * strData) : MyHTFuncs(strData) {}
		std::hash<StaticString> hasher;
		inline std::size_t operator()(const StaticString & str) const {
			return hasher(cbegin(str), cend(str));
		}
	};
	
	struct MyEq: MyHTFuncs {
		MyEq(const std::vector<char> * strData) : MyHTFuncs(strData) {}
		inline bool operator()(const StaticString & a, const StaticString & b) const {
			return a.size() == b.size() && std::equal(cbegin(a), cend(a), cbegin(b));
		}
	};
	
	struct MySmaller: MyHTFuncs {
		MySmaller(const std::vector<char> * strData) : MyHTFuncs(strData) {}
		inline bool operator()(const StaticString & a, const StaticString & b) const {
			return sserialize::unicodeIsSmaller(cbegin(a), cend(a), cbegin(b), cend(b));
		}
	};

	typedef std::unordered_map<StaticString, uint32_t, MyHasher, MyEq> MyMap;
private:
	std::vector<char> m_strData;
	std::vector<StaticString> m_strings;
	MyMap m_map;
private:
	inline const char * cbegin(const StaticString & str) const {
		return &(m_strData[0])+reinterpret_cast<uint64_t>(str.begin());
	}
	inline const char * cend(const StaticString & str) const {
		return &(m_strData[0])+reinterpret_cast<uint64_t>(str.end());
	}
public:
	StringTable() : m_map(0, MyHasher(&m_strData), MyEq(&m_strData)) {}
	~StringTable() {}
	inline SizeType size() const { return (SizeType) m_strings.size(); }
	inline SizeType insert(const std::string & str) {
		SizeType strSize(narrow_check<StringSizeType>(str.size()));
		MyMap::iterator it(m_map.find(StaticString(const_cast<char*>(str.c_str()), strSize, true)));
		if (it != m_map.end()) {
			return it->second;
		}
		else {
			StaticString istr((char*)m_strData.size(), strSize, false);
			m_strData.insert(m_strData.end(), str.cbegin(), str.cend());
			SizeType id = (SizeType)m_strings.size();
			m_strings.push_back(istr);
			m_map[istr] = id;
			return id;
		}
	}
	
	///@param oldToNew an object with operator[] = ;
	template<typename T_OLD_TO_NEW_MAP>
	void sort(T_OLD_TO_NEW_MAP & oldToNew) {
		std::sort(m_strings.begin(), m_strings.end(), MySmaller(&m_strData));
		for(SizeType i(0), s((SizeType)m_strings.size()); i < s; ++i) {
			SizeType & old = m_map[ m_strings[i] ];
			oldToNew[old] = i;
			old = i;
		}
	}

	inline void sort() {
		std::sort(m_strings.begin(), m_strings.end(), MySmaller(&m_strData));
		for(SizeType i(0), s(m_strings.size()); i < s; ++i) {
			m_map[ m_strings[i] ] = i;
		}
	}
	
	inline bool count(const std::string & str) const { return m_map.count(StaticString(const_cast<char*>(str.c_str()), str.size(), true)) > 0; }
	
	inline std::string at(uint32_t id) const {
		if (id < m_strings.size()) {
			const StaticString & str = m_strings[id];
			return std::string( cbegin(str), cend(str));
		}
		return std::string();
	}
	
	uint32_t at(const std::string & str) const {
		MyMap::const_iterator it = m_map.find(StaticString(const_cast<char*>(str.c_str()), str.size(), true));
		if (it != m_map.end()) {
			return it->second;
		}
		throw sserialize::OutOfBoundsException("sserialize::StringTable::at");
	}
	
	///This remapds the string ids, sort has to be called before and you should update your string ids afterwards
	inline UByteArrayAdapter serialize(UByteArrayAdapter & dest) const {
		if (!std::is_sorted(m_strings.begin(), m_strings.end(), MySmaller(&m_strData))) {
			throw sserialize::CreationException("StringTable is not sorted");
		}
		sserialize::Static::ArrayCreator<std::string> ac(dest);
		for(const StaticString & str : m_strings) {
			ac.put(std::string(cbegin(str), cend(str)));
		}
		ac.flush();
		return dest;
	};
	
	void swap(StringTable & other) {
		using std::swap;
		swap(m_map, other.m_map);
		swap(m_strings, other.m_strings);
	}
	
	///@return lower bound for the memory usage
	inline OffsetType memoryUsage() const {
		return m_strData.capacity()*sizeof(char) + m_strings.capacity()*sizeof(StaticString) + m_map.max_size()*(sizeof(MyMap::value_type));
	}
};

}//end namespace

inline void swap(sserialize::StringTable & a, sserialize::StringTable & b) {
	a.swap(b);

}
#endif