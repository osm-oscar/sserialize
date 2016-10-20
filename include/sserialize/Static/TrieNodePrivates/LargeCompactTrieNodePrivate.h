#ifndef LARGE_COMPACT_STATIC_TRIE_NODE_PRIVATE_H
#define LARGE_COMPACT_STATIC_TRIE_NODE_PRIVATE_H
#include <stdint.h>
#include <string>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/Static/TrieNodePrivate.h>



//TODO: Im Fall eines nicht vollständigen index sollte die Größe für den Merge angegeben werden


/* Data and Layout definitions (v0):
 * 
 * 
 * 
 * Layouts:
 *
 *
 * Node layout
 * 
 * -----------------------------------------------------------------------------------------------------------
 * aamiiiii4idtlLLL|LLLLLLLLL|SLSLSLSL|SSSSSSSSS|CCCCCCCCCCCC|FCPFCPFCPFCP|CPCPCPCPCPCPCPCPCPCPCP| IndexPtr
 * -----------------------------------------------------------------------------------------------------------
 * 2 Bytes         |1-5 v32  | 1 Byte |SL byte |(1-4 byte)*L|  1-5 byte   |CompactUintArray(L-1) |IndexEntry
 * a = width of the characters (0 => 1 byte, 1 => 2 bytes, 2 => 3 bytes, 3 => 4 bytes)
 * m = node has a merge index
 * 4idt = index types (4 bits)
 * l = length bytes following
 * L = length of the following char->pointer array 
 * SL = length of the string
 * S = the node's string, not null-terminated, encoded as UTF-8, the first char is stored in the parent 
 * C = character (width is determined by a), encoded in UTF-32 (essentially it's just the unicde point)
 * FCP = displacement from the end of the node to the first child node, only present if L > 0
 * CP = displacement from the beginning of the first child, every childnode-stripe adds a new offset
 *
 * IndexEntry
 * -----------------------------------
 * BASEOFFSET|Offsets
 * -----------------------------------
 *   v32
 *
 *
 */ 

namespace sserialize {
namespace Static {


class LargeCompactTrieNodePrivate: public TrieNodePrivate {
public:
	friend class CompactStaticTrieCreationNode;
protected:
	UByteArrayAdapter m_data;

	//Node info
	uint16_t m_header; //aami iiii 4idt
	uint32_t m_childCount;
	uint8_t m_strLen;
	uint32_t m_childPtrBaseOffset;
	
	//data pointers:
	uint32_t m_strBegin;
	uint32_t m_childArrayStart;
	uint32_t m_childPointerArrayStart;
	uint32_t m_myEndPtr;
	
	//index ptr
	uint32_t m_exactIndexPtr;
	uint32_t m_prefixIndexPtr;
	uint32_t m_suffixIndexPtr;
	uint32_t m_substrIndexPtr;


private:
	uint32_t getChildPtrBeginOffset() const;
	
	IndexTypes indexTypes() const { return (IndexTypes) (m_header & 0xF); }
	uint8_t childPtrBits() const { return ((m_header >> 4) & 0x1F) +1; }
	
public:
	LargeCompactTrieNodePrivate(const UByteArrayAdapter & nodeData);
	virtual ~LargeCompactTrieNodePrivate() {}

	virtual uint32_t childCount() const { return m_childCount;}
	virtual uint8_t charWidth() const { return ((m_header >> 10) & 0x3) + 1; }
	virtual uint8_t strLen() const { return m_strLen; }
	virtual bool hasMergeIndex() const { return m_header & 0x300; }
	virtual bool hasExactIndex() const { return indexTypes() & IT_EXACT;}
	virtual bool hasPrefixIndex() const { return indexTypes() & IT_PREFIX;}
	virtual bool hasSuffixIndex() const { return indexTypes() & IT_SUFFIX;}
	virtual bool hasSuffixPrefixIndex() const { return indexTypes() & IT_SUFFIX_PREFIX;}
	virtual bool hasAnyIndex() const { return indexTypes();}

	/** @return: Returns a pointer to the subclass, destruction will be handled by the outer non-private class **/
	virtual TrieNodePrivate* childAt(uint32_t pos) const;

	virtual uint32_t childCharAt(uint32_t pos) const;
	virtual UByteArrayAdapter strData() const;
	virtual std::string str() const;
	virtual int32_t posOfChar(uint32_t ucode) const;
	virtual void dump() const;

	virtual UByteArrayAdapter::SizeType getStorageSize() const;
	virtual UByteArrayAdapter::SizeType getHeaderStorageSize() const;
	virtual UByteArrayAdapter::SizeType getNodeStringStorageSize() const;
	virtual UByteArrayAdapter::SizeType getChildPtrStorageSize() const;
	virtual UByteArrayAdapter::SizeType getChildCharStorageSize() const;
	virtual UByteArrayAdapter::SizeType getIndexPtrStorageSize() const;

	virtual uint32_t getExactIndexPtr() const;
	virtual uint32_t getPrefixIndexPtr() const;
	virtual uint32_t getSuffixIndexPtr() const;
	virtual uint32_t getSuffixPrefixIndexPtr() const;
	virtual uint32_t getChildPtr(uint32_t pos) const;
};


struct LargeCompactTrieNodeCreator {
	enum ErrorTypes { NO_ERROR=0, TOO_MANY_CHILDREN=1, NODE_STRING_TOO_LONG=2, CHILD_PTR_FAILED=4, INDEX_PTR_FAILED=8, CHILD_CHAR_FAILED=16};
	static unsigned int createNewNode(const sserialize::Static::TrieNodeCreationInfo & nodeInfo, UByteArrayAdapter& destination);
	static unsigned int appendNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter& destination);
	static unsigned int prependNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, std::deque<uint8_t> & destination);
	static bool isError(unsigned int error);
	static std::string errorString(unsigned int error);
	static uint8_t getType();
};

}}//end namespace

#endif