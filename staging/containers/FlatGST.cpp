#include "FlatGST.h"
#include <sserialize/utility/ProgressInfo.h>
#include <sserialize/vendor/utf8/checked.h>
#include <sserialize/templated/EdgeList.h>
#include <unordered_set>
#include <algorithm>

namespace sserialize {

void FlatGST::nextSuffixString(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd) {
	if (m_suffixDelimeters.size()) {
		while (strIt != strEnd) {
			if (m_suffixDelimeters.count( utf8::next(strIt, strEnd) ) > 0)
				break;
		}
	}
	else if (strIt != strEnd) {
		utf8::next(strIt, strEnd);
	}
}

//end points to one beyond
uint32_t FlatGST::Node::findNextChild(sserialize::FlatGST::TrieStorage * trie, const sserialize::FlatGST::OrderedStringTable * strTable, uint32_t begin, uint32_t end, uint32_t strOffset, uint32_t prevChildCp) const {
	uint32_t l = begin;
	uint32_t r = end;
	uint32_t m = (r-l)/2;
	uint32_t cp = 0;
	while (begin+1 < end) {
		m = (r-l)/2;
		const Entry & ms = trie->at(m);
		//this alwasy works as children are always longer than strOffset which comes from their parents
		cp = utf8::peek_next(ms.cbegin(strTable)+strOffset, ms.cend(strTable));
		if (prevChildCp <= cp) {
			l = m;
		}
		else {
			r = m;
		}
	}
	//m should always end one before the next theoretical child
	return m+1;
}

std::map< uint32_t, FlatGST::Node > FlatGST::Node::children(FlatGST::TrieStorage * trie, const OrderedStringTable * strTable) const {
	if (m_end <= m_begin || m_end - m_begin == 1) {
		return std::map< uint32_t, FlatGST::Node >();
	};
	uint32_t offset = trie->at(m_begin).cend(strTable) - trie->at(m_begin).cbegin(strTable);
	std::map<uint32_t, Node> children;
	uint32_t childBegin = m_begin+1;
	while (childBegin < m_end) {
		uint32_t childCp = utf8::peek_next(trie->at(childBegin).cbegin(strTable)+offset, trie->at(childBegin).cend(strTable));
		uint32_t childEnd = findNextChild(trie, strTable, childBegin, m_end, offset, childCp);
		children[childCp] = Node(childBegin, childEnd);
		childBegin = childEnd;
	}
	return children;
}

FlatGST::Node::Node(uint32_t begin, uint32_t end) : 
m_begin(begin),
m_end(end)
{}

FlatGST::Node::~Node() {}

FlatGST::Node FlatGST::rootNode() {
	return Node(0, m_trie.size());
}

uint32_t FlatGST::find(const std::string & str, StringCompleter::QuerryType qt) const {
	if (m_trie.size() == 0)
		return npos;

	EntryComparator comparator(&m_strTable);
	EntryLcpCalculator lcpCalc(&m_strTable);
		
	uint32_t left = 0;
	uint32_t right = m_trie.size()-1;
	uint32_t mid  = (right-left)/2 + left;

	uint16_t lLcp = lcpCalc(m_trie.at(left), str);
	if (lLcp == str.size()) //first is match
		return 0;
	uint16_t rLcp = lcpCalc(m_trie.at(right), str);
	uint16_t mLcp = 0;
	int8_t cmp = comparator(m_trie.at(mid), str, mLcp);
	
	while(right-left > 1) {
		mid = (right-left)/2 + left;
		cmp = comparator(m_trie.at(mid), str, mLcp);
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
			cmp = comparator(m_trie.at(mid), str, mLcp);
		}
		else if (rLcp == str.size()) {
			mid = right;
			mLcp = rLcp;
			cmp = comparator(m_trie.at(mid), str, mLcp);
		}
	}
	
	if (mLcp == str.size()) {
		if (qt & sserialize::StringCompleter::QT_PREFIX || qt & sserialize::StringCompleter::QT_SUFFIX_PREFIX || m_trie.at(mid).len == str.size()) {
			return mid;
		}
	}
	return npos;
}


FlatGST::FlatGST() :
m_isSuffixTrie(false),
m_caseSensitive(false),
m_addTransDiacs(false)
{}

FlatGST::~FlatGST() {}

ItemIndex FlatGST::complete(const std::string & str, StringCompleter::QuerryType qtype) const {
	std::string qstr;
	if (m_caseSensitive) {
		qstr = str;
	}
	else {
		qstr = unicode_to_lower(str);
	}
	uint32_t p = find(qstr, qtype);
	if (p != npos) {
		const Entry & e = m_trie.at(p);
		if (qtype & sserialize::StringCompleter::QT_EXACT) {
			std::vector<uint32_t> r(e.exactMatchedItems.begin(), e.exactMatchedItems.end());
			return ItemIndex::absorb(r);
		}
		else if (qtype & sserialize::StringCompleter::QT_SUFFIX) {
			std::vector<uint32_t> r(e.suffixMatchedItems.begin(), e.suffixMatchedItems.end());
			return ItemIndex::absorb(r);
		}
		else {
			throw sserialize::UnimplementedFunctionException("sserialize::FlatGST does not support prefix/substring matching");
		}
	}
	return ItemIndex();
}

StringCompleter::SupportedQuerries FlatGST::getSupportedQuerries() const {
	uint32_t sq = sserialize::StringCompleter::SQ_EXACT;
	if (m_isSuffixTrie) {
		sq |= sserialize::StringCompleter::SQ_SUFFIX;
	}
	if (m_caseSensitive) {
		sq |= sserialize::StringCompleter::SQ_CASE_SENSITIVE;
	}
	else {
		sq |= sserialize::StringCompleter::SQ_CASE_INSENSITIVE;
	}
	return (sserialize::StringCompleter::SupportedQuerries)sq;
}

std::ostream & FlatGST::printStats(std::ostream & out) const {
	return sserialize::StringCompleterPrivate::printStats(out);
}


void FlatGST::trieFromMyStringTable() {
	EntrySmallerComparator entryComparator(&m_strTable);
	EntryEqualityComparator entryEqualityComparator(&m_strTable);
	EntryHasher entryHasher(&m_strTable);
	std::unordered_set<Entry, EntryHasher, EntryEqualityComparator> trieStrings(10, entryHasher, entryEqualityComparator);
	sserialize::ProgressInfo progressInfo;
	progressInfo.begin(m_strTable.size(), "FlatGST::trieFromMyStringTable: Creating Trie from strings");
	uint32_t count = 0;
	for(uint32_t i= 0, s = m_strTable.size(); i < s; ++i) {
		Entry entry(i, 0, m_strTable[i].size());
		if (trieStrings.count(entry) == 0) {
			trieStrings.insert(entry);
			m_trie.push_back(entry);
		}

		if (m_isSuffixTrie) {
			const std::string & str = m_strTable[i];
			std::string::const_iterator strBegin = str.begin();
			std::string::const_iterator strIt = strBegin;
			std::string::const_iterator strEnd = str.end();
			nextSuffixString(strIt, strEnd);
			while (strIt != strEnd) {
				entry.pos = (strIt - strBegin);
				entry.len = str.size() - entry.pos;
				if (trieStrings.count(entry) == 0) {
					trieStrings.insert(entry);
					m_trie.push_back(entry);
				}
				nextSuffixString(strIt, strEnd);
			}
		}
	}
	progressInfo(++count);
	
	std::sort(m_trie.begin(), m_trie.end(), entryComparator);
	//in the next phase we have to add our inner nodes that do not have a node
}

void FlatGST::serialize(UByteArrayAdapter & /*dest*/) {
	EdgeList<uint32_t> edgeList;
}


}//end namespacec