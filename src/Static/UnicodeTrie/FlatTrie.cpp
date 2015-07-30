#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/strings/stringfunctions.h>

namespace sserialize {
namespace Static {
namespace UnicodeTrie {
namespace detail {
namespace FlatTrie {

bool CompFunc::operator()(uint32_t a, const StaticString & b) const {
	UByteArrayAdapter strB = strHandler->strData(b);
	return a < utf8::peek_next(strB+posInStr, strB.end());
}

bool CompFunc::operator()(const StaticString & a, uint32_t b) const {
	UByteArrayAdapter strA = strHandler->strData(a);
	return utf8::peek_next(strA+posInStr, strA.end()) < b;
}


Node::Iterator::Iterator(const uint32_t parentBegin, const uint32_t parentEnd, const CompFunc & compFunc) :
m_childNodeBegin(parentBegin),
m_childNodeEnd(parentBegin),
m_childrenEnd(parentEnd),
m_compFunc(compFunc) {
	if (m_childNodeBegin != m_childrenEnd) {
		++m_childNodeEnd;
		operator++();
	}
}

Node::Iterator & Node::Iterator::operator++() {
	m_childNodeBegin = m_childNodeEnd;
	if (m_childNodeBegin != m_childrenEnd) {
		UByteArrayAdapter strData = m_compFunc.strHandler->strData(m_childNodeBegin);
		uint32_t cp = utf8::peek_next( strData+m_compFunc.posInStr, strData.end());
		sserialize::Static::UnicodeTrie::FlatTrieBase::StaticStringsIterator trieBegin(m_compFunc.strHandler->staticStringsBegin());
		m_childNodeEnd = std::upper_bound( trieBegin+m_childNodeBegin, trieBegin+m_childrenEnd, cp, m_compFunc).id();
	}
	return *this;
}

bool Node::Iterator::operator!=(const Iterator & other) {
	return m_childNodeBegin != other.m_childNodeBegin ||
			m_childNodeEnd != other.m_childNodeEnd ||
			m_childrenEnd != other.m_childrenEnd ||
			m_compFunc != other.m_compFunc;
}

bool Node::Iterator::operator==(const Iterator & other) {
	return m_childNodeBegin == other.m_childNodeBegin ||
			m_childNodeEnd == other.m_childNodeEnd ||
			m_childrenEnd == other.m_childrenEnd ||
			m_compFunc == other.m_compFunc;
}

Node Node::Iterator::operator*() const {
	return Node(m_childNodeBegin, m_childNodeEnd, m_compFunc.strHandler);
}

Node::Node(uint32_t begin, uint32_t end, const FlatTrieBase * trie) :
m_trie(trie),
m_begin(begin),
m_end(end)
{}

StaticString Node::sstr() const {
	return m_trie->sstr(id());
}

UByteArrayAdapter Node::strData() const {
	return m_trie->strData(id());
}

std::string Node::str() const {
	return m_trie->strAt(id());
}

Node::const_iterator Node::begin() const {
	return const_iterator(m_begin, m_end, CompFunc(m_trie, sstr().size()));
}

Node::const_iterator Node::cbegin() const {
	return const_iterator(m_begin, m_end, CompFunc(m_trie, sstr().size()));
}

Node::const_iterator Node::end() const {
	return const_iterator(m_end, m_end, CompFunc(m_trie, sstr().size()));
}

Node::const_iterator Node::cend() const {
	return const_iterator(m_end, m_end, CompFunc(m_trie, sstr().size()));
}


}}//end namespace detail::FlatTrie

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
			compare(strData(mid), str, mLcp); //calc mLcp
		}
		else if (rLcp == str.size()) {
			mid = right;
			mLcp = rLcp;
			compare(strData(mid), str, mLcp); //calc mLcp
		}
	}
	
	if (mLcp == str.size()) {
		if (prefixMatch || strData(mid).size() == str.size()) {
			return mid;
		}
	}
	return npos;
}

uint32_t FlatTrieBase::size() const {
	return m_trie.size();
}

FlatTrieBase::StaticStringsIterator FlatTrieBase::staticStringsBegin() const {
	return StaticStringsIterator(0, this);
}

FlatTrieBase::StaticStringsIterator FlatTrieBase::staticStringsEnd() const {
	return FlatTrieBase::StaticStringsIterator(size(), this);
}

FlatTrieBase::StaticString FlatTrieBase::sstr(uint32_t pos) const {
	return FlatTrieBase::StaticString(m_trie.at(pos, TA_STR_OFFSET), m_trie.at(pos, FlatTrieBase::TA_STR_LEN));
}

UByteArrayAdapter FlatTrieBase::strData(const FlatTrieBase::StaticString & str) const {
	return UByteArrayAdapter(m_strData, str.off(), str.size());
}

std::string FlatTrieBase::strAt(const StaticString & str) const {
	UByteArrayAdapter::MemoryView mem(strData(str).asMemView());
	return std::string(mem.begin(), mem.end());
}

UByteArrayAdapter FlatTrieBase::strData(uint32_t pos) const {
	return strData(sstr(pos));
}

std::string FlatTrieBase::strAt(uint32_t pos) const {
	return strAt(sstr(pos));
}

FlatTrieBase::Node FlatTrieBase::root() const {
	return Node(0, size(), this);
}

std::ostream& FlatTrieBase::printStats(std::ostream& out) const {
	out << "sserialize::Static::UnicodeTrie::FlatTrieBase::stats--BEGIN" << std::endl;
	out << "total data size=" << m_strData.size() + m_trie.getSizeInBytes() << std::endl;
	out << "string data size=" << m_strData.size() << std::endl;
	m_trie.printStats(out);
	out << "sserialize::Static::UnicodeTrie::FlatTrieBase::stats--END" << std::endl;
	return out;
}


}}}//end namespace