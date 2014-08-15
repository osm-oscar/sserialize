#ifndef SSERIALIZE_HASH_BASED_FLAT_TRIE_H
#define SSERIALIZE_HASH_BASED_FLAT_TRIE_H
#include <sserialize/templated/OADHashTable.h>
#include <sserialize/templated/MMVector.h>
#include <sserialize/templated/TransformIterator.h>
#include <sserialize/utility/MmappedMemory.h>
#include <sserialize/utility/stringfunctions.h>
#include <sserialize/utility/hashspecializations.h>
#include <sserialize/utility/CompactUintArray.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <vendor/libs/minilzo/lzoconf.h>


namespace sserialize {

template<typename TValue>
class HashBasedFlatTrie;

namespace detail {
namespace HashBasedFlatTrie {

class StaticString {
public:
	typedef const char * const_iterator;
	friend class HashBasedFlatTrie;
private:
	const char * m_begin;
	const char * m_end;
public:
	StaticString() : m_begin(0), m_end(0) {}
	StaticString(const StaticString & other) : m_begin(other.m_begin), m_end(other.m_end) {}
	StaticString(const char * begin, const char * end) : m_begin(begin), m_end(end) {}
	inline uint32_t size() const { return m_end-m_begin; }
	inline bool operator==(const StaticString & other) const {
		return (size() == other.size() ? std::equal(m_begin, m_end, other.m_begin) : false);
	}
	inline bool operator<(const StaticString & other) const {
		return sserialize::unicodeIsSmaller(m_begin, m_end, other.m_begin, other.m_end);
	}
	inline const_iterator begin() const { return m_begin; }
	inline const_iterator cbegin() const { return m_begin; }
	inline const_iterator end() const { return m_end; }
	inline const_iterator cend() const { return m_end; }
	inline char at(uint32_t pos) const { return *(begin()+pos);}
	
	inline size_t hash() const {
		uint64_t seed = 0;
		for(const_iterator it(cbegin()), end(cend()); it != end; ++it) {
			hash_combine(seed, *it);
		}
		return seed;
	}
	void dump(std::ostream & out) const {
		out.write(cbegin(), size());
	}
	void dump() const {
		dump(std::cout);
	}
};

bool operator==(const std::string & a, const StaticString & b) {
	return StaticString(a.c_str(), a.c_str()+a.size()) == b;
}

bool operator==(const StaticString & b, const std::string & a) {
	return StaticString(a.c_str(), a.c_str()+a.size()) == b;
}

///T_NODE_ENTRY_IT dereferences to std::pair<StaticString, TValue>
template<typename T_NODE_ENTRY_IT, typename TValue>
class Node {
public:
	///This only works if the range is sorted and inner nodes are in the range as well.
	///An inner node is a node where a trie splits into children
	class Iterator {
	public:
		struct CompFunc {
			uint32_t posInStr;
			CompFunc(uint32_t posInStr) : posInStr(posInStr) {}
			inline bool operator()(uint32_t a, const std::pair<StaticString, TValue> & b) const {
				return a < utf8::peek_next(b.first.begin()+posInStr, b.first.end()); }
			inline bool operator()(const std::pair<StaticString, TValue> & a, uint32_t b) const {
				return utf8::peek_next(a.first.begin()+posInStr, a.first.end()) < b;
			}
		};
	private:
		T_NODE_ENTRY_IT m_childNodeBegin;
		T_NODE_ENTRY_IT m_childNodeEnd;
		T_NODE_ENTRY_IT m_childrenEnd;
		CompFunc m_compFunc;
	public:
	
		Iterator(const T_NODE_ENTRY_IT & parentBegin, const T_NODE_ENTRY_IT & parentEnd, uint32_t posInStr) :
		m_childNodeBegin(parentBegin), m_childNodeEnd(parentBegin), m_childrenEnd(parentEnd), m_compFunc(posInStr) {
			if (m_childNodeBegin != m_childrenEnd) {
				++m_childNodeEnd;
				operator++();
			}
		}
		~Iterator() {}
		Iterator & operator++() {
			m_childNodeBegin = m_childNodeEnd;
			uint32_t cp = utf8::peek_next(m_childNodeBegin->begin()+m_compFunc.posInStr, m_childNodeBegin->end());
			m_childNodeEnd = std::upper_bound(m_childNodeBegin, m_childrenEnd, cp, m_compFunc);
			return *this;
		}
		bool operator!=(const Iterator & other) {
			return m_childNodeBegin != other.m_childNodeBegin ||
					m_childNodeEnd != other.m_childNodeEnd ||
					m_childrenEnd != other.m_childrenEnd ||
					m_compFunc != other.m_compFunc;
		}
		Node operator*() const;
	};
	typedef Iterator const_iterator;
private:
	T_NODE_ENTRY_IT m_begin;
	T_NODE_ENTRY_IT m_end;
public:
	Node() {}
	Node(const T_NODE_ENTRY_IT & begin, const T_NODE_ENTRY_IT & end) : m_begin(begin), m_end(end) {}
	~Node() {}
	const StaticString & str() const { return m_begin->first; }
	const_iterator begin() const { return const_iterator(m_begin, m_end, str().size()); }
	const_iterator end() const { return const_iterator(m_end, m_end, str().size()); }
	TValue & value() {return m_begin->second;}
};

template<typename T_NODE_ENTRY_IT, typename TValue>
Node<T_NODE_ENTRY_IT, TValue>
Node<T_NODE_ENTRY_IT, TValue>::Iterator::operator*() const {
	return Node(m_childNodeBegin, m_childNodeEnd);
}


}}//end namespace detail


template<typename TValue>
class HashBasedFlatTrie {
public:
	typedef detail::HashBasedFlatTrie::StaticString StaticString;
	typedef TValue mapped_type;
	typedef StaticString key_type;
	typedef StaticString HTKey;
	typedef MMVector< std::pair<HTKey, TValue> > HTValueStorage;
	typedef MMVector<uint32_t> HTStorage;
	typedef typename HTValueStorage::const_iterator const_iterator;
	typedef typename HTValueStorage::iterator iterator;
	typedef detail::HashBasedFlatTrie::Node<iterator, mapped_type> Node;
private:
	struct HashFunc1 {
		std::size_t operator()(const HTKey & a) const {
			return a.hash();
		}
	};
	struct HashFunc2 {
		std::size_t operator()(const HTKey & a) const {
			return ~a.hash();
		}
	};
	typedef OADHashTable<StaticString, TValue, HashFunc1, HashFunc2,  HTValueStorage, HTStorage> HashTable;
private:
	MMVector<char> m_stringData;
	HashTable m_ht;
private:
	void finalize(typename HashTable::const_iterator nodeBegin, typename HashTable::const_iterator nodeEnd, uint32_t posInStr);
public:
	HashBasedFlatTrie() :
	m_stringData(sserialize::MM_SHARED_MEMORY),
	m_ht(HTValueStorage(sserialize::MM_SHARED_MEMORY),
	HTStorage(sserialize::MM_SHARED_MEMORY))
	{}
	~HashBasedFlatTrie() {}
	uint32_t size() const { return m_ht.size(); }
	
	const_iterator begin() const { return m_ht.cbegin(); }
	const_iterator cbegin() const { return m_ht.cbegin(); }
	const_iterator end() const { return m_ht.cend(); }
	const_iterator cend() const { return m_ht.cend(); }
	
	StaticString insert(const std::string & a);
	StaticString insert(const StaticString & a);
	TValue & operator[](const StaticString & str);
	
	///call this if you want to navigate the trie
	///you can always choose to add more strings to trie, but then you'd have to call this again
	void finalize();
	
	///before using this, finalize has to be called and no inserts were made afterwards
	Node root() { return Node(m_ht.begin(), m_ht.end()); }
	
	///you can only call this after finalize()
	bool append(UByteArrayAdapter & dest);
};

template<typename TValue>
typename HashBasedFlatTrie<TValue>::StaticString
HashBasedFlatTrie<TValue>::insert(const std::string & a) {
	return insert(StaticString(a.c_str(), a.c_str()+a.size()));
}


template<typename TValue>
TValue &
HashBasedFlatTrie<TValue>::operator[](const StaticString & a) {
	if (a.begin() >= m_stringData.begin() && a.end() <= m_stringData.end()) {
		return m_ht[a];
	}
	else {
		const char * nsB = m_stringData.end();
		m_stringData.push_back(a.begin(), a.end());
		const char * nsE = m_stringData.end();
		return m_ht[StaticString(nsB, nsE)];
	}
}

template<typename TValue>
typename HashBasedFlatTrie<TValue>::StaticString
HashBasedFlatTrie<TValue>::insert(const StaticString & a) {
	typename HashTable::const_iterator it(m_ht.find(a));
	if (it != m_ht.cend()) {
		return it->first;
	}
	if (a.begin() >= m_stringData.begin() && a.end() <= m_stringData.end()) {
		m_ht.insert(a);
		return a;
	}
	else {
		const char * nsB = m_stringData.end();
		m_stringData.push_back(a.begin(), a.end());
		const char * nsE = m_stringData.end();
		StaticString ns(nsB, nsE);
		m_ht.insert(ns);
		return ns;
	}
}

template<typename TValue>
void HashBasedFlatTrie<TValue>::finalize(typename HashTable::const_iterator nodeBegin, typename HashTable::const_iterator nodeEnd, uint32_t posInStr) {
	if (nodeBegin != nodeEnd) {
		{//find the end of our current node
			StaticString::const_iterator beginLookUp = nodeBegin->first.begin() + posInStr;
			StaticString::const_iterator endLookUp = beginLookUp;
			while (posInStr < nodeBegin->first.size()) {
				uint32_t cp = utf8::next(endLookUp, nodeBegin->first.end());
				typedef typename Node::Iterator::CompFunc CompFunc;
				typename HashTable::const_iterator endChildNode = std::upper_bound(nodeBegin, nodeEnd, cp, CompFunc(posInStr));
				if (endChildNode == nodeEnd) {
					posInStr += endLookUp - beginLookUp;
					beginLookUp = endLookUp;
				}
				else {
					break;
				}
			}
		}
		//if we reach this, the following is true: posInStr points to the next unicodepoint
		if (nodeBegin->first.size() > posInStr) {//our current node has a missing parent, add it to the hash
			m_ht.insert(StaticString(nodeBegin->first.cbegin(), nodeBegin->first.cbegin()+posInStr));
		}
		else {//the first string is the correct internal node string
			++nodeBegin;
		}
		typename Node::Iterator::CompFunc compFunc(posInStr);
		//nodeBegin now points to the beginning of the children (the string of the first child)
		while(nodeBegin != nodeEnd) {
			StaticString::const_iterator childNextCP = nodeBegin->first.begin()+posInStr;
			uint32_t cp = utf8::next(childNextCP, nodeBegin->first.end());
			typename HashTable::const_iterator endChildNode = std::upper_bound(nodeBegin, nodeEnd, cp, compFunc);
			finalize(nodeBegin, endChildNode, childNextCP - nodeBegin->first.begin());
			nodeBegin = endChildNode;
		}
	}
}

template<typename TValue>
void HashBasedFlatTrie<TValue>::finalize() {
	auto sortFunc = [](const typename HashTable::value_type & a, const typename HashTable::value_type & b) {return a < b;};
	m_ht.sort(sortFunc);
	finalize(m_ht.cbegin(), m_ht.cend(), 0);
	m_ht.sort(sortFunc);
}

template<typename TValue>
bool HashBasedFlatTrie<TValue>::append(UByteArrayAdapter & dest) {
	finalize();
	dest.putUint8(1); //version
	dest.putOffset(m_stringData.size());
	dest.put(reinterpret_cast<const uint8_t*>(m_stringData.begin()), m_stringData.size());
	const char * maxBegin = m_stringData.begin();
	uint32_t maxLen = 0;
	for(const auto & x : m_ht) {
		maxBegin = std::max<const char*>(maxBegin, x.first.begin());
		maxLen = std::max<uint32_t>(maxLen, x.first.size());
	}
	uint64_t maxOffset = maxBegin - m_stringData.begin();
	if (maxOffset > std::numeric_limits<uint32_t>::max()) {
		throw sserialize::OutOfBoundsException("String data is too large");
	}
	
	MultiVarBitArrayCreator tsCreator(std::vector<uint8_t>({CompactUintArray::minStorageBits(maxOffset), CompactUintArray::minStorageBits(maxLen)}), dest);
	uint32_t count = 0;
	const char * strDataBegin = m_stringData.begin();
	for(const auto & x : m_ht) {
		tsCreator.set(count, 0, x.first.begin()-strDataBegin);
		tsCreator.set(count, 1, x.first.size());
		++count;
	}
	tsCreator.flush();
	Static::ArrayCreator<TValue> vsCreator(dest);
	vsCreator.reserveOffsets(m_ht.size());
	for(const auto & x : m_ht) {
		vsCreator.put(x.second);
	}
	vsCreator.flush();
	return true;
}


}//end namespac

#endif