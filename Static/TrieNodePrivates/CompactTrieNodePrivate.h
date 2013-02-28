#ifndef COMPACT_STATIC_TRIE_NODE_PRIVATE_H
#define COMPACT_STATIC_TRIE_NODE_PRIVATE_H
#include <stdint.h>
#include <string>
#include <sserialize/utility/UByteArrayAdapter.h>
#include "TrieNodePrivate.h"



//TODO: Im Fall eines nicht vollständigen index sollte die Größe für den Merge angegeben werden


/* Data and Layout definitions (v3):
 * 
 * 
 * 
 * Layouts:
 *
 *
 * Node layout
 * 
 * ----------------------------------------------------------------------------------------------------------------
 * aaaaaccc|cciiiiim|epsSlLLL|LLLLLLLL|SLSLSLSL|FCPFCPFCP|CPDIFFCPDIFF|SSSSSSSSS|CCCCCCCCCC|CPCPCPCPCP|  IDXPTRS |
 * ----------------------------------------------------------------------------------------------------------------
 * 3 byte                     | 1 byte | 1 byte | vl32    |    vl32    |SL byte  |CompactArr|CompactArr|CompactArr|
 * a = bit width in the child-char array
 * c = bit width in the child ptr array
 * i = index bits per number count
 * m = node has a merge index
 * e = node has a exact index
 * p = node has a prefix index
 * s = node has a suffix index
 * S = node has a suffixPrefix index
 * l = if true, then there's another length byte following
 * L = length of the following char->pointer array
 * SL = length of the string
 * S = the node's string, not null-terminated, encoded as UTF-8, the first char is stored in the parent 
 * C = character (width is determined by a), encoded in UTF-32 (essentially it's just the unicde point)
 * FCP = displacement from the beginning of the tree to the first child node
 * CPDIFFCPDIFF = minumum diff betwwen child pointers, which is only present if childCount > 2
 * IDXPTR = Pointer to index (offset in index file
 */

namespace sserialize {
namespace Static {

class CompactStaticTrieCreationNode;

class CompactTrieNodePrivate: public TrieNodePrivate {
enum {HS_CHILD_COUNT=0, HS_CHILD_COUNT_FLAG=11, HS_INDEX_TYPES=12, HS_MERGE_INDEX=16, HS_INDEX_BITS=17, HS_CHILD_PTR_BITS=22, HS_CHILD_CHAR_BITS=27} HeaderShifts;
enum {HM_CHILD_COUNT=0x7FF, HM_CHILD_COUNT_FLAG=0x1, HM_INDEX_TYPES=0xF, HM_MERGE_INDEX=0x1, HM_INDEX_BITS=0x1F, HM_CHILD_PTR_BITS=0x1F, HM_CHILD_CHAR_BITS=0x1F} HeaderMasks;
public:
	friend class CompactStaticTrieCreationNode;
protected:
	UByteArrayAdapter m_data;

	//Node info
	uint32_t m_header;
	uint16_t m_childCount;
	uint8_t m_strLen;
	uint32_t m_childPtrBeginOffset;
	uint32_t m_childPtrDiff;
	
	//data pointers:
	uint32_t m_strStart;
	uint32_t m_childArrayStart;
	uint32_t m_childPointerArrayStart;
	uint32_t m_indexPtrStart;
	uint32_t m_myEndPtr;


private:
	uint32_t getSubTriePtrOffset() const;
	uint32_t getChildPtrBeginOffset() const;
	
	
	IndexTypes indexTypes() const;
	uint8_t childPtrBits() const;
	uint8_t indexPtrBits() const;
	
	
public:
	CompactTrieNodePrivate(const UByteArrayAdapter & nodeData);
	~CompactTrieNodePrivate() {}

	virtual uint16_t childCount() const { return m_childCount;}
	virtual uint8_t charWidth() const;
	virtual uint8_t strLen() const { return m_strLen;}
	virtual bool hasMergeIndex() const;
	virtual bool hasExactIndex() const { return indexTypes() & IT_EXACT;}
	virtual bool hasPrefixIndex() const { return indexTypes() & IT_PREFIX;}
	virtual bool hasSuffixIndex() const { return indexTypes() & IT_SUFFIX;}
	virtual bool hasSuffixPrefixIndex() const { return indexTypes() & IT_SUFFIX_PREFIX;}
	virtual bool hasAnyIndex() const { return indexTypes();}

	/** @return: Returns a pointer to the subclass, destruction will be handled by the outer non-private class **/
	virtual TrieNodePrivate* childAt(uint16_t pos) const;

	virtual uint32_t childCharAt(uint16_t pos) const;
	virtual UByteArrayAdapter strData() const;
	virtual std::string str() const;
	virtual int16_t posOfChar(uint32_t ucode) const;
	virtual void dump() const;

	virtual uint32_t getStorageSize() const;
	virtual uint32_t getHeaderStorageSize() const;
	virtual uint32_t getNodeStringStorageSize() const;
	virtual uint32_t getChildPtrStorageSize() const;
	virtual uint32_t getChildCharStorageSize() const;
	virtual uint32_t getIndexPtrStorageSize() const;

	virtual uint32_t getExactIndexPtr() const;
	virtual uint32_t getPrefixIndexPtr() const;
	virtual uint32_t getSuffixIndexPtr() const;
	virtual uint32_t getSuffixPrefixIndexPtr() const;
	virtual uint32_t getChildPtr(uint32_t pos) const;
};


class CompactStaticTrieCreationNode {
	CompactTrieNodePrivate m_node;
	UByteArrayAdapter m_data;
public:
	enum ErrorTypes { NO_ERROR=0, TOO_MANY_CHILDREN=1, NODE_STRING_TOO_LONG=2, CHILD_PTR_FAILED=4, INDEX_PTR_FAILED=8};
	CompactStaticTrieCreationNode(const UByteArrayAdapter & nodeData);
	~CompactStaticTrieCreationNode() {}
	static unsigned int createNewNode(const sserialize::Static::TrieNodeCreationInfo & nodeInfo, UByteArrayAdapter& destination);
	static unsigned int appendNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter& destination);
	static unsigned int prependNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, std::deque<uint8_t> & destination);
	static bool isError(unsigned int error);
	static std::string errorString(unsigned int error);
	static uint8_t getType();
};

}}//end namespace

#endif