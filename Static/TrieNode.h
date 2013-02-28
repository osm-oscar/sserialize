#ifndef SSERIALIZE_STATIC_TRIE_NODE_H
#define SSERIALIZE_STATIC_TRIE_NODE_H
#include "TrieNodePrivates/TrieNodePrivate.h"
#include <sserialize/utility/refcounting.h>

namespace sserialize {
namespace Static {


class TrieNode: public RCWrapper<TrieNodePrivate> {
public:
	enum Types {
		T_EMPTY, T_SIMPLE, T_COMPACT
	};
private:
	uint32_t getSubTriePtrOffset() const;
	uint32_t getChildPtrBeginOffset() const;

public:

	TrieNode(TrieNodePrivate * node) : RCWrapper<TrieNodePrivate>(node) {}

	TrieNode(const TrieNode & other) : RCWrapper<TrieNodePrivate>(other) {}

	inline TrieNode & operator=(const TrieNode & other) {
		RCWrapper<TrieNodePrivate>::operator=(other);
		return *this;
	}

	~TrieNode() {}
	inline uint16_t childCount() const { return priv()->childCount();}
	///char width in bits
	inline uint8_t charWidth() const { return priv()->charWidth(); }
	inline uint32_t childCharAt(const uint16_t pos) const { return priv()->childCharAt(pos);}
	inline uint8_t strLen() const { return priv()->strLen(); }
	inline UByteArrayAdapter strData() const { return priv()->strData(); }
	inline std::string str() const { return priv()->str(); }
	inline int16_t posOfChar(const uint32_t ucode) const { return priv()->posOfChar(ucode);};
	inline TrieNode childAt(const uint16_t pos) const { return TrieNode( priv()->childAt(pos) );}

	inline bool hasMergeIndex() const { return priv()->hasMergeIndex();}
	inline bool hasExactIndex() const { return priv()->hasExactIndex();}
	inline bool hasPrefixIndex() const { return priv()->hasPrefixIndex();}
	inline bool hasSuffixIndex() const { return priv()->hasSuffixIndex();}
	inline bool hasSuffixPrefixIndex() const { return priv()->hasSuffixPrefixIndex();}
	inline bool hasAnyIndex() const { return priv()->hasAnyIndex();}

	inline void dump() const { priv()->dump();}

	inline uint32_t getStorageSize() const { return priv()->getStorageSize(); }
	inline uint32_t getHeaderStorageSize() const { return priv()->getHeaderStorageSize(); }
	inline uint32_t getNodeStringStorageSize() const { return priv()->getNodeStringStorageSize();}
	inline uint32_t getChildPtrStorageSize() const { return priv()->getChildPtrStorageSize();}
	inline uint32_t getChildCharStorageSize() const { return priv()->getChildCharStorageSize();}
	inline uint32_t getIndexPtrStorageSize() const { return priv()->getIndexPtrStorageSize();}

	inline uint32_t getIndexPtr() const { return 0xFFFFFFFF;}
	inline uint32_t getExactIndexPtr() const { return priv()->getExactIndexPtr();}
	inline uint32_t getPrefixIndexPtr() const { return priv()->getPrefixIndexPtr();}
	inline uint32_t getSuffixIndexPtr() const { return priv()->getSuffixIndexPtr();}
	inline uint32_t getSuffixPrefixIndexPtr() const { return priv()->getSuffixPrefixIndexPtr();}

	inline uint32_t getSubTriePtr() const { return priv()->getSubTriePtr();}
	inline uint32_t getChildPtr(const uint32_t pos) const { return priv()->getChildPtr(pos); }
};

}} //end namespace

#endif