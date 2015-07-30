#ifndef SIMPLE_STATIC_TRIE_NODE_PRIVATE_H
#define SIMPLE_STATIC_TRIE_NODE_PRIVATE_H
#include <stdint.h>
#include <string>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/Static/TrieNodePrivate.h>

/* Data and Layout definitions (v2):
 * 
 * This is a very simple space wasting implementation
 * 
 * 
 * * Layouts:
 *
 *
 * Node layout
 * 
 * --------------------------------------------------------------------------------------------------
 * CHILDRENCOUNT|CHARWIDTH|INDEXTYPE|STRINGLENGTH|STRING|CHILDREN_CHARS|CHILD_POINTERS|INDEXPOINTERS|
 * --------------------------------------------------------------------------------------------------
 *       2      |     1   |    1    |      1     |   *  |     4*n      |     4*n      |     4*4     |
 */

namespace sserialize {
namespace Static {

class SimpleStaticTrieCreationNode;

class SimpleTrieNodePrivate: public TrieNodePrivate {
public:
	friend class SimpleStaticTrieCreationNode;
protected:
	UByteArrayAdapter m_data;

	//Node info
	uint16_t m_childCount;
	uint8_t m_charWidth;
	IndexTypes m_indexTypes;
	uint8_t m_strLen;

	//data pointers:
	uint32_t m_strStart;
	uint32_t m_childArrayStart;
	uint32_t m_childPointerArrayStart;
	uint32_t m_indexPtrStart;
	uint32_t m_myEndPtr;
private:
	uint32_t getChildPtrBeginOffset() const;
	
public:
	SimpleTrieNodePrivate(const UByteArrayAdapter & nodeData);
	~SimpleTrieNodePrivate() {}

	virtual uint32_t childCount() const { return m_childCount;}
	virtual uint8_t charWidth() const { return m_charWidth; }
	virtual uint8_t strLen() const { return m_strLen; }
	virtual bool hasMergeIndex() const { return m_indexTypes & IT_MERGE_INDEX;}
	virtual bool hasExactIndex() const { return m_indexTypes & IT_EXACT;}
	virtual bool hasPrefixIndex() const { return m_indexTypes & IT_PREFIX;}
	virtual bool hasSuffixIndex() const { return m_indexTypes & IT_SUFFIX;}
	virtual bool hasSuffixPrefixIndex() const { return m_indexTypes & IT_SUFFIX_PREFIX;}
	virtual bool hasAnyIndex() const { return m_indexTypes;}

	/** @return: Returns a pointer to the subclass, destruction will be handled by the outer non-private class **/
	virtual TrieNodePrivate* childAt(uint32_t pos) const;

	virtual uint32_t childCharAt(uint32_t pos) const;
	virtual UByteArrayAdapter strData() const;
	virtual std::string str() const;
	virtual int32_t posOfChar(uint32_t ucode) const;
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


class SimpleStaticTrieCreationNode {
	SimpleTrieNodePrivate m_node;
	UByteArrayAdapter m_data;
public:
	enum ErrorTypes { NO_ERROR=0, TOO_MANY_CHILDREN=1, NODE_STRING_TOO_LONG=2, CHILD_PTR_FAILED=4};
	SimpleStaticTrieCreationNode(const UByteArrayAdapter & nodeData);
	~SimpleStaticTrieCreationNode() {}
	bool setChildPointer(uint32_t childNum, uint32_t offSetFromBeginning);
	/** @return: returns ErrorTypes **/
	static  unsigned int createNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter& destination);
	static  unsigned int appendNewNode(const TrieNodeCreationInfo & nodeInfo, UByteArrayAdapter& destination);
	static  unsigned int prependNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, std::deque< uint8_t >& destination);
	static bool isError(unsigned int error);
	static std::string errorString(unsigned int error);
	static uint8_t getType();
};

}}//end namespace

#endif