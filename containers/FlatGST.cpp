#include "FlatGST.h"
#include <set>
#include <sserialize/vendor/libs/utf8/source/utf8.h>
#include <utility/unicode_case_functions.h>


namespace sserialize { namespace dynamic {

GeneralizedSuffixArray::GeneralizedSuffixArray() : m_caseSensitive(false) {

}


GeneralizedSuffixArray::GeneralizedSuffixArray(const std::map<unsigned int, std::string> & strIdToStr, bool caseSensitive) :
m_caseSensitive(caseSensitive)
{
	constructGSA(strIdToStr, caseSensitive);
}

GeneralizedSuffixArray::~GeneralizedSuffixArray() {}


void GeneralizedSuffixArray::constructGSA(const std::map< unsigned int, std::string > & strIdToStr, bool caseSensitive) {
	
	std::map< std::string, GeneralizedSuffixArray::GSAInfoElement > gsa;
	
	std::string insStr;
	bool insStrIsInStrDB = false;
	for(std::map< unsigned int, std::string >::const_iterator it = strIdToStr.begin(); it != strIdToStr.end(); ++it) {
		insStrIsInStrDB = false;
		if (caseSensitive)
			insStr = it->second;
		else
			insStr = unicode_to_lower(it->second);
		
		if (gsa.count(insStr) > 0) {
			gsa.at(insStr).exactStrIds.push_back(it->first);
		}
		else {
			insStrIsInStrDB = true;
			GSAInfoElement elem;
			elem.strId = m_strDB.size();
			m_strDB.push_back(insStr);
			elem.pos = 0;
			elem.exactStrIds.push_back(it->first);
		}
		uint16_t pos = 0;
		while (insStr.size() > 0) {
			if (gsa.count(insStr) > 0) {
				gsa.at(insStr).suffixStrIds.push_back(it->first);
			}
			else {
				if (!insStrIsInStrDB) {
					m_strDB.push_back(insStr);
				}
				GSAInfoElement elem;
				elem.strId = m_strDB.size()-1;
				elem.pos = pos;
				elem.suffixStrIds.push_back(it->first);
			}
			std::string::iterator it = insStr.begin();
			std::string::iterator insStrEnd = insStr.end();
			utf8::next<std::string::iterator>(it, insStrEnd);
			pos += std::distance<std::string::const_iterator>(insStr.begin(), it);
			insStr.erase(insStr.begin(), it);
		}
	}
	for(std::map< std::string, GeneralizedSuffixArray::GSAInfoElement >::const_iterator it = gsa.begin(); it != gsa.end(); ++it) {
		m_gsa.push_back(it->second);
	}
}

GeneralizedSuffixArray::StringComparisonType
GeneralizedSuffixArray::compare(const std::string& str, uint32_t gsaPos, uint16_t & lcp) const {
	const std::string & gsaStr = m_strDB.at(m_gsa.at(gsaPos).strId);
	
	uint32_t gsaStrPos = m_gsa.at(gsaPos).pos + lcp;
	uint32_t strPos = lcp;
	while(gsaStrPos < gsaStr.size() && strPos < str.size()) {
		if (str.at(strPos) < gsaStr.at(gsaStrPos))
			return SC_SMALLER;
		else if (str.at(strPos) > gsaStr.at(gsaStrPos)) {
			return SC_LARGER;
		}
		else {
			gsaStrPos++;
			strPos++;
			lcp++;
		}
	}
	if (strPos == str.size()) {
		if (gsaStrPos < gsaStr.size()) {
			return SC_SMALLER;
		}
		return SC_EQUAL;
	}
	return SC_LARGER;
}

inline uint16_t min(const uint16_t a, const uint16_t b) {
	return (a < b ? a : b);
}

std::unordered_set<uint32_t>
GeneralizedSuffixArray::match(const std::string& suffix, StringCompleter::QuerryType qt) {
	uint16_t leftLcp, rightLcp;
	uint16_t midLcp = 0;
	compare(suffix, 0, leftLcp);
	compare(suffix, m_gsa.size()-1, rightLcp);
	
	int32_t len = m_gsa.size();
	if (len == 0)
		return std::unordered_set<uint32_t>();
	int32_t left = 0;
	int32_t right = len-1;
	int32_t mid = (right-left)/2+left;
	while( right - left > 1 ) {
		StringComparisonType sc = compare(suffix, mid, midLcp);
		if (sc == SC_EQUAL)
			break;
		if (sc == SC_LARGER) { // key should be to the right
			left = mid;
			leftLcp = midLcp;
			midLcp = min(rightLcp, midLcp);
		}
		else { // key should be to the left of mid
			right = mid;
			rightLcp = midLcp;
			midLcp = min(midLcp, leftLcp);
		}
		mid = (right-left)/2+left;
	}
	StringComparisonType sc = compare(suffix, mid, midLcp);
	std::unordered_set<uint32_t> ret;
	if (sc == SC_EQUAL) {
		if (qt & StringCompleter::QT_EXACT) {
			std::deque<uint32_t> & exactStrIds = m_gsa[mid].exactStrIds;
			for(size_t i = 0; i < exactStrIds.size(); i++) {
				ret.insert(exactStrIds[i]);
			}
		}
		if (qt & StringCompleter::QT_SUFFIX) {
			std::deque<uint32_t> & suffixStrIds = m_gsa[mid].suffixStrIds;
			for(size_t i = 0; i < suffixStrIds.size(); i++) {
				ret.insert(suffixStrIds[i]);
			}
		}
	}
	return ret;
}

StringCompleter::SupportedQuerries GeneralizedSuffixArray::getSupportedQuerries() {
	if (m_caseSensitive) {
		return (StringCompleter::SupportedQuerries) (StringCompleter::SQ_EPSP | StringCompleter::SQ_CASE_SENSITIVE);
	}
	else {
		return (StringCompleter::SupportedQuerries) (StringCompleter::SQ_EPSP | StringCompleter::SQ_CASE_INSENSITIVE);
	}
}


}}//end namespace