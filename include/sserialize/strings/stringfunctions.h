#ifndef COMMON_STRING_FUNCTIONS
#define COMMON_STRING_FUNCTIONS
#include <deque>
#include <set>
#include <string>
#include <functional>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/vendor/utf8.h>



namespace sserialize {

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

/** @param str: returns true if str is either yes,true,0, otherwise returns false */
bool toBool(const std::string & str);

///a locale unaware version
double stod(const std::string & str);

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

bool isPrefix(const std::string & prefix, const std::string & str);

/** @param strA: string without header */
std::string::size_type calcLcp(const UByteArrayAdapter & strA, const std::string & strB);

/** @return: -1 if strA < strB; 0 if eq; 1 strA > strB 
  * @param lcp: searches with offset of lcp and updates lcp for equal chars
  */
int8_t compare(const UByteArrayAdapter& strA, const std::string& strB, uint16_t& lcp);
#ifdef __LP64__
int8_t compare(const UByteArrayAdapter& strA, const std::string& strB, uint32_t & lcp);
#endif
int8_t compare(const UByteArrayAdapter& strA, const std::string& strB, std::string::size_type & lcp);

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

///Split string into multiple strings at every location of unicode separators, but skip those escaped with a unicode codepoint in escapes
///@param out output iterator accepting std::string
///@return number of consumed chars (NOT codepoints)
template<typename T_OCTET_ITERATOR, typename T_SEPARATOR_SET, typename T_ESCAPES_SET, typename T_OUTPUT_ITERATOR>
std::size_t split(const T_OCTET_ITERATOR & begin, const T_OCTET_ITERATOR & end, 
					const typename std::enable_if<!std::is_integral<T_SEPARATOR_SET>::value, T_SEPARATOR_SET>::type & separators,
					const typename std::enable_if<!std::is_integral<T_ESCAPES_SET>::value, T_ESCAPES_SET>::type & escapes,
					T_OUTPUT_ITERATOR out) {
	std::string curStr;
	T_OCTET_ITERATOR it(begin), prev(begin);
	for(; it != end;) {
		uint32_t cp = utf8::next(it, end);
		if (escapes.count(cp) > 0) {
			if (it == end) {
				continue;
			}
			prev = it;
			utf8::advance(it, 1, end);
		}
		else if (separators.count(cp) > 0) {
			*out = curStr;
			++out;
			curStr.clear();
			prev = it;
			continue;
		}
		curStr.append(prev, it);
		prev = it;
	}
	if(curStr.length()) {
		*out = curStr;
	}
	return end-it;
}

template<typename T_RETURN_CONTAINER, typename T_SEPARATOR_SET, typename T_ESCAPES_SET>
T_RETURN_CONTAINER split(const std::string & str,
						const typename std::enable_if<!std::is_integral<T_SEPARATOR_SET>::value, T_SEPARATOR_SET>::type & separators,
						const typename std::enable_if<!std::is_integral<T_ESCAPES_SET>::value, T_ESCAPES_SET>::type & escapes) {
	typedef std::insert_iterator<T_RETURN_CONTAINER> MyInsertIterator;
	T_RETURN_CONTAINER dest;
	std::insert_iterator<T_RETURN_CONTAINER> insertIt(dest, dest.end());
	split<std::string::const_iterator, T_SEPARATOR_SET, T_ESCAPES_SET, MyInsertIterator>(str.cbegin(), str.cend(), separators, escapes, insertIt);
	return dest;
}

template<typename T_RETURN_CONTAINER>
T_RETURN_CONTAINER split(const std::string & str, uint32_t separator, uint32_t escape) {
	OneValueSet<uint32_t> sepSet(separator);
	OneValueSet<uint32_t> escapeSet(escape);
	return split< T_RETURN_CONTAINER, OneValueSet<uint32_t>, OneValueSet<uint32_t> >(str, sepSet, escapeSet);
}

template<typename T_RETURN_CONTAINER>
T_RETURN_CONTAINER split(const std::string & str, uint32_t separator) {
	return split<T_RETURN_CONTAINER>(str, separator, 0xFFFFFFFF);
}


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

class AsciiCharEscaper final {
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
	
	static AsciiCharEscaper regexEscaper();
};

class JsonEscaper final {
	AsciiCharEscaper m_escaper;
	char m_escapeMap[128];
public:
	JsonEscaper();
	~JsonEscaper() {}
	template<typename T_SRC_IT, typename T_OUTPUT_IT>
	void escape(T_SRC_IT begin, T_SRC_IT end, T_OUTPUT_IT out) const {
		for(; begin != end; ++begin, ++out) {
			char c = *begin;
			if (m_escaper.escapeChar(c)) {
				*out = '\\';
				++out;
				*out = m_escapeMap[(unsigned char)c];
			}
			else {
				*out = c;
			}
		}
	}
	std::string escape(const std::string & str) const;
};

//A very simple, invalid XmlEscaper which only escapes ><'"&. It does not check for explicit unicode point definitions
class XmlEscaper final {
private:
private:
	AsciiCharEscaper m_escaper;
	const char * m_escapeMap[128];
public:
	XmlEscaper();
	~XmlEscaper() {}
	template<typename T_SRC_IT, typename T_OUTPUT_IT>
	void escape(T_SRC_IT begin, T_SRC_IT end, T_OUTPUT_IT out) const {
		for(; begin != end; ++begin, ++out) {
			char c = *begin;
			if (m_escaper.escapeChar(c)) {
				*out = '\\';
				++out;
				for(const char * it(m_escapeMap[(unsigned char)c]); *it != 0; ++it) {
					*out = *it;
					++out;
				}
			}
			else {
				*out = c;
			}
		}
	}
	std::string escape(const std::string & str) const;
};

}//end namespace
#endif
