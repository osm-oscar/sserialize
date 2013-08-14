#include <sserialize/Static/GeneralizedTrie.h>
#include <iostream>
#include <map>
#include <algorithm>
#include <string>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/stringfunctions.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/Static/StringCompleter.h>
#include <sserialize/containers/DynamicBitSet.h>
#include "triestats.h"

namespace sserialize {
namespace Static {

GeneralizedTrie::GeneralizedTrie() :
StringCompleterPrivate()
{}

GeneralizedTrie::GeneralizedTrie(const Static::GeneralizedTrie& other) :
StringCompleterPrivate(),
m_tree(other.m_tree),
m_indexStore(other.m_indexStore),
m_type(other.m_type),
m_nodeType(other.m_nodeType),
m_maxCompletionStrLen(other.m_maxCompletionStrLen),
m_depth(other.m_depth),
m_sq(other.m_sq)
{

}


GeneralizedTrie::GeneralizedTrie(const UByteArrayAdapter& trieData, const Static::ItemIndexStore& indexStore) :
StringCompleterPrivate(),
m_indexStore(indexStore)
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_GENERALIZED_TRIE_VERSION, trieData.at(0), "Static::GeneralizedTrie");
	m_type = trieData.at(1);
	m_nodeType = trieData.at(2); 
	m_maxCompletionStrLen = trieData.getUint16(3);
	m_depth = trieData.at(5);

	m_tree = trieData+STATIC_TRIE_HEADER_SIZE;

	m_sq = (sserialize::StringCompleter::SQ_EP | sserialize::StringCompleter::SQ_CASE_INSENSITIVE);
	if (m_type & STO_CASE_SENSITIVE)
		m_sq |= sserialize::StringCompleter::SQ_CASE_SENSITIVE;

	if (m_type & STO_SUFFIX) {
		m_sq |= (sserialize::StringCompleter::SQ_SSP);
	}
}

GeneralizedTrie::~GeneralizedTrie() {}

Static::TrieNode GeneralizedTrie::getRootNode() const {
	switch (m_nodeType) {
		case (Static::TrieNode::T_SIMPLE):
			return TrieNode(new SimpleTrieNodePrivate(m_tree));
		case (Static::TrieNode::T_COMPACT):
			return TrieNode(new CompactTrieNodePrivate(m_tree));
		case (Static::TrieNode::T_LARGE_COMPACT):
			return TrieNode(new LargeCompactTrieNodePrivate(m_tree));
		default:
			return TrieNode(new EmptyTrieNodePrivate());
	}
}

void GeneralizedTrie::insertIndexRecursive(const sserialize::Static::TrieNode & node, sserialize::Static::GeneralizedTrie::IndexMergeType type, sserialize::DynamicBitSet & dest) const {
	IndexMergeType nextType = IT_NONE;
	if (node.hasMergeIndex()) {
		if (type & IT_PREFIX) {
			if (node.hasExactIndex())
				indexFromId(node.getExactIndexPtr()).putInto(dest);

			if (node.hasPrefixIndex()) {
				indexFromId(node.getPrefixIndexPtr()).putInto(dest);
			}
			else {
				nextType = (IndexMergeType) (nextType | IT_PREFIX);
			}
		}
		if (type & IT_SUFFIXPREFIX) {
			if (node.hasExactIndex())
				indexFromId(node.getExactIndexPtr()).putInto(dest);
			if (node.hasSuffixIndex())
				indexFromId(node.getSuffixIndexPtr()).putInto(dest);

			if (node.hasPrefixIndex()) {
				indexFromId(node.getPrefixIndexPtr()).putInto(dest);
			}

			if (node.hasSuffixPrefixIndex()) {
				indexFromId(node.getSuffixPrefixIndexPtr()).putInto(dest);
			}
			else {
				nextType = (IndexMergeType) (nextType | IT_SUFFIXPREFIX);
			}
		}
	}
	else {
		if (type & IT_PREFIX) {
			if (node.hasPrefixIndex()) {
				indexFromId(node.getPrefixIndexPtr()).putInto(dest);
			}
			else {
				if (node.hasExactIndex())
					indexFromId(node.getExactIndexPtr()).putInto(dest);
				nextType = (IndexMergeType) (nextType | IT_PREFIX);
			}
		}
		if (type & IT_SUFFIXPREFIX) {
			if (node.hasSuffixPrefixIndex()) {
				indexFromId(node.getSuffixPrefixIndexPtr()).putInto(dest);
			}
			else {
				if (node.hasExactIndex())
					indexFromId(node.getExactIndexPtr()).putInto(dest);
				if (node.hasSuffixIndex())
					indexFromId(node.getSuffixIndexPtr()).putInto(dest);
				nextType = (IndexMergeType) (nextType | IT_SUFFIXPREFIX);
			}
		}
	}
	if (nextType  != IT_NONE) {
		uint16_t childCount = node.childCount();
		for(uint16_t i = 0; i < childCount; ++i)
			insertIndexRecursive(node.childAt(i), nextType, dest);
	}
}


void GeneralizedTrie::addPrefixIndexPtrsRecursive(const sserialize::Static::TrieNode& node, std::vector<uint32_t>& indexPtrs) const {
	if (node.hasPrefixIndex()) {
		indexPtrs.push_back(node.getPrefixIndexPtr());
	}
	else {
		if (node.hasExactIndex())
			indexPtrs.push_back(node.getExactIndexPtr());
		for(size_t i = 0; i < node.childCount(); i++) {
			addPrefixIndexPtrsRecursive(node.childAt(i), indexPtrs);
		}
	}
}

void GeneralizedTrie::addSuffixPrefixIndexPtrsRecursive(const Static::TrieNode & node, std::vector< uint32_t >& indexPtrs) const {
	if (node.hasSuffixPrefixIndex()) {
		indexPtrs.push_back(node.getSuffixPrefixIndexPtr());
	}
	else {
		if (node.hasMergeIndex() && node.hasExactIndex())
			indexPtrs.push_back(node.getExactIndexPtr());
		if (node.hasSuffixIndex())
			indexPtrs.push_back(node.getSuffixIndexPtr());

		for(size_t i = 0; i < node.childCount(); i++) {
			addSuffixPrefixIndexPtrsRecursive(node.childAt(i), indexPtrs);
		}
	}
}

ItemIndex GeneralizedTrie::getItemIndexFromNode(const sserialize::Static::TrieNode& node, sserialize::StringCompleter::QuerryType type) const {
	if (type & sserialize::StringCompleter::QT_SUFFIX_PREFIX) {
		if (node.hasSuffixPrefixIndex()) {
			if (node.hasMergeIndex()) {
				ItemIndex tmp;
				if (node.hasExactIndex())
					tmp = tmp + indexFromId( node.getExactIndexPtr() );
				if (node.hasPrefixIndex())
					tmp = tmp + indexFromId( node.getPrefixIndexPtr() );
				if (node.hasSuffixIndex())
					tmp = tmp + indexFromId( node.getSuffixIndexPtr() );
				if (node.hasSuffixPrefixIndex())
					tmp = tmp + indexFromId( node.getSuffixPrefixIndexPtr() );
				return tmp;
			}
			else {
				return indexFromId( node.getSuffixPrefixIndexPtr() );
			}
		}
		else {
			if (node.childCount()) {
				DynamicBitSet bitSet(UByteArrayAdapter::createCache(0, false));
				insertIndexRecursive(node, IT_SUFFIXPREFIX, bitSet);
				return bitSet.toIndex(m_indexStore.indexType());
			}
			else {
				if (node.hasMergeIndex() && node.hasExactIndex())
					return indexFromId(node.getSuffixIndexPtr()) + indexFromId( node.getExactIndexPtr() );
				else
					return indexFromId(node.getSuffixIndexPtr());
			}
		}
	}
	else if (type & sserialize::StringCompleter::QT_SUFFIX) {
		if (node.hasSuffixIndex()) {
			if (node.hasMergeIndex() && node.hasExactIndex()) {
				return indexFromId(node.getExactIndexPtr()) + indexFromId(node.getSuffixIndexPtr());
			}
			else {
				return indexFromId(node.getSuffixIndexPtr());
			}
		}
	}
	else if (type & sserialize::StringCompleter::QT_PREFIX) {
		if (node.hasPrefixIndex()) {
			if (node.hasMergeIndex() && node.hasExactIndex())
				return indexFromId(node.getPrefixIndexPtr()) + indexFromId(node.getExactIndexPtr());
			else
				return indexFromId(node.getPrefixIndexPtr());
		}
		else {
			if (node.childCount()) {
				DynamicBitSet bitSet(UByteArrayAdapter::createCache(0, false));
				insertIndexRecursive(node, IT_PREFIX, bitSet);
				return bitSet.toIndex(m_indexStore.indexType());
			}
			else if (node.hasExactIndex())
				return indexFromId(node.getExactIndexPtr());
		}
	}
	else if (type & sserialize::StringCompleter::QT_EXACT) {
		if (node.hasExactIndex()) {
			return indexFromId(node.getExactIndexPtr());
		}
	}
	return ItemIndex();
}

ItemIndex GeneralizedTrie::getItemIndexFromNode(const sserialize::Static::TrieNode& node, sserialize::StringCompleter::QuerryType qtype, const ItemIndex& indirectIndexParent) const {
	if (qtype & sserialize::StringCompleter::QT_SUFFIX_PREFIX) {
		if (node.hasSuffixPrefixIndex()) {
			return m_indexStore.at(node.getSuffixPrefixIndexPtr(), indirectIndexParent);
		}
		else {
			std::vector<uint32_t> childIndexPtrs;
			addSuffixPrefixIndexPtrsRecursive(node, childIndexPtrs);
			std::vector<ItemIndex> idxSet;
			idxSet.reserve(childIndexPtrs.size());
			for(size_t i = 0; i < childIndexPtrs.size(); i++) {
				idxSet.push_back(m_indexStore.at(childIndexPtrs[i], indirectIndexParent));
			}
			return ItemIndex::unite(idxSet);
		}
	}
	else if (qtype & sserialize::StringCompleter::QT_SUFFIX) {
		if (node.hasSuffixIndex()) {
			return m_indexStore.at(node.getSuffixIndexPtr(), indirectIndexParent);
		}
	}
	else if (qtype & sserialize::StringCompleter::QT_PREFIX) {
		if (node.hasPrefixIndex()) {
			return m_indexStore.at(node.getPrefixIndexPtr(), indirectIndexParent);
		}
		else {
			std::vector<uint32_t> childIndexPtrs;
			addPrefixIndexPtrsRecursive(node, childIndexPtrs);
			std::vector<ItemIndex> idxSet;
			idxSet.reserve(childIndexPtrs.size());
			for(size_t i = 0; i < childIndexPtrs.size(); i++) {
				idxSet.push_back(m_indexStore.at(childIndexPtrs[i], indirectIndexParent));
			}
			return ItemIndex::unite(idxSet);
		}
	}
	else if (qtype & sserialize::StringCompleter::QT_EXACT) {
		if (node.hasExactIndex()) {
			return m_indexStore.at(node.getExactIndexPtr(), indirectIndexParent);
		}
	}
	return ItemIndex();
}

//if indexPtrs is present, then also set the index ptrs
std::map< uint16_t, ItemIndex > GeneralizedTrie::getNextCharacters(const std::string& str, sserialize::StringCompleter::QuerryType qtype, bool withIndex) const {

	Static::TrieNode node = getRootNode();

	std::string curStr;
	if ( ! (m_type & STO_CASE_SENSITIVE) || qtype & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
		curStr = unicode_to_lower(str);
	}
	else {
		curStr = str;
	}
	std::string::iterator strIt = curStr.begin();

	std::string nodeStr = node.str();
	std::string::iterator nStrIt = nodeStr.begin();
	
	//Find the end-node
	while(strIt != curStr.end()) {
		while(strIt != curStr.end() && nStrIt != nodeStr.end()) {
			if (*strIt == *nStrIt) {
				strIt++;
				nStrIt++;
			}
			else { //no common prefix
				return std::map<uint16_t, ItemIndex>();
			}
		}

		if (nStrIt == nodeStr.end() && strIt != curStr.end()) { //node->c is real prefix, strIt points to new element
			uint32_t key = utf8::peek_next(strIt, curStr.end());
			int16_t pos = node.posOfChar(key);
			if (pos >= 0) {
				utf8::next(strIt, curStr.end());
				node = node.childAt(pos);
				nodeStr = node.str();
				nStrIt = nodeStr.begin();
			}
			else {
				return std::map<uint16_t, ItemIndex>();
			}
		}
	}

	//we have reached our end-node, add the character hints for this string
	std::map<uint16_t, ItemIndex> charHints;
	if (nStrIt != nodeStr.end()) {
			charHints[utf8::peek_next<std::string::const_iterator>(nStrIt, nodeStr.end())] = (withIndex ? getItemIndexFromNode(node, qtype): ItemIndex());
	}
	else {
		for(uint32_t i = 0, s = node.childCount(); i < s; i++) {
			charHints[node.childCharAt(i)] = (withIndex ? getItemIndexFromNode(node.childAt(i), qtype): ItemIndex());
		}
	}
	return charHints;
}

ItemIndex GeneralizedTrie::complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {

	if (qtype & sserialize::StringCompleter::QT_CASE_INSENSITIVE && m_type & STO_CASE_SENSITIVE) {
		return completeCISRecursive(str.begin(), str.end(), qtype, getRootNode());
	}
	else {
		if (qtype & sserialize::StringCompleter::QT_CASE_INSENSITIVE || ! (m_type & STO_CASE_SENSITIVE) ) {
			return completeCS(unicode_to_lower(str), qtype);
		}
		else {
			return completeCS(str, qtype);
		}
	}
}


/** @param str: string to complete. */
ItemIndex
GeneralizedTrie::
completeCISRecursive(std::string::const_iterator strIt, const std::string::const_iterator strEnd, sserialize::StringCompleter::QuerryType qtype, const sserialize::Static::GeneralizedTrie::Node& node) const {
	std::string nodeStr = node.str();
	std::string::const_iterator nStrIt = nodeStr.begin();
	std::string::const_iterator nStrEnd = nodeStr.end();
	
	//Find the end-node

	uint32_t strItUCode;
	uint32_t nodeStrItUCode;
	while(strIt != strEnd && nStrIt != nStrEnd) {
		strItUCode = utf8::peek_next(strIt, strEnd);
		nodeStrItUCode = utf8::peek_next(nStrIt, nStrEnd);
		if (unicode32_to_lower(strItUCode) == unicode32_to_lower(nodeStrItUCode)) { //str already is lowercase!
			utf8::next(strIt, strEnd);
			utf8::next(nStrIt, nStrEnd);
		}
		else { //no common prefix
			return ItemIndex();
		}
	}

	if (nStrIt == nStrEnd && strIt != strEnd) { //node->c is real prefix, strIt points to new element
		uint32_t lowerCaseKey = unicode32_to_lower(utf8::next(strIt, strEnd)); //watchout: this already moves to the next ptr. no need to do it later
		uint32_t upperCaseKey = unicode32_to_upper(lowerCaseKey);
		int32_t lowerCasePos = node.posOfChar(lowerCaseKey);
		int32_t upperCasePos = (lowerCaseKey != upperCaseKey ? node.posOfChar(upperCaseKey) : -1);
		
		if (upperCasePos >= 0 && lowerCasePos >= 0) {
			return completeCISRecursive(strIt, strEnd, qtype, node.childAt(lowerCasePos)) + completeCISRecursive(strIt, strEnd, qtype, node.childAt(upperCasePos));
		}
		else if (lowerCasePos >= 0) {
			return completeCISRecursive(strIt, strEnd, qtype, node.childAt(lowerCasePos));
		}
		else if (upperCasePos >= 0) {
			return completeCISRecursive(strIt, strEnd, qtype, node.childAt(upperCasePos));
		}
		else {
			return ItemIndex();
		}
	}
	//Check if we're at the end of the string
	
	if (qtype & sserialize::StringCompleter::QT_SUFFIX_PREFIX)
		return getItemIndexFromNode(node, qtype);
	
	if ((qtype & sserialize::StringCompleter::QT_EXACT || qtype & sserialize::StringCompleter::QT_SUFFIX) && (strIt != strEnd || nStrIt != nStrEnd))
		return ItemIndex();

	return getItemIndexFromNode(node, qtype);
}


ItemIndex GeneralizedTrie::completeCS(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {
	Static::TrieNode node = getRootNode();

	std::string nodeStr = node.str();
	std::string::const_iterator strIt = str.begin();
	std::string::const_iterator nStrIt = nodeStr.begin();
	std::string::const_iterator strEnd = str.end();
	std::string::const_iterator nStrEnd = nodeStr.end();
	
	//Find the end-node
	while(strIt != strEnd) {
// 			node.dump();
		while(strIt != strEnd && nStrIt != nStrEnd) {
			if (*strIt == *nStrIt) {
				++strIt;
				++nStrIt;
			}
			else { //no common prefix
// 				std::cout << "getCompletionSetCaseSensitive: no match: " << nodeStr << " with " << curStr << std::endl;
				return ItemIndex();
			}
		}

		if (nStrIt == nStrEnd && strIt != strEnd) { //node->c is real prefix, strIt points to new element
			uint32_t key = utf8::peek_next(strIt, strEnd);
			int32_t pos = node.posOfChar(key);
			if (pos >= 0) {
				utf8::next(strIt, strEnd);
				node = node.childAt(pos);
				nodeStr = node.str();
				nStrIt = nodeStr.begin();
				nStrEnd = nodeStr.end();
			}
			else {
				return ItemIndex();
			}
		}
	}
	
	if (qtype & sserialize::StringCompleter::QT_SUFFIX_PREFIX)
		return getItemIndexFromNode(node, qtype);
	
	//Check if we're at the end of the string
	if ((qtype & sserialize::StringCompleter::QT_EXACT || qtype & sserialize::StringCompleter::QT_SUFFIX) && (strIt != strEnd || nStrIt != nStrEnd))
		return ItemIndex();

	return getItemIndexFromNode(node, qtype);
}

std::ostream& GeneralizedTrie::printStats(std::ostream& out) const {
	out << "GeneralizedTrie::printStats--BEGIN" << std::endl;
	printTrieStats(*this, out);
	out << "GeneralizedTrie::printStats--END" << std::endl;
	return out;
}

std::string GeneralizedTrie::getName() const {
    return std::string("Static::GeneralizedTrie");
}


Static::ItemIndexStore GeneralizedTrie::getIndexStore() const {
	return m_indexStore;
}

ItemIndex GeneralizedTrie::indexFromId(uint32_t ptr) const {
	return m_indexStore.at(ptr);
}

sserialize::StringCompleter::SupportedQuerries GeneralizedTrie::getSupportedQuerries() const {
    return (sserialize::StringCompleter::SupportedQuerries) m_sq;
}


bool GeneralizedTrie::addHeader(uint8_t trieType, uint8_t nodeType, uint16_t longestString, uint8_t depth, std::deque<uint8_t> & destination) {
	destination.push_back(SSERIALIZE_STATIC_GENERALIZED_TRIE_VERSION); //Version
	destination.push_back(trieType);
	destination.push_back(nodeType);
	
	uint8_t buf[4];
	pack_uint16_t(longestString, buf);
	for(size_t i = 0; i < 2; i++)
		destination.push_back(buf[i]);

	destination.push_back(depth);

	for(size_t i = 0; i < STATIC_TRIE_HEADER_SIZE-6; i++)
		destination.push_back(0);
	return true;
}

uint8_t GeneralizedTrie::getType() {
	return Static::StringCompleter::T_TRIE;
}

}}//end namespace