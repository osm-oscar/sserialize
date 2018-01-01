#include <sserialize/Static/FlatGeneralizedTrie.h>
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <unordered_set>
#include <iostream>



namespace sserialize {
namespace Static {

FlatGST::StringEntry::StringEntry() {}
FlatGST::StringEntry::StringEntry(const sserialize::MultiVarBitArray& data, uint32_t pos) :
m_strId((uint32_t) data.at(pos, 0) ),
m_strBegin((uint16_t) data.at(pos, 1) ),
m_strLen((uint16_t) data.at(pos, 2) )
{}

FlatGST::StringEntry::~StringEntry() {}

uint32_t FlatGST::StringEntry::strId() const {
	return m_strId;
}

uint16_t FlatGST::StringEntry::strBegin() const {
	return m_strBegin;
}

uint16_t FlatGST::StringEntry::strLen() const {
	return m_strLen;
}

FlatGST::IndexEntry::IndexEntry() : m_header(IT_NONE) {}
FlatGST::IndexEntry::IndexEntry(const UByteArrayAdapter& data) :
m_header(data.at(0)),
m_data(data+1, entrySize()*8)
{}
FlatGST::IndexEntry::~IndexEntry() {}

bool FlatGST::IndexEntry::mergeIndex() const {
	return indexType() & IT_MERGE_INDEX;
}

bool FlatGST::IndexEntry::itemIdIndex() const {
	return indexType() & IT_ITEM_ID_INDEX;
}


uint8_t FlatGST::IndexEntry::indexType() const {
	return m_header;
}

uint8_t FlatGST::IndexEntry::entrySize() const {
	return ((m_header >> 5) & 0x3) + 1;
}

uint32_t FlatGST::IndexEntry::exactPtr() const {
	if (indexType() & IT_EXACT)
		return m_data.at(0);
	return 0;
}

uint32_t FlatGST::IndexEntry::prefixPtr() const {
	if (indexType() & IT_PREFIX) {
		uint32_t offset = popCount(indexType() & 0x1);
		return m_data.at(offset);
	}
	return 0;
}

uint32_t FlatGST::IndexEntry::suffixPtr() const {
	if (indexType() & IT_SUFFIX) {
		uint32_t offset = popCount(indexType() & 0x3);
		return m_data.at(offset);
	}
	return 0;
}

uint32_t FlatGST::IndexEntry::suffixPrefixPtr() const {
	if (indexType() & IT_SUFFIX_PREFIX) {
		uint32_t offset = popCount(indexType() & 0x7);
		return m_data.at(offset);
	}
	return 0;
}

uint32_t FlatGST::IndexEntry::minId() const {
	if ( !(indexType() & IT_ITEM_ID_INDEX)) {
		uint32_t offset = popCount(indexType() & 0xF);
		return m_data.at(offset);
	}
	return 0;
}

uint32_t FlatGST::IndexEntry::maxId() const {
	if ( !(indexType() & IT_ITEM_ID_INDEX)) {
		uint32_t offset = popCount(indexType() & 0xF)+1;
		return m_data.at(offset);
	}
	return 0;
}

FlatGST::ForwardIterator::ForwardIterator(const sserialize::Static::FlatGST * cmp) : m_cmp(cmp) {}

FlatGST::ForwardIterator::ForwardIterator(const FlatGST::ForwardIterator & other) :
m_string(other.m_string),
m_cmp(other.m_cmp)
{}


FlatGST::ForwardIterator::~ForwardIterator() {}

std::set<uint32_t> FlatGST::ForwardIterator::getNext() const {
	return std::set<uint32_t>();
}

bool FlatGST::ForwardIterator::hasNext(uint32_t codepoint) const {
	std::string testStr = m_string;
	utf8::append(codepoint, std::back_insert_iterator<std::string>(testStr));
	return m_cmp->lowerBound(testStr,  static_cast<sserialize::StringCompleter::QuerryType>(sserialize::StringCompleter::QT_SUBSTRING | sserialize::StringCompleter::QT_CASE_INSENSITIVE)) >= 0;
}

bool FlatGST::ForwardIterator::next(uint32_t codepoint) {
	utf8::append(codepoint, std::back_insert_iterator<std::string>(m_string));
	return m_cmp->lowerBound(m_string, static_cast<sserialize::StringCompleter::QuerryType>(sserialize::StringCompleter::QT_SUBSTRING | sserialize::StringCompleter::QT_CASE_INSENSITIVE)) >= 0;
}

FlatGST::MyBaseClass::ForwardIterator * FlatGST::ForwardIterator::copy() const {
	return new FlatGST::ForwardIterator(*this);
}

FlatGST::FlatGST() :
StringCompleterPrivate(),
m_sq(sserialize::StringCompleter::SQ_NONE)
{}


FlatGST::FlatGST(const UByteArrayAdapter& data, const ItemIndexStore& idxStore) :
StringCompleterPrivate(),
m_sq( (sserialize::StringCompleter::SupportedQuerries) data.at(1) ),
m_idxStore(idxStore),
m_stable(data+2),
m_strEntries(data + (2 + m_stable.getSizeInBytes()) ),
m_indexEntries(data + (2 + m_stable.getSizeInBytes() + m_strEntries.getSizeInBytes()) )
{}

FlatGST::~FlatGST() {}

UByteArrayAdapter FlatGST::fgstStringAt(uint32_t pos) const {
	StringEntry e( m_strEntries, pos );
	UByteArrayAdapter r( m_stable.strDataAt( e.strId() ), e.strBegin(), e.strLen() );
	return r;
}

int32_t
FlatGST::lowerBound(const std::string& str, sserialize::StringCompleter::QuerryType qt) const {
	if (m_strEntries.size() == 0)
		return -1;

	uint32_t left = 0;
	uint32_t right = m_strEntries.size()-1;
	uint32_t mid  = (right-left)/2 + left;

	uint16_t lLcp = (uint16_t)calcLcp(fgstStringAt(left), str);
	if (lLcp == str.size()) //first is match
		return 0;
	uint16_t rLcp = (uint16_t)calcLcp(fgstStringAt(right), str);
	uint16_t mLcp = 0;
	int8_t cmp = compare(fgstStringAt(mid), str, mLcp);
	
	while(right-left > 1) {
		mid = (right-left)/2 + left;
		cmp = compare(fgstStringAt(mid), str, mLcp);
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
			compare(fgstStringAt(mid), str, mLcp);//calc mLcp
		}
		else if (rLcp == str.size()) {
			mid = right;
			mLcp = rLcp;
			compare(fgstStringAt(mid), str, mLcp);//calc mLcp
		}
	}
	
	if (mLcp == str.size()) {
		if (qt & sserialize::StringCompleter::QT_PREFIX || qt & sserialize::StringCompleter::QT_SUBSTRING || fgstStringAt(mid).size() == str.size()) {
			return narrow_check<int32_t>( mid );
		}
	}
	return -1;
}

int32_t FlatGST::getStringEntryPos(const std::string& str, sserialize::StringCompleter::QuerryType qtype) const {
	int32_t pos = -1;
	if (qtype & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
		std::string lstr = unicode_to_lower(str);
		pos = lowerBound(lstr, qtype);
	}
	else
		pos = lowerBound(str, qtype);
	return pos;
}


ItemIndex FlatGST::complete(const std::string& str, sserialize::StringCompleter::QuerryType qtype) const {
	int32_t pos = getStringEntryPos(str, qtype);
	if (pos > 0) {
		return indexFromPosition((uint32_t)pos, qtype);
	}
	return ItemIndex();
}

ItemIndexIterator FlatGST::partialComplete(const std::string& str, sserialize::StringCompleter::QuerryType qtype) const {
	int32_t pos = getStringEntryPos(str, qtype);
	if (pos > 0) {
		return indexIteratorFromPosition((uint32_t)pos, qtype);
	}
	return ItemIndexIterator();
}


ItemIndex FlatGST::indexFromId(uint32_t idxId) const {
    return m_idxStore.at(idxId);
}

ItemIndexIterator FlatGST::indexIteratorFromId(uint32_t idxId) const {
	return ItemIndexIterator( indexFromId(idxId) );
}

sserialize::StringCompleter::SupportedQuerries FlatGST::getSupportedQuerries() const {
	return m_sq;
}

std::ostream& FlatGST::printStats(std::ostream& out) const {
	out << "FlatGST::Stats::Begin" << std::endl;
	m_stable.printStats(out);
	out << "Entries:" << m_strEntries.size() << std::endl;
	out << "Size in Bytes:" << getSizeInBytes() << std::endl;
	out << "Size-Distribution:";
	out << "StringTable:" << (double) m_stable.getSizeInBytes()/getSizeInBytes()*100;
	out << ", StringEntries: " << (double) m_strEntries.getSizeInBytes()/getSizeInBytes()*100;
	out << ", IndexEntries: " << (double) m_indexEntries.getSizeInBytes()/getSizeInBytes()*100 << std::endl;
	out << "StringEntries:" << std::endl;
	out << "StringId-Bits: " << (uint32_t) m_strEntries.bitCount(0) << std::endl;
	out << "StringBegin-Bits: " << (uint32_t) m_strEntries.bitCount(1) << std::endl;
	out << "StringLen-Bits: " << (uint32_t) m_strEntries.bitCount(2) << std::endl;
	out << std::endl;
	std::unordered_set<uint32_t> indexPtrs;
	for(uint32_t i = 0; i < size(); i++) {
		FlatGST::IndexEntry e = indexEntryAt(i);
		indexPtrs.insert(e.exactPtr());
		indexPtrs.insert(e.prefixPtr());
		indexPtrs.insert(e.suffixPtr());
		indexPtrs.insert(e.suffixPrefixPtr());
	}
	m_idxStore.printStats(out, [&indexPtrs](uint32_t id) -> bool { return indexPtrs.count(id); });
	
	out << "FlatGST::Stats::End" << std::endl;
	return out;
}

std::string FlatGST::getName() const {
    return std::string("Static::FlatGeneralizedTrie");
}


FlatGST::MyBaseClass::ForwardIterator * FlatGST::forwardIterator() const {
	return new FlatGST::ForwardIterator(this);
}

OffsetType FlatGST::getSizeInBytes() const {
	return 2 + m_stable.getSizeInBytes() + m_strEntries.getSizeInBytes() + m_indexEntries.getSizeInBytes();
}

uint32_t FlatGST::size() const {
	return m_strEntries.size();
}

FlatGST::StringEntry FlatGST::stringEntryAt(uint32_t pos) const {
	return StringEntry(m_strEntries, pos);
}

FlatGST::IndexEntry FlatGST::indexEntryAt(uint32_t pos) const {
	return m_indexEntries.at(pos);
}

ItemIndex FlatGST::indexFromPosition(uint32_t pos, sserialize::StringCompleter::QuerryType qtype) const {
	if (pos < size()) {
		return indexFromEntry(m_indexEntries.at(pos), qtype);
	}
	return ItemIndex();
}

ItemIndexIterator FlatGST::indexIteratorFromPosition(uint32_t pos, sserialize::StringCompleter::QuerryType qtype) const {
	if (pos < size()) {
		return indexIteratorFromEntry(m_indexEntries.at(pos), qtype);
	}
	return ItemIndexIterator();
}

ItemIndex FlatGST::indexFromEntry(const FlatGST::IndexEntry& e, sserialize::StringCompleter::QuerryType qtype) const {
	if (qtype & sserialize::StringCompleter::QT_EXACT) {
		return indexFromId( e.exactPtr() );
	}
	if (qtype & sserialize::StringCompleter::QT_SUFFIX) {
		if (e.mergeIndex())
			return indexFromId( e.exactPtr()) + indexFromId( e.suffixPtr() );
		else
			return indexFromId( e.suffixPtr() );
	}
	if (qtype & sserialize::StringCompleter::QT_PREFIX) {
		if (e.mergeIndex())
			return indexFromId( e.exactPtr() ) + indexFromId( e.prefixPtr() );
		else
			return indexFromId( e.prefixPtr() );
	}
	if (qtype & sserialize::StringCompleter::QT_SUBSTRING) {
		if (e.mergeIndex()) {
			return (indexFromId( e.exactPtr() ) + indexFromId( e.suffixPtr() ) ) +
					(indexFromId( e.prefixPtr() ) + indexFromId( e.suffixPrefixPtr() ) );
		}
		else
			return indexFromId( e.suffixPrefixPtr() );
	}
	return ItemIndex();
}

ItemIndexIterator FlatGST::indexIteratorFromEntry(const FlatGST::IndexEntry& e, sserialize::StringCompleter::QuerryType qtype) const {
	if (qtype & sserialize::StringCompleter::QT_EXACT) {
		return  indexIteratorFromId( e.exactPtr() );
	}
	if (qtype & sserialize::StringCompleter::QT_SUFFIX) {
		if (e.mergeIndex())
			return indexIteratorFromId( e.exactPtr()) + indexIteratorFromId( e.suffixPtr() );
		else
			return indexIteratorFromId( e.suffixPtr() );
	}
	if (qtype & sserialize::StringCompleter::QT_PREFIX) {
		if (e.mergeIndex())
			return indexIteratorFromId( e.exactPtr() ) + indexIteratorFromId( e.prefixPtr() );
		else
			return indexIteratorFromId( e.prefixPtr() );
	}
	if (qtype & sserialize::StringCompleter::QT_SUBSTRING) {
		if (e.mergeIndex()) {
			return (indexIteratorFromId( e.exactPtr() ) + indexIteratorFromId( e.suffixPtr() ) ) +
					(indexIteratorFromId( e.prefixPtr() ) + indexIteratorFromId( e.suffixPrefixPtr() ) );
		}
		else
			return indexIteratorFromId( e.suffixPrefixPtr() );
	}
	return ItemIndexIterator();
}


void FlatGST::dump() {
	dump(std::cout);
}

std::ostream& FlatGST::dump(std::ostream& out) const {
	out << "FlatGST::dump::begin" << std::endl;
	out << "FlatGST::Entries::begin" << std::endl;
	for(uint32_t i = 0; i < size(); i++) {
		out << "[" << i << "]: " << fgstStringAt(i).toString() << std::endl;
	}
	out << "FlatGST::Entries::end" << std::endl;
	out << "FlatGST::dump::End" << std::endl;
	return out;
}


}}