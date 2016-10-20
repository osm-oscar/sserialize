#ifndef COMPACT_STATIC_TRIE_NODE_PRIVATE_H
#define COMPACT_STATIC_TRIE_NODE_PRIVATE_H
#include <stdint.h>
#include <string>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/Static/TrieNodePrivate.h>



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
 * ----------------------------------------------------------------------------------------------------------
 * amliiiii|epsSLLLL|LLLLLLLL|SLSLSLSL|SSSSSSSSS|CCCCCCCCCCCC|FCPFCPFCP|CPCPCPCP|  IDXPTRS |SUBTRIEPTR|
 * ----------------------------------------------------------------------------------------------------------
 * 1 byte  | 1 byte | 1 byte | 1 byte | SL byte |(1-2 byte)*l|  4 byte |2*(L-1) |CompactArr|  4 byte  |
 * a = width of the characters (0 => 1 byte, 1 => 2 bytes)
 * m = node has a merge index
 * e = node has a exact index
 * p = node has a prefix index
 * s = node has a suffix index
 * S = node has a suffixPrefix index
 * i = index bits per number count
 * l = if true, then there's another length byte following
 * L = length of the following char->pointer array
 * SL = length of the string
 * S = the node's string, not null-terminated, encoded as UTF-8, the first char is stored in the parent 
 * C = character (width is determined by a), encoded in UTF-32 (essentially it's just the unicde point)
 * FCP = displacement from the beginning of the tree to the first child node
 * CP = displacement from the beginning of the first child, every childnode-stripe adds a new offset
 * IDXPTR = Pointer to index (offset in index file
 * SUBTRIEPTR = Pointer to the subtrie (offset in trie file)
 */

namespace sserialize {
namespace Static {

class CompactStaticTrieCreationNode;

class CompactTrieNodePrivate: public TrieNodePrivate {
public:
	friend class CompactStaticTrieCreationNode;
protected:
	UByteArrayAdapter m_data;

	//Node info
	uint16_t m_nodeHeader;
	uint16_t m_childCount;
	uint8_t m_strLen;

	//data pointers:
	uint32_t m_strStart;
	uint32_t m_childArrayStart;
	uint32_t m_childPointerArrayStart;
	uint32_t m_indexPtrStart;
	uint32_t m_myEndPtr;


private:
	uint32_t getSubTriePtrOffset() const;
	uint32_t getChildPtrBeginOffset() const;
	
	IndexTypes indexTypes() const { return (IndexTypes) ((m_nodeHeader & 0xF0) >> 4);}
	uint8_t indexArrBpn() const { return ((m_nodeHeader >> 8) & 0x1F) + 1;}
	
public:
	CompactTrieNodePrivate(const UByteArrayAdapter & nodeData);
	~CompactTrieNodePrivate() {}

	virtual uint32_t childCount() const { return m_childCount;}
	virtual uint8_t charWidth() const { return (m_nodeHeader >> 15) + 1;}
	virtual uint8_t strLen() const { return m_strLen; }
	virtual bool hasMergeIndex() const { return (m_nodeHeader >> 14) & 0x1; }
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


class CompactStaticTrieCreationNode {
	CompactTrieNodePrivate m_node;
	UByteArrayAdapter m_data;
public:
	enum ErrorTypes { NO_ERROR=0, TOO_MANY_CHILDREN=1, NODE_STRING_TOO_LONG=2, CHILD_PTR_FAILED=4, INDEX_PTR_FAILED=8};
	CompactStaticTrieCreationNode(const UByteArrayAdapter & nodeData);
	~CompactStaticTrieCreationNode() {}
	bool setChildPointer(uint32_t childNum, uint32_t offSetFromBeginning);
	static unsigned int createNewNode(const sserialize::Static::TrieNodeCreationInfo & nodeInfo, UByteArrayAdapter& destination);
	static unsigned int appendNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter& destination);
	static unsigned int prependNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, std::deque<uint8_t> & destination);
	static bool isError(unsigned int error);
	static std::string errorString(unsigned int error);
	static uint8_t getType();
};

}}//end namespace

#endif