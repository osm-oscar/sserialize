#ifndef STATIC_TRIE_NODE_PRIVATE_H
#define STATIC_TRIE_NODE_PRIVATE_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>
#include <string>
#include <limits>

namespace sserialize {
namespace Static {

class TrieNodePrivate: public RefCountObject {
public:
	enum IndexTypes {
		IT_NONE=0, IT_EXACT=1, IT_PREFIX=2, IT_SUFFIX=4, IT_SUFFIX_PREFIX=8, IT_ALL=15, IT_MERGE_INDEX=0x80
	};
	static const uint32_t npos = std::numeric_limits<uint32_t>::max();
public:
	TrieNodePrivate() : RefCountObject() {}
	virtual ~TrieNodePrivate() {}

	//Virtual functions that need implementation
	virtual uint32_t childCount() const = 0;
	virtual uint8_t charWidth() const = 0;
	virtual uint8_t strLen() const = 0;

	virtual bool hasMergeIndex() const = 0;
	virtual bool hasExactIndex() const = 0;
	virtual bool hasPrefixIndex() const = 0;
	virtual bool hasSuffixIndex() const = 0;
	virtual bool hasSuffixPrefixIndex() const = 0;
	virtual bool hasAnyIndex() const = 0;


	/** @return: Returns a pointer to the subclass, destruction will be handled by the outer non-private class **/
	virtual TrieNodePrivate* childAt(uint32_t pos) const = 0;

	virtual uint32_t childCharAt(uint32_t pos) const = 0;
	virtual UByteArrayAdapter strData() const = 0;
	virtual std::string str() const = 0;
	virtual int32_t posOfChar(uint32_t ucode) const = 0;
	virtual void dump() const = 0;
	virtual uint32_t getStorageSize() const = 0;
	virtual uint32_t getHeaderStorageSize() const = 0;
	virtual uint32_t getNodeStringStorageSize() const = 0;
	virtual uint32_t getChildPtrStorageSize() const = 0;
	virtual uint32_t getChildCharStorageSize() const = 0;
	virtual uint32_t getIndexPtrStorageSize() const = 0;
	virtual uint32_t getExactIndexPtr() const = 0;
	virtual uint32_t getPrefixIndexPtr() const = 0;
	virtual uint32_t getSuffixIndexPtr() const = 0;
	virtual uint32_t getSuffixPrefixIndexPtr() const = 0;
	virtual uint32_t getChildPtr(uint32_t pos) const = 0;

};

class EmptyTrieNodePrivate: public TrieNodePrivate {
public:
	EmptyTrieNodePrivate() {}
	virtual ~EmptyTrieNodePrivate() {}
	
	virtual uint32_t childCount() const { return 0;}
	virtual uint8_t charWidth() const { return 0; }
	virtual uint8_t strLen() const { return 0; }

	virtual bool hasMergeIndex() const {return true; }
	virtual bool hasExactIndex() const { return false;}
	virtual bool hasPrefixIndex() const { return false;}
	virtual bool hasSuffixIndex() const { return false;}
	virtual bool hasSuffixPrefixIndex() const { return false;}
	virtual bool hasAnyIndex() const { return false;}


	/** @return: Returns a pointer to the subclass, destruction will be handled by the outer non-private class **/
	virtual TrieNodePrivate* childAt(uint32_t pos) const { return new EmptyTrieNodePrivate();}

	virtual uint32_t childCharAt(uint32_t pos) const { return 0;} 
	virtual UByteArrayAdapter strData() const { return UByteArrayAdapter();}
	virtual std::string str() const { return std::string();}
	virtual int32_t posOfChar(uint32_t ucode) const { return -1;}
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
	virtual uint32_t getChildPtr(uint32_t pos) const { return 0;}
};

struct TrieNodeCreationInfo {
	TrieNodeCreationInfo() : indexTypes(TrieNodePrivate::IT_NONE), mergeIndex(true), exactIndexPtr(0), prefixIndexPtr(0), suffixIndexPtr(0), suffixPrefixIndexPtr(0) {}
	std::string nodeStr;
	std::vector< uint32_t > childChars; //sorted
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