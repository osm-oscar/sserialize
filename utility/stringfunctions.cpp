#include <sserialize/utility/stringfunctions.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/vendor/utf8.h>
#include <set>


namespace sserialize {

std::deque<std::string> toLowerCase(std::deque<std::string> & strs) {
	std::set<std::string> lowerCase;
	for(std::deque<std::string> ::iterator it = strs.begin(); it != strs.end(); it++) {
		lowerCase.insert(unicode_to_lower(*it));
	}
	std::deque<std::string> lowerCaseD;
	for(std::set<std::string>::iterator it = lowerCase.begin(); it != lowerCase.end(); it++) {
		lowerCaseD.push_back(*it);
	}
	return lowerCaseD;
}

std::deque<std::string> toUpperCase(std::deque<std::string> & strs) {
	std::set<std::string> upperCase;
	for(std::deque<std::string> ::iterator it = strs.begin(); it != strs.end(); it++) {
		upperCase.insert(unicode_to_upper(*it));
	}
	std::deque<std::string> upperCaseD;
	for(std::set<std::string>::iterator it = upperCase.begin(); it != upperCase.end(); it++) {
		upperCaseD.push_back(*it);
	}
	return upperCaseD;
}

bool backslashEscape(std::string & str, char c) {
	std::string escaped;
	escaped.reserve(str.size());
	
	std::string::const_iterator from(str.begin());
	std::string::const_iterator to(str.begin());
	std::string::const_iterator end(str.end());
	for(; to != end; ++to) {
		if (*to == c) {
			escaped.append(from, to);
			escaped += '\\';
			escaped += *to;
			from = to+1;
		}
	}
	if (escaped.size()) {
		str.swap(escaped);
		return true;
	}
	return false;
}

bool toBool(const std::string& str) {
	return (str == "yes" || str == "true" || str == "1");
}

bool isLowerCase(uint32_t cp) {
	return cp == unicode32_to_lower(cp);
}

bool isUpperCase(uint32_t cp) {
	return cp == unicode32_to_upper(cp);
}

bool hasLowerCase(const std::string& str) {
	std::string::const_iterator strIt(str.begin());
	std::string::const_iterator strEnd(str.end());
	uint32_t ucode;
	while(strIt != strEnd) {
		ucode = utf8::next(strIt, strEnd);
		if (unicode32_to_lower(ucode) == ucode)
			return true;
	}
	return false;
}

bool hasUpperCase(const std::string& str) {
	std::string::const_iterator strIt(str.begin());
	std::string::const_iterator strEnd(str.end());
	uint32_t ucode;
	while(strIt != strEnd) {
		ucode = utf8::next(strIt, strEnd);
		if (unicode32_to_upper(ucode) == ucode)
			return true;
	}
	return false;
}


bool unicodeIsSmaller(const std::string & a, const std::string & b) {
	std::string::const_iterator ait = a.begin();
	std::string::const_iterator bit = b.begin();
	uint32_t acode;
	uint32_t bcode;
	while(ait != a.end() && bit != b.end()) {
		acode = utf8::peek_next(ait, a.end());
		bcode = utf8::peek_next(bit, b.end());
		if (acode < bcode) return true;
		if (acode > bcode) return false;
		utf8::next(ait, a.end());
		utf8::next(bit, b.end());
	}
	//one is a prefix of the other => the shorter one is the smaller one
	return (a.length() < b.length());
}

bool oneIsPrefix(const std::string& a, const std::string& b) {
	std::string::const_iterator ait = a.begin();
	std::string::const_iterator bit = b.begin();
	while(ait != a.end() && bit != b.end()) {
		if (*ait != *bit) {
			return false;
		}
		ait++;
		bit++;
	}
	return true;
}

uint16_t calcLcp(const UByteArrayAdapter & strA, const std::string & strB) {
	uint16_t len = strA.size();
	if (strB.size() < len)
		len = strB.size();
	for(size_t i = 0; i < len; i++) {
		if (strA.at(i) != static_cast<uint8_t>(strB.at(i)))
			return i;
	}
	return len;
}


int8_t compare(const UByteArrayAdapter& strA, const std::string& strB, uint16_t& lcp) {
	uint16_t len = strA.size();
	if (strB.size() < len)
		len = strB.size();
	for(uint16_t i = lcp; i < len; i++, lcp++) {
		if (strA.at(i) < static_cast<uint8_t>(strB.at(i))) {
			return -1;
		}
		if (strA.at(i) > static_cast<uint8_t>(strB.at(i))) {
			return 1;
		}
	}
	if (strA.size() < strB.size()) {
		return -1;
	}
	else if (strA.size() == strB.size()) {
		return 0;
	}
	else {
		return 1;
	}
}

std::deque<std::string> splitLine(const std::string & str, const std::set<char> & seps) {
	std::deque<std::string> splits;
	std::string curStr;
	for(std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
		if (seps.count( *it ) > 0) {
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

}//end namespace