#ifndef STATIC_TRIE_NODE_PRIVATE_H
#define STATIC_TRIE_NODE_PRIVATE_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>
#include <string>

namespace sserialize {
namespace Static {

class TrieNodePrivate {
public:
	enum IndexTypes {
		IT_NONE=0, IT_EXACT=1, IT_PREFIX=2, IT_SUFFIX=4, IT_SUFFIX_PREFIX=8, IT_ALL=15, IT_MERGE_INDEX=0x80
	};
public:
	TrieNodePrivate() {}
	virtual ~TrieNodePrivate() {}

	//Virtual functions that need implementation
	virtual uint16_t childCount() const { return 0;}
	virtual uint8_t charWidth() const { return 0; }
	virtual uint8_t strLen() const { return 0; }

	virtual bool hasMergeIndex() const {return true; }
	virtual bool hasExactIndex() const { return false;}
	virtual bool hasPrefixIndex() const { return false;}
	virtual bool hasSuffixIndex() const { return false;}
	virtual bool hasSuffixPrefixIndex() const { return false;}
	virtual bool hasAnyIndex() const { return false;}


	/** @return: Returns a pointer to the subclass, destruction will be handled by the outer non-private class **/
	virtual TrieNodePrivate* childAt(uint16_t pos) const { return new TrieNodePrivate();}

	virtual uint32_t childCharAt(uint16_t pos) const { return 0;} 
	virtual UByteArrayAdapter strData() const { return UByteArrayAdapter();}
	virtual std::string str() const { return std::string();}
	virtual int16_t posOfChar(uint32_t ucode) const { return -1;}
	virtual void dump() const {}
	virtual uint32_t getStorageSize() const { return 0;}
	virtual uint32_t getHeaderStorageSize() const { return 0;}
	virtual uint32_t getNodeStringStorageSize() const { return 0;}
	virtual uint32_t getChildPtrStorageSize() const { return 0;}
	virtual uint32_t getChildCharStorageSize() const { return 0;}
	virtual uint32_t getIndexPtrStorageSize() const { return 0;}
	virtual uint32_t getExactIndexPtr() const { return 0;}
	virtual uint32_t getPrefixIndexPtr() const { return 0;}
	virtual uint32_t getSuffixIndexPtr() const { return 0;}
	virtual uint32_t getSuffixPrefixIndexPtr() const { return 0;}
	virtual uint32_t getSubTriePtr() const { return 0;}
	virtual uint32_t getChildPtr(uint32_t pos) const { return 0;}

};


struct TrieNodeCreationInfo {
	TrieNodeCreationInfo() : charWidth(1), childrenCount(0), indexTypes(TrieNodePrivate::IT_NONE), mergeIndex(true), exactIndexPtr(0), prefixIndexPtr(0), suffixIndexPtr(0), suffixPrefixIndexPtr(0) {}
	uint8_t charWidth;
	std::string nodeStr;
	uint32_t childrenCount;
	std::vector< uint32_t > childChars;
	std::vector< uint32_t > childPtrs;
	TrieNodePrivate::IndexTypes indexTypes;
	bool mergeIndex;
	uint32_t exactIndexPtr;
	uint32_t prefixIndexPtr;
	uint32_t suffixIndexPtr;
	uint32_t suffixPrefixIndexPtr;
};

}}//end namespace

#endif