#ifndef COMMON_STRING_FUNCTIONS
#define COMMON_STRING_FUNCTIONS
#include <string>
#include <deque>
#include <set>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/vendor/utf8.h>
#include <iostream>


namespace sserialize {

std::deque<std::string> toLowerCase(const std::deque<std::string> & strs);
std::deque<std::string> toUpperCase(const std::deque<std::string> & strs);

/** @param str: returns true if str is either yes,true,0, otherwise returns false */
bool toBool(const std::string & str);


inline bool isValidUtf8(const std::string & str) {
	return utf8::is_valid(str.cbegin(), str.cend());
}

template<typename CPIterator>
std::string stringFromUnicodePoints(CPIterator begin, const CPIterator & end) {
	std::string ret;
	while(begin != end) {
		utf8::append(*begin, std::back_insert_iterator<std::string>(ret));
		++begin;
	}
	return ret;
}

bool backslashEscape(std::string & str, char c);

bool isLowerCase(uint32_t cp);
bool isUpperCase(uint32_t cp);

bool hasLowerCase(const std::string & str);
bool hasUpperCase(const std::string & str);

/** Returns if a < b **/
bool unicodeIsSmaller(const std::string& a,const std::string& b);

template<typename T_OCTET_ITERATOR1, typename T_OCTET_ITERATOR2>
inline bool unicodeIsSmaller(T_OCTET_ITERATOR1 itA, const T_OCTET_ITERATOR1 & endA, T_OCTET_ITERATOR2 itB, const T_OCTET_ITERATOR2 & endB) {
	while (itA != endA && itB != endB) {
		uint32_t aCode = utf8::next(itA, endA);
		uint32_t bCode = utf8::next(itB, endB);
		if (aCode < bCode)
			return true;
		else if (bCode < aCode)
			return false;
	}
	return (itB != endB);
}

bool oneIsPrefix(const std::string& a, const std::string& b);

/** @param strA: string without header */
uint16_t calcLcp(const UByteArrayAdapter & strA, const std::string & strB);

/** @return: -1 if strA < strB; 0 if eq; 1 strA > strB 
  * @param lcp: searches with offset of lcp and updates lcp for equal chars
  */
int8_t compare(const UByteArrayAdapter& strA, const std::string& strB, uint16_t& lcp);

/** Returns the number of utf8 characters (NOT encoding chars) */ 
template<typename octetIterator>
uint32_t utf8CharCount(octetIterator begin, const octetIterator & end) {
	uint32_t count = 0;
	while(begin != end) {
		utf8::next(begin, end);
		++count;
	}
	return count;
}

/** splits the string at the given spearator */
template<char TSEP>
std::deque<std::string> splitLine(const std::string & str) {
	std::deque<std::string> splits;
	std::string curStr;
	for(std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
		if (*it == TSEP) {
			splits.push_back(curStr);
			curStr = std::string();
		}
		else {
			curStr.push_back(*it);
		}
	}
	if(curStr.length())
		splits.push_back(curStr);
	return splits;
}

template<char TSEP>
std::vector<std::string> splitLineVec(const std::string & str) {
	std::vector<std::string> splits;
	std::string curStr;
	for(std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
		if (*it == TSEP) {
			splits.push_back(curStr);
			curStr = std::string();
		}
		else {
			curStr.push_back(*it);
		}
	}
	if(curStr.length())
		splits.push_back(curStr);
	return splits;
}

///split string @str into multiple strings at seperators and use escapes as escape char, NOT unicode aware!
template<typename T_RETURN_CONTAINER>
T_RETURN_CONTAINER split(const std::string & str, const char & separators, const char & escapes) {
	T_RETURN_CONTAINER splits;
	std::string curStr;
	for(std::string::const_iterator it(str.cbegin()), end(str.cend()); it != end; ++it) {
		if (*it == escapes) {
			++it;
			if (it != end) {
				curStr.push_back(*it);
			}
		}
		else if (separators == *it) {
			splits.insert(splits.end(), curStr);
			curStr.clear();
		}
		else {
			curStr.push_back(*it);
		}
	}
	if(curStr.length())
		splits.push_back(curStr);
	return splits;
}

///split string @str into multiple strings at seperators and use escapes as escape char, NOT unicode aware!
template<typename T_RETURN_CONTAINER, typename T_SEPARATOR_SET, typename T_ESCAPES_SET>
T_RETURN_CONTAINER split(const std::string & str, const T_SEPARATOR_SET & separators, const T_ESCAPES_SET & escapes) {
	T_RETURN_CONTAINER splits;
	std::string curStr;
	for(std::string::const_iterator it(str.cbegin()), end(str.cend()); it != end; ++it) {
		if (escapes.count( *it ) > 0) {
			++it;
			if (it != end) {
				curStr.push_back(*it);
			}
		}
		else if (separators.count( *it ) > 0) {
			splits.insert(splits.end(), curStr);
			curStr.clear();
		}
		
		else {
			curStr.push_back(*it);
		}
	}
	if(curStr.length())
		splits.push_back(curStr);
	return splits;
}

template<typename TVALUE>
struct OneValueSet final {
	TVALUE m_value;
public:
	OneValueSet() {}
	OneValueSet(const TVALUE & v) : m_value(v) {}
	~OneValueSet() {}
	inline bool count(const TVALUE & v) const { return v == m_value; }
	inline TVALUE & value() { return m_value; }
	inline const TVALUE & value() const { return m_value; }
};

/** splits the string at the given spearator */
std::deque<std::string> splitLine(const std::string & str, const std::set<char> & seps);

///move strIt to the beginning of the next suffix string
template<typename T_OCTET_ITERATOR, typename T_SEPARATOR_SET>
void nextSuffixString(T_OCTET_ITERATOR & strIt, const T_OCTET_ITERATOR & strEnd, const T_SEPARATOR_SET & separators) {
	if (separators.size()) {
		while (strIt != strEnd) {
			if (separators.count( utf8::next(strIt, strEnd) ) > 0)
				break;
		}
	}
	else if (strIt != strEnd) {
		utf8::next(strIt, strEnd);
	}
}

///Return all suffixes of @str including @str itself
template<typename T_RETURN_CONTAINER, typename T_STRING_TYPE, typename T_SEPARATOR_SET>
T_RETURN_CONTAINER suffixStrings(const T_STRING_TYPE & str, const T_SEPARATOR_SET & separators) {
	T_RETURN_CONTAINER ss;
	typename T_STRING_TYPE::const_iterator strIt(str.cbegin()), strEnd(str.cend());
	while(strIt != strEnd) {
		ss.insert(strIt, strEnd);
		nextSuffixString(strIt, strEnd, separators);
	}
	return ss;
}

class AsciiCharEscaper {
	uint64_t m_repChars[2];
public:
	AsciiCharEscaper() : m_repChars{0,0} {}
	~AsciiCharEscaper() {}
	template<typename T>
	AsciiCharEscaper(std::initializer_list<T> l) : m_repChars{0,0}  {
		setEscapeChars(l.begin(), l.end());
	}
	template<typename T_UNICODE_POINT_ITERATOR>
	AsciiCharEscaper(const T_UNICODE_POINT_ITERATOR & begin, const T_UNICODE_POINT_ITERATOR & end) : m_repChars{0, 0} {
		setEscapeChars(begin, end);
	}
	inline bool escapeChar(char c) const {
		return (c >= 0) && (c > 63 ? (m_repChars[1] & (static_cast<uint64_t>(1) << (c-64))) : (m_repChars[0] & (static_cast<uint64_t>(1) << (c))));
	}
	void setEscapeChar(char c);
	template<typename T_IT>
	void setEscapeChars(T_IT begin, T_IT end) {
		for(; begin != end; ++begin)
			setEscapeChar(*begin);
	}
	
	template<typename T_SRC_IT, typename T_OUTPUT_IT>
	void escape(T_SRC_IT begin, T_SRC_IT end, T_OUTPUT_IT out) const {
		for(; begin != end; ++begin, ++out) {
			char c = *begin;
			if (escapeChar(c)) {
				*out = '\\';
				++out;
			}
			*out = c;
		}
	}
	std::string escape(const std::string & str) const;
};

}//end namespace
#endif