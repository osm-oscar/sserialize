#include <sserialize/utility/stringfunctions.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/vendor/utf8.h>
#include <set>


namespace sserialize {

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
	uint32_t ccode, lcode;
	while(strIt != strEnd) {
		ccode = utf8::next(strIt, strEnd);
		lcode = unicode32_to_lower(ccode);
		if (! (ccode == lcode) ) {
			return true;
		}
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

void AsciiCharEscaper::setEscapeChar(char c) {
	if (c >= 0) {
		if (c > 63) {
			c -= 64;
			m_repChars[1] |= static_cast<uint64_t>(1) << c;
		}
		else {
			m_repChars[0] |= static_cast<uint64_t>(1) << c;
		}
	}
}

std::string AsciiCharEscaper::escape(const std::string & str) const {
	std::string tmp;
	tmp.reserve(str.size());
	std::back_insert_iterator<std::string> outIt(tmp);
	escape(str.cbegin(), str.cend(), outIt);
	return tmp;
}

}//end namespace