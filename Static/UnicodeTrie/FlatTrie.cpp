#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/utility/stringfunctions.h>

namespace sserialize {
namespace Static {
namespace UnicodeTrie {

FlatTrieBase::FlatTrieBase() {}

FlatTrieBase::FlatTrieBase(const sserialize::UByteArrayAdapter & src) :
m_strData(src, 1+UByteArrayAdapter::OffsetTypeSerializedLength(), src.getOffset(1)),
m_trie(src+(1+UByteArrayAdapter::OffsetTypeSerializedLength()+m_strData.size()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_BASE_VERSION, src.at(0), "sserialize::Static::UnicodeTrie::FlatTrieBase");
}

UByteArrayAdapter::OffsetType FlatTrieBase::getSizeInBytes() const {
	return 1+UByteArrayAdapter::OffsetTypeSerializedLength()+m_strData.size()+m_trie.getSizeInBytes();
}

uint32_t FlatTrieBase::find(const std::string & str, bool prefixMatch) const {
	if (size() == 0)
		return npos;

	uint32_t left = 0;
	uint32_t right = size()-1;
	uint32_t mid  = (right-left)/2 + left;

	uint16_t lLcp = calcLcp(strData(left), str);
	if (lLcp == str.size()) //first is match
		return 0;
	uint16_t rLcp = calcLcp(strData(right), str);
	uint16_t mLcp = 0;
	int8_t cmp = compare(strData(mid), str, mLcp);
	
	while(right-left > 1) {
		mid = (right-left)/2 + left;
		cmp = compare(strData(mid), str, mLcp);
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

	if (cmp != 0) { //mid does not point to the equal element, the correct one might be either in left or right
		if (lLcp == str.size()) {
			mid = left;
			mLcp = lLcp;
			cmp = compare(strData(mid), str, mLcp);
		}
		else if (rLcp == str.size()) {
			mid = right;
			mLcp = rLcp;
			cmp = compare(strData(mid), str, mLcp);
		}
	}
	
	if (mLcp == str.size()) {
		if (prefixMatch || strData(mid).size() == str.size()) {
			return mid;
		}
	}
	return npos;
}


}}}//end namespace