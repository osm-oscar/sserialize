#include <sserialize/Static/StringTable.h>
#include <iostream>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/find_key_in_array_functions.h>

namespace sserialize {
namespace Static {

StringTable::StringTable() :
RCWrapper< sserialize::Static::Array<std::string> >(new sserialize::Static::Array< std::string >())
{}

StringTable::StringTable(const UByteArrayAdapter& data) :
RCWrapper< sserialize::Static::Array<std::string> >(new sserialize::Static::Array< std::string >(data) )
{}

StringTable::~StringTable() {}

StringTable::StringTable(Array< std::string >* data): RCWrapper< sserialize::Static::Array< std::string > >(data)
{}

StringTable::StringTable(const StringTable& other) : RCWrapper< sserialize::Static::Array<std::string> >(other)
{}

StringTable& StringTable::operator=(const StringTable& other) {
	RCWrapper< sserialize::Static::Array<std::string> >::operator=(other);
	return *this;
}


typedef bool (*StrMatchFunction)(const std::string& searchStr, const UByteArrayAdapter & str);

bool matchPrefix(const std::string& searchStr, const UByteArrayAdapter & str) {
	if (searchStr.size() > str.size())
		return false;
	uint32_t strSize = searchStr.size();
	for(uint16_t i = 0; i < strSize; i++) {
		if (str.at(i) != static_cast<uint8_t>( searchStr[i] ))
			return false;
	}
	return true;
}

bool matchPrefixCIS(const std::string& searchStr, const UByteArrayAdapter & str) {
	UByteArrayAdapter strIt(str);
	UByteArrayAdapter strEnd(str + str.size());

	std::string::const_iterator searchStrIt = searchStr.begin();
	std::string::const_iterator searchStrEnd = searchStr.end();

	while(searchStrIt != searchStrEnd && strIt.size() > 0) {
		uint32_t searchStrItUCode = utf8::next(searchStrIt, searchStrEnd);
		uint32_t strItUCode = unicode32_to_lower( utf8::next(strIt, strEnd) );
		if (searchStrItUCode != strItUCode)
				return false;
	}
	return searchStrIt == searchStrEnd;
}

bool matchExact(const std::string& searchStr, const UByteArrayAdapter & str) {
	if (searchStr.size() != str.size() || searchStr.size() > str.size())
		return false;
	return matchPrefix(searchStr, str);
}

bool matchExactCIS(const std::string& searchStr, const UByteArrayAdapter & str) {
	if (searchStr.size() == 0 && str.size() != 0)
		return false;
	UByteArrayAdapter strIt(str);
	UByteArrayAdapter strEnd(str + str.size());

	std::string::const_iterator searchStrIt = searchStr.begin();
	std::string::const_iterator searchStrEnd = searchStr.end();

	while(searchStrIt != searchStrEnd && strIt.size() > 0) {
		uint32_t searchStrItUCode = utf8::peek_next(searchStrIt, searchStrEnd);
		uint32_t strItUCode = unicode32_to_lower( utf8::peek_next(strIt, strEnd) );
		if (searchStrItUCode != strItUCode)
				return false;
		utf8::next(searchStrIt, searchStrEnd);
		utf8::next(strIt, strEnd);
	}
	return (searchStrIt == searchStrEnd && strIt.size() == 0);
}

bool matchSuffix(const std::string& searchStr, const UByteArrayAdapter & str) {
	if (searchStr.size() == 0)
		return true;
	if (searchStr.size() > str.size() || str.size() == 0)
		return false;

	std::string::const_reverse_iterator searchStrIt = searchStr.rbegin();
	std::string::const_reverse_iterator searchStrEnd = searchStr.rend();

	int32_t strPos = str.size()-1;
	while (searchStrIt != searchStrEnd && strPos >= 0) {
		if (static_cast<uint8_t>(*searchStrIt) != str.at(strPos))
			return false;
		strPos--;
		++searchStrIt;
	}
	return true;
}

bool matchSuffixCIS(const std::string& searchStr, const UByteArrayAdapter & str) {
	if (searchStr.size() == 0)
		return true;
	if (str.size() == 0)
		return false;

	UByteArrayAdapter strBegin = str;
	UByteArrayAdapter strEnd( str.end() );
	
	std::string::const_iterator searchStrBegin = searchStr.begin();
	std::string::const_iterator searchStrEnd = searchStr.end();

	uint32_t searchStrItUCode = utf8::prior(searchStrEnd, searchStrBegin);
	uint32_t strItUCode = unicode32_to_lower( utf8::prior(strEnd, strBegin) );
	while (searchStrBegin != searchStrEnd && strBegin != strEnd) {
		if (searchStrItUCode != strItUCode)
			return false;
		searchStrItUCode = utf8::prior(searchStrEnd, searchStrBegin);
		strItUCode = unicode32_to_lower( utf8::prior(strEnd, strBegin) );

	}
	return (searchStrItUCode == strItUCode && searchStrBegin == searchStrEnd);
}

bool matchSuffixPrefix(const std::string& searchStr, const UByteArrayAdapter & str) {
	if (searchStr.size() > str.size())
		return false;

	uint16_t searchStrPos = 0;
	uint16_t strPos = 0;
	for(; strPos < str.size() && searchStrPos < searchStr.size(); strPos++) {
		if (static_cast<uint8_t>(searchStr[searchStrPos]) != str[strPos]) {
			searchStrPos = 0;
		}
		else {
			searchStrPos++;
		}
	}
	return (searchStrPos == searchStr.size());
}

bool matchSuffixPrefixCIS(const std::string& searchStr, const UByteArrayAdapter & str) {
	if (searchStr.size() == 0)
		return true;
	if (str.size() == 0)
		return false;
	UByteArrayAdapter strIt(str);
	UByteArrayAdapter strEnd(str + str.size());

	std::string::const_iterator searchStrIt = searchStr.begin();
	std::string::const_iterator searchStrEnd = searchStr.end();

	while(searchStrIt != searchStrEnd && strIt.size() > 0) {
		uint32_t searchStrItUCode = utf8::peek_next(searchStrIt, searchStrEnd);
		uint32_t strItUCode = unicode32_to_lower( utf8::peek_next(strIt, strEnd) );
		if (searchStrItUCode != strItUCode) {
			searchStrIt = searchStr.begin();
		}
		else {
			utf8::next(searchStrIt, searchStrEnd);
		}
		utf8::next(strIt, strEnd);
	}
	return (searchStrIt == searchStrEnd);
}

std::unordered_set< uint32_t > StringTable::find(const std::string& searchStr, sserialize::StringCompleter::QuerryType qtype) const {
	std::unordered_set< uint32_t > ret;
	if (searchStr.empty())
		return std::unordered_set< uint32_t >();
	find(ret, searchStr, qtype);
	return ret;
}

void 
StringTable::find(std::unordered_set< uint32_t >& ret, std::string searchStr, sserialize::StringCompleter::QuerryType qtype) const {
	StrMatchFunction strMatchFunc;

	if (qtype & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
		searchStr = unicode_to_lower(searchStr);
		if (qtype & sserialize::StringCompleter::QT_EXACT) {
			strMatchFunc = &matchExactCIS;
		}
		else if (qtype & sserialize::StringCompleter::QT_PREFIX) {
			strMatchFunc = &matchPrefixCIS;
		}
		else if (qtype & sserialize::StringCompleter::QT_SUFFIX) {
			strMatchFunc = &matchSuffixCIS;
		}
		else {
			strMatchFunc = &matchSuffixPrefixCIS;
		}
	}
	else {
		if (qtype & sserialize::StringCompleter::QT_EXACT) {
			strMatchFunc = &matchExact;
		}
		else if (qtype & sserialize::StringCompleter::QT_PREFIX) {
			strMatchFunc = &matchPrefix;
		}
		else if (qtype & sserialize::StringCompleter::QT_SUFFIX) {
			strMatchFunc = &matchSuffix;
		}
		else {
			strMatchFunc = &matchSuffixPrefix;
		}
	}
	UByteArrayAdapter strs = data();
	for(uint32_t i = 0; i < size(); i++) {
		if ( (*strMatchFunc)(searchStr, strs.getStringData()) ) {
			ret.insert(i);
		}
	}
}

bool StringTable::match(uint32_t stringId, const std::string & searchStr, sserialize::StringCompleter::QuerryType qtype) const {
	if (size() < stringId)
		return false;

	StrMatchFunction strMatchFunc;
		
	if (qtype & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
		if (qtype & sserialize::StringCompleter::QT_EXACT) {
			strMatchFunc = &matchExactCIS;
		}
		else if (qtype & sserialize::StringCompleter::QT_PREFIX) {
			strMatchFunc = &matchPrefixCIS;
		}
		else if (qtype & sserialize::StringCompleter::QT_SUFFIX) {
			strMatchFunc = &matchSuffixCIS;
		}
		else {
			strMatchFunc = &matchSuffixPrefixCIS;
		}
	}
	else {
		if (qtype & sserialize::StringCompleter::QT_EXACT) {
			strMatchFunc = &matchExact;
		}
		else if (qtype & sserialize::StringCompleter::QT_PREFIX) {
			strMatchFunc = &matchPrefix;
		}
		else if (qtype & sserialize::StringCompleter::QT_SUFFIX) {
			strMatchFunc = &matchSuffix;
		}
		else {
			strMatchFunc = &matchSuffixPrefix;
		}
	}

	return  (*strMatchFunc)(searchStr, dataAt(stringId).getStringData());
}

std::ostream& StringTable::printStats(std::ostream& out) const {
	out << "Static::StringTable::Stats->BEGIN" << std::endl;
	out << "size: " << size() << std::endl;
	out << "storage size: " << getSizeInBytes() << std::endl;
	if (size()) {
		uint32_t longestStringLength = 0;
		uint32_t totallen = 0;
		for(uint32_t i = 0; i <size(); i++) {
			uint32_t len = at(i).size();
			totallen += len;
			if (len > longestStringLength)
				longestStringLength = len;
		}
		out << "longest string length: " << longestStringLength << std::endl;
		out << "Average string length: " << (double)totallen/size() << std::endl;
	}
	out << "Static::StringTable::Stats->END" << std::endl;
	return out;
}

FrequencyStringTable::FrequencyStringTable() : StringTable() {}
FrequencyStringTable::FrequencyStringTable(const UByteArrayAdapter & data) : StringTable(data) {}
FrequencyStringTable::FrequencyStringTable(const FrequencyStringTable & other) : StringTable(other) {}
FrequencyStringTable::~FrequencyStringTable() {}
FrequencyStringTable & FrequencyStringTable::operator=(const FrequencyStringTable & other) {
	StringTable::operator=(other);
	return *this;
}

SortedStringTable::SortedStringTable() : StringTable() {}
SortedStringTable::SortedStringTable(const UByteArrayAdapter & data) : StringTable(data) {}
SortedStringTable::SortedStringTable(const sserialize::Static::SortedStringTable & other) : StringTable(other) {}
SortedStringTable::~SortedStringTable() {}
int32_t SortedStringTable::find(const std::string & value) const {
	return binarySearchKeyInArray(*priv(), value);
}

}}//end namespace
