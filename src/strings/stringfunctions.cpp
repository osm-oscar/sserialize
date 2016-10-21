#include <sserialize/strings/stringfunctions.h>
#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/checks.h>
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

std::string::size_type calcLcp(const UByteArrayAdapter & strA, const std::string & strB) {
	std::string::size_type len = (std::string::size_type) strA.size();
	if (strB.size() < len) {
		len = strB.size();
	}
	for(std::string::size_type i(0); i < len; ++i) {
		if (strA.at(i) != static_cast<uint8_t>(strB.at(i))) {
			return i;
		}
	}
	return len;
}

int8_t compare(const UByteArrayAdapter& strA, const std::string& strB, std::string::size_type & lcp) {
	UByteArrayAdapter::OffsetType len = strA.size();
	if (strB.size() < len) {
		len = strB.size();
	}
	for(UByteArrayAdapter::OffsetType i = lcp; i < len; i++, lcp++) {
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

int8_t compare(const UByteArrayAdapter& strA, const std::string& strB, uint16_t & lcp) {
	std::string::size_type tmpLcp;
	int8_t ret = compare(strA, strB, tmpLcp);
	lcp = narrow_check<uint16_t>(tmpLcp);
	return ret;
}

#ifdef __LP64__
int8_t compare(const UByteArrayAdapter& strA, const std::string& strB, uint32_t & lcp) {
	std::string::size_type tmpLcp;
	int8_t ret = compare(strA, strB, tmpLcp);
	lcp = narrow_check<uint32_t>(tmpLcp);
	return ret;
}
#endif


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

AsciiCharEscaper AsciiCharEscaper::regexEscaper() {
	return AsciiCharEscaper({'(', ')', '[', ']', '{', '}','|', '*', '.', '+', '?', '$', '^', '\\', ':', '='});
}

JsonEscaper::JsonEscaper() :
//escapes as per http://stackoverflow.com/questions/7123908/javascript-json-parser-that-tells-error-position
m_escaper({'"', '\\', '/', '\b', '\f', '\n', '\r', '\t'})
{
	m_escapeMap[(uint32_t)'"'] = '"';
	m_escapeMap[(uint32_t)'\\'] = '\\';
	m_escapeMap[(uint32_t)'/'] = '/';
	m_escapeMap[(uint32_t)'\b'] = 'b';
	m_escapeMap[(uint32_t)'\f'] = 'f';
	m_escapeMap[(uint32_t)'\n'] = 'n';
	m_escapeMap[(uint32_t)'\r'] = 'r';
	m_escapeMap[(uint32_t)'\t'] = 't';
}

std::string JsonEscaper::escape(const std::string & str) const {
	std::string tmp;
	escape(str.cbegin(), str.cend(), std::back_insert_iterator<std::string>(tmp));
	return tmp;
}



XmlEscaper::XmlEscaper() :
m_escaper({'"', '\'', '&', '<', '>'})
{
	m_escapeMap[(std::size_t)'"'] = "&quot;";
	m_escapeMap[(std::size_t)'\''] = "&apos;";
	m_escapeMap[(std::size_t)'&'] = "&amp;";
	m_escapeMap[(std::size_t)'<'] = "&lt;";
	m_escapeMap[(std::size_t)'>'] = "&gt;";
}

std::string XmlEscaper::escape(const std::string & str) const {
	std::string tmp;
	escape(str.cbegin(), str.cend(), std::back_insert_iterator<std::string>(tmp));
	return tmp;
}

}//end namespace