#include <sserialize/Static/GeneralizedSuffixArray.h>

namespace sserialize {
namespace Static {

GeneralizedSuffixArray::GeneralizedSuffixArray() :
m_flags(F_NONE)
{
}

GeneralizedSuffixArray::GeneralizedSuffixArray(const UByteArrayAdapter& data) :
m_flags((Flags)data.getUint8(1)),
m_stable(data),
m_gsa(data + (2+m_stable.getSizeInBytes()) ),
m_vStore(data + (2+m_stable.getSizeInBytes()+m_gsa.getSizeInBytes()))
{
}

GeneralizedSuffixArray::~GeneralizedSuffixArray() {}

StringCompleter::SupportedQuerries GeneralizedSuffixArray::getSupportedQuerries() const {
	uint32_t sq = StringCompleter::SQ_EP;
	if (m_flags & F_SUFFIX_ARRAY)
		sq |= StringCompleter::SQ_SSP;
		
	if (m_flags & F_CASE_SENSITIVE)
		sq |= StringCompleter::SQ_CASE_SENSITIVE;
	else
		sq |= StringCompleter::SQ_CASE_INSENSITIVE;
		
	return (StringCompleter::SupportedQuerries) sq;
}

void indexIntoSet(const ItemIndex & idx, std::unordered_set<uint32_t> & dest) {
	uint32_t size = idx.size();
	for(uint32_t i = 0; i < size; i++) {
		dest.insert( idx.at(i) );
	}
}

std::unordered_set< uint32_t >
GeneralizedSuffixArray::match(const std::string& str, sserialize::StringCompleter::QuerryType qt) const {
	std::unordered_set<uint32_t> ret;
	int32_t begin = lowerBound(str, qt);
	int32_t end = upperBound(str, qt);
	if (begin < 0 || end < 0)
		return ret;
	
	if (qt & StringCompleter::QT_EXACT) { //This is only a single hit => begin==end
		indexIntoSet(m_vStore.at(begin).first(), ret);
	}
	else if (qt & StringCompleter::QT_SUFFIX) { //This is only a single hit => begin==end
		indexIntoSet(m_vStore.at(begin).second(), ret);
	}
	else if (qt & StringCompleter::QT_PREFIX) {
		for(size_t i = begin; i < end; i++) {
			indexIntoSet(m_vStore.at(i).first(), ret);
		}
	}
	else if (qt & StringCompleter::QT_SUFFIX_PREFIX) {
		for(size_t i = begin; i < end; i++) {
			indexIntoSet(m_vStore.at(i).second(), ret);
		}
	}
	return ret;
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

UByteArrayAdapter GeneralizedSuffixArray::gsaStringAt(uint32_t pos) const {
	Pair<uint32_t, uint16_t> item(m_gsa.at(pos));
	return m_stable.dataAt(item.first()).getStringData()+item.second();
}


int8_t
GeneralizedSuffixArray::compare(const UByteArrayAdapter& strA, const std::string& strB, uint16_t& lcp) const {
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

int32_t
GeneralizedSuffixArray::lowerBound(const std::string& str, StringCompleter::QuerryType qt) const {
	uint32_t left = 0;
	uint32_t right = m_gsa.size()-1;
	uint32_t mid = 0;

	uint16_t lLcp = calcLcp(gsaStringAt(left), str);
	if (lLcp == str.size()) //first is match
		return 0;
	uint16_t rLcp = calcLcp(gsaStringAt(right), str);
	uint16_t mLcp = 0;
	int8_t cmp;
	while(right-left > 1) {
		mid = (right-left)/2 + left;
		cmp = compare(gsaStringAt(mid), str, mLcp);
		if (cmp == -1) { // mid is smaller than str
			lLcp = mLcp;
			mLcp = std::min(lLcp, rLcp);
			left = mid;
		}
		else if (cmp == 1) {//mid is larger than str
			rLcp = mLcp;
			mLcp = std::min(lLcp, rLcp);
			right = mid;
		}
		else { //equal, mid points to correct element
			break;
		}
	}
	//mid points to last element and cmp contains the equality relation
	if (mLcp == str.size()) {
		if (qt & StringCompleter::QT_PREFIX || qt & StringCompleter::QT_SUFFIX_PREFIX || gsaStringAt(mid).size() == str.size()) {
			return mid;
		}
	}
	else
		return -1;
}

int32_t
GeneralizedSuffixArray::upperBound(const std::string& str, StringCompleter::QuerryType qt) const {

}




}}//end namespce