#ifndef SSERIALIZE_STATIC_GENERALIZED_TRIE_H
#define SSERIALIZE_STATIC_GENERALIZED_TRIE_H
#include <sserialize/completers/StringCompleterPrivate.h>
#include <sserialize/Static/ItemIndexStore.h>
#include "TrieNodePrivates/TrieNodePrivates.h"

#define STATIC_TRIE_HEADER_SIZE 16
#define PRIVATE_USE_CHAR_VALUE 0xF0F0

#define SSERIALIZE_STATIC_GENERALIZED_TRIE_VERSION 0

/*
 * Static Trie Layout
 * --------------------------------------------------------------------------------------
 * vvvvvvvv|TTTTTTTT|NTNTNTNT|LSLSLSLS|DDDDDDDD|SCSCSCSC|NCNCNCNC|PADDING |NODES
 * --------------------------------------------------------------------------------------
 *  1 byte | 1 byte | 1 byte |2 byte | 2 byte | 4 byte | 4 byte  | 1 byte |multiple bytes
 * 
 * v = version number as uint8
 
 * T = tree type
 * NT = NodeType
 * LS = length of the longest string as uint16
 * D = depth of the tree as uint16
 * NC = number of nodes in the tree as uint32
 * SC = number of unique strings in the tree as uint32
 * 
 * TrieNodes have 4 different indices:
 * Exact: contains Elements with the exact query string only
 * Prefix: Elements with the query string as prefix. Prefix - Exact = RemPrefix
 * Suffix: contains Elements with the query string as suffix. Suffix - Exact = RemSuffix(<= this is stored)
 * SuffixPrefix: contains Elements with the query string as sub string. SuffixPrefix - Exact - Prefix - Suffix = RemSuffixPrefix
 * 
 * Note: end nodes do not have a prefix/suffixPrefix Index
 * 
 * BUT: If Trie has a indirect Index, then prefix/suffixPrefix indices are fully stored to be used as remapping tables for exact and suffix sets respectively
 * 
 */


namespace sserialize {
namespace Static {

class GeneralizedTrie: public StringCompleterPrivate {
public:
	typedef sserialize::Static::TrieNode Node;
protected:
	UByteArrayAdapter m_tree;
	Static::ItemIndexStore m_indexStore;
	uint8_t m_type;
	uint8_t m_nodeType;
	uint16_t m_maxCompletionStrLen;
	uint8_t m_depth;
	uint16_t m_sq;
public:
	typedef enum {
		STO_NORMALIZED=1,
		STO_CASE_SENSITIVE=2,
		STO_SUBTRIES=4,
		STO_INDIRECT_INDEX=8,
		STO_SUFFIX=16
	} TrieOptions;
	typedef enum {IT_NONE=0x0, IT_PREFIX=0x1, IT_SUFFIXPREFIX=0x2} IndexMergeType;
protected:
	void insertIndexRecursive(const sserialize::Static::TrieNode& node, IndexMergeType type, DynamicBitSet & dest) const;

	void addPrefixIndexPtrsRecursive(const sserialize::Static::TrieNode& node, std::vector< uint32_t >& indexPtrs) const;
	void addSuffixPrefixIndexPtrsRecursive(const sserialize::Static::TrieNode& node, std::vector< uint32_t >& indexPtrs) const;

protected://completion functions
	ItemIndex completeCISRecursive(std::string::const_iterator strIt, const std::string::const_iterator strEnd, sserialize::StringCompleter::QuerryType qtype, const sserialize::Static::GeneralizedTrie::Node& node) const;
	ItemIndex completeCS(const std::string& str, sserialize::StringCompleter::QuerryType qtype) const;
	
public:
	GeneralizedTrie();
	/** copy the other tree,  but not the ref-count */
	GeneralizedTrie(const GeneralizedTrie & other);
	GeneralizedTrie(const UByteArrayAdapter & trieData, const Static::ItemIndexStore & indexStore);
	virtual ~GeneralizedTrie();
	Static::TrieNode getRootNode() const;
	ItemIndex getItemIndexFromNode(const sserialize::Static::TrieNode& node, sserialize::StringCompleter::QuerryType type) const;
	ItemIndex getItemIndexFromNode(const sserialize::Static::TrieNode& node, sserialize::StringCompleter::QuerryType qtype, const ItemIndex & indirectIndexParent) const;


	virtual ItemIndex complete(const std::string& str, sserialize::StringCompleter::QuerryType qtype) const;
	/** @return returns pairs of char->ItemIndex **/
	virtual std::map<uint16_t, ItemIndex> getNextCharacters(const std::string& str, sserialize::StringCompleter::QuerryType qtype, bool withIndex) const;

	virtual std::ostream& printStats(std::ostream& out) const;

	virtual std::string getName() const;
	
	virtual Static::ItemIndexStore getIndexStore() const;
	virtual ItemIndex indexFromId(uint32_t ptr) const;
	virtual sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;


public:
	static bool addHeader(uint8_t trieType, uint8_t nodeType, uint16_t longestString, uint8_t depth, std::deque<uint8_t> & destination);
	static uint8_t getType();

};

}}//end namespace

#endif