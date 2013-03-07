#ifndef COMMON_STRING_FUNCTIONS
#define COMMON_STRING_FUNCTIONS
#include <string>
#include <deque>
#include <set>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/vendor/utf8.h>


namespace sserialize {

std::deque<std::string> toLowerCase(const std::deque<std::string> & strs);
std::deque<std::string> toUpperCase(const std::deque<std::string> & strs);

/** @param str: returns true if str is either yes,true,0, otherwise returns false */
bool toBool(const std::string & str);

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

/** splits the string at the given spearator */
std::deque<std::string> splitLine(const std::string & str, const std::set<char> & seps);

}//end namespace
#endif