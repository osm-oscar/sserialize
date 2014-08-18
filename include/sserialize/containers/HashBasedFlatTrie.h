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


namespace sserialize {

template<typename TValue>
class HashBasedFlatTrie;

namespace detail {
namespace HashBasedFlatTrie {

// class StaticString: public StaticStringBase {
// public:
// 	typedef const char * const_iterator;
// 	friend class HashBasedFlatTrie;
// private:
// 	const char * m_d;
// public:
// 	StaticString() : StaticStringBase(), m_d(0) {}
// 	StaticString(const StaticString & other) : StaticStringBase(other), m_d(other.m_d) {}
// 	StaticString(const char * begin, const char * end) : StaticStringBase(0, end-begin), m_d(begin) {}
// 	StaticString(const StaticStringBase & base, const char * d) : StaticStringBase(base), m_d(d) {}
// 	inline bool operator==(const StaticString & other) const {
// 		return (size() == other.size() ? std::equal(begin(), end(), other.begin()) : false);
// 	}
// 	inline bool operator<(const StaticString & other) const {
// 		return sserialize::unicodeIsSmaller(begin(), end(), other.begin(), other.end());
// 	}
// 	inline const_iterator begin() const { return m_d+m_off; }
// 	inline const_iterator cbegin() const { return m_d+m_off; }
// 	inline const_iterator end() const { return m_d+m_off+m_size; }
// 	inline const_iterator cend() const { return m_d+m_off+m_size; }
// 	inline char at(uint32_t pos) const { return *(begin()+pos);}
// 	
// 	inline size_t hash() const {
// 		uint64_t seed = 0;
// 		for(const_iterator it(cbegin()), end(cend()); it != end; ++it) {
// 			hash_combine(seed, *it);
// 		}
// 		return seed;
// 	}
// 	void dump(std::ostream & out) const {
// 		out.write(cbegin(), size());
// 	}
// 	void dump() const {
// 		dump(std::cout);
// 	}
// };

// bool operator==(const std::string & a, const StaticString & b) {
// 	return StaticString(a.c_str(), a.c_str()+a.size()) == b;
// }
// 
// bool operator==(const StaticString & b, const std::string & a) {
// 	return StaticString(a.c_str(), a.c_str()+a.size()) == b;
// }

///T_NODE_ENTRY_IT dereferences to std::pair<StaticString, TValue>
// template<typename T_NODE_ENTRY_IT, typename TValue>
// class Node {
// public:
// 	///This only works if the range is sorted and inner nodes are in the range as well.
// 	///An inner node is a node where a trie splits into children
// 	class Iterator {
// 	public:
// 		struct CompFunc {
// 			uint32_t posInStr;
// 			CompFunc(uint32_t posInStr) : posInStr(posInStr) {}
// 			inline bool operator()(uint32_t a, const std::pair<StaticString, TValue> & b) const {
// 				return a < utf8::peek_next(b.first.begin()+posInStr, b.first.end()); }
// 			inline bool operator()(const std::pair<StaticString, TValue> & a, uint32_t b) const {
// 				return utf8::peek_next(a.first.begin()+posInStr, a.first.end()) < b;
// 			}
// 		};
// 	private:
// 		T_NODE_ENTRY_IT m_childNodeBegin;
// 		T_NODE_ENTRY_IT m_childNodeEnd;
// 		T_NODE_ENTRY_IT m_childrenEnd;
// 		CompFunc m_compFunc;
// 	public:
// 	
// 		Iterator(const T_NODE_ENTRY_IT & parentBegin, const T_NODE_ENTRY_IT & parentEnd, uint32_t posInStr) :
// 		m_childNodeBegin(parentBegin), m_childNodeEnd(parentBegin), m_childrenEnd(parentEnd), m_compFunc(posInStr) {
// 			if (m_childNodeBegin != m_childrenEnd) {
// 				++m_childNodeEnd;
// 				operator++();
// 			}
// 		}
// 		~Iterator() {}
// 		Iterator & operator++() {
// 			m_childNodeBegin = m_childNodeEnd;
// 			uint32_t cp = utf8::peek_next(m_childNodeBegin->begin()+m_compFunc.posInStr, m_childNodeBegin->end());
// 			m_childNodeEnd = std::upper_bound(m_childNodeBegin, m_childrenEnd, cp, m_compFunc);
// 			return *this;
// 		}
// 		bool operator!=(const Iterator & other) {
// 			return m_childNodeBegin != other.m_childNodeBegin ||
// 					m_childNodeEnd != other.m_childNodeEnd ||
// 					m_childrenEnd != other.m_childrenEnd ||
// 					m_compFunc != other.m_compFunc;
// 		}
// 		Node operator*() const;
// 	};
// 	typedef Iterator const_iterator;
// private:
// 	T_NODE_ENTRY_IT m_begin;
// 	T_NODE_ENTRY_IT m_end;
// public:
// 	Node() {}
// 	Node(const T_NODE_ENTRY_IT & begin, const T_NODE_ENTRY_IT & end) : m_begin(begin), m_end(end) {}
// 	~Node() {}
// 	const StaticString & str() const { return m_begin->first; }
// 	const_iterator begin() const { return const_iterator(m_begin, m_end, str().size()); }
// 	const_iterator end() const { return const_iterator(m_end, m_end, str().size()); }
// 	TValue & value() {return m_begin->second;}
// };
// 
// template<typename T_NODE_ENTRY_IT, typename TValue>
// Node<T_NODE_ENTRY_IT, TValue>
// Node<T_NODE_ENTRY_IT, TValue>::Iterator::operator*() const {
// 	return Node(m_childNodeBegin, m_childNodeEnd);
// }


}}//end namespace detail


template<typename TValue>
class HashBasedFlatTrie {
private:
	class StringHandler;
public:

	class StaticString {
	public:
		typedef uint32_t OffsetType;
		static constexpr OffsetType special = 0xFFFFFFFF;
	protected:
		friend class HashBasedFlatTrie<TValue>;
		uint32_t m_off;
		uint32_t m_size;
		StaticString(uint32_t offset, uint32_t size) : m_off(offset), m_size(size) {}
		StaticString(uint32_t size) : m_off(special), m_size(size) {}
	public:
		StaticString() : m_off(special), m_size(special) {}
		StaticString(const StaticString & other) : m_off(other.m_off), m_size(other.m_size) {}
		virtual ~StaticString() {}
		inline uint32_t offset() const { return m_off; }
		inline uint32_t size() const { return m_size; }
		inline bool isSpecial() const { return m_off == special; }
		inline bool isInvalid() const { return m_off == special && m_size == special; }
		StaticString addOffset(OffsetType off) { return StaticString(m_off + off, m_size-off); }
	};

	typedef StaticString StaticString;
	typedef TValue mapped_type;
	typedef StaticString key_type;
	typedef MMVector<char> StringStorage;
	typedef MMVector< std::pair<key_type, TValue> > HTValueStorage;
	typedef MMVector<uint32_t> HTStorage;
	typedef typename HTValueStorage::const_iterator const_iterator;
	typedef typename HTValueStorage::iterator iterator;
private:

	struct StringHandler {
		const StringStorage * strStorage;
		const char * specialString;
		StringHandler(const StringStorage * strStorage = 0) : strStorage(strStorage) {}
		inline const char * strBegin(const StaticString & str) const {
			return str.isSpecial() ? specialString : strStorage->cbegin()+str.offset();
		}
		const char * strEnd(const StaticString & str) const { return strBegin(str)+str.size();} 
	};
	
	struct CompFunc {
		const StringHandler * strHandler;
		uint32_t posInStr;
		CompFunc(const StringHandler * strHandler, uint32_t posInStr) : strHandler(strHandler), posInStr(posInStr) {}
		inline bool operator()(uint32_t a, const std::pair<StaticString, TValue> & b) const {
			return a < utf8::peek_next(strHandler->strBegin(b.first)+posInStr, strHandler->strEnd(b.first));
		}
		inline bool operator()(const std::pair<StaticString, TValue> & a, uint32_t b) const {
			return utf8::peek_next(strHandler->strBegin(a.first)+posInStr, strHandler->strEnd(a.first)) < b;
		}
		inline bool operator==(const CompFunc & other) const { return posInStr == other.posInStr && strHandler == other.strHandler; }
		inline bool operator!=(const CompFunc & other) const { return posInStr == other.posInStr && strHandler == other.strHandler; }
	};

	struct StringEq {
		const StringHandler * strHandler;
		StringEq(const StringHandler * strHandler = 0) : strHandler(strHandler) {}
		std::size_t operator()(const key_type & a, const key_type & b) const {
			return (a.size() != b.size() ? false : std::equal(strHandler->strBegin(a), strHandler->strEnd(a), strHandler->strBegin(b)));
		}
	};
	struct StringSmaller {
		const StringHandler * strHandler;
		StringSmaller(const StringHandler * strHandler = 0) : strHandler(strHandler) {}
		std::size_t operator()(const key_type & a, const key_type & b) const {
			return sserialize::unicodeIsSmaller(strHandler->strBegin(a), strHandler->strEnd(a), strHandler->strBegin(b), strHandler->strEnd(b));
		}
	};
public:
	class Node {
	public:
		///This only works if the range is sorted and inner nodes are in the range as well.
		///An inner node is a node where a trie splits into children
		class Iterator {
		public:
		private:
			HashBasedFlatTrie::const_iterator m_childNodeBegin;
			HashBasedFlatTrie::const_iterator m_childNodeEnd;
			HashBasedFlatTrie::const_iterator m_childrenEnd;
			HashBasedFlatTrie::CompFunc m_compFunc;
		public:
			Iterator(const HashBasedFlatTrie::const_iterator & parentBegin, const HashBasedFlatTrie::const_iterator & parentEnd, const HashBasedFlatTrie::CompFunc & compFunc);
			~Iterator();
			Iterator & operator++();
			bool operator!=(const Iterator & other);
			bool operator==(const Iterator & other);
			Node operator*() const;
		};
		typedef Iterator const_iterator;
	private:
		HashBasedFlatTrie::const_iterator m_begin;
		HashBasedFlatTrie::const_iterator m_end;
		const StringHandler * m_strHandler;
	public:
		Node() {}
		Node(const HashBasedFlatTrie::const_iterator & begin, const HashBasedFlatTrie::const_iterator & end, const StringHandler * strHandler) :
		m_begin(begin), m_end(end), m_strHandler(strHandler) {}
		~Node() {}
		const StaticString & str() const { return m_begin->first; }
		const_iterator begin() const { return const_iterator(m_begin, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		const_iterator end() const { return const_iterator(m_end, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		TValue & value() {return m_begin->second;}
	};
private:
	struct HashFunc1 {
		const StringHandler * strHandler;
		HashFunc1(const StringHandler * strHandler = 0) : strHandler(strHandler) {}
		std::size_t operator()(const key_type & a) const {
			uint64_t seed = 0;
			typedef const char * const_iterator;
			for(const_iterator it(strHandler->strBegin(a)), end(strHandler->strEnd(a)); it != end; ++it) {
				hash_combine(seed, *it);
			}
			return seed;
		}
	};
	struct HashFunc2: public HashFunc1 {
		HashFunc2(const StringHandler * strHandler = 0) : HashFunc1(strHandler) {}
		std::size_t operator()(const key_type & a) const {
			return ~HashFunc1::operator()(a);
		}
	};
	typedef OADHashTable<StaticString, TValue, HashFunc1, HashFunc2,  HTValueStorage, HTStorage, StringEq> HashTable;
private:
	StringStorage m_stringData;
	StringHandler m_strHandler;
	HashTable m_ht;
private:
	void finalize(uint64_t nodeBegin, uint64_t nodeEnd, uint32_t posInStr);
public:
	HashBasedFlatTrie() :
	m_stringData(sserialize::MM_SHARED_MEMORY),
	m_ht(HTValueStorage(sserialize::MM_SHARED_MEMORY),
	HTStorage(sserialize::MM_SHARED_MEMORY))
	{
		m_strHandler.specialString = 0;
		m_strHandler.strStorage = &m_stringData;
		const StringHandler * strHandlerPtr = &m_strHandler;
		m_ht = HashTable(HashFunc1(strHandlerPtr), HashFunc2(strHandlerPtr), StringEq(strHandlerPtr), 0.8, HTValueStorage(sserialize::MM_SHARED_MEMORY), HTStorage(sserialize::MM_SHARED_MEMORY));
	}
	~HashBasedFlatTrie() {}
	
	const HashTable & hashTable() const { return m_ht; }
	
	uint32_t size() const { return m_ht.size(); }
	
	const_iterator begin() const { return m_ht.cbegin(); }
	const_iterator cbegin() const { return m_ht.cbegin(); }
	const_iterator end() const { return m_ht.cend(); }
	const_iterator cend() const { return m_ht.cend(); }
	///Adding items invalidates this
	const char * staticStringBegin(const StaticString & str) const { return m_strHandler.strBegin(str); }
	///Adding items invalidates this
	const char * staticStringEnd(const StaticString & str) const { return m_strHandler.strEnd(str); }
	std::string toStr(const StaticString & str) const { return std::string(staticStringBegin(str), staticStringEnd(str)); }
	///@return a very simple string type which is valid while this class exists
	StaticString insert(const std::string & a);
	StaticString insert(const StaticString & a);
	TValue & operator[](const StaticString & str);
	///You have to call finalize() before using this @param prefixMatch strIt->strEnd can be a prefix of the path
	template<typename T_OCTET_ITERATOR>
	Node findNode(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) const;
	
	
	///call this if you want to navigate the trie
	///you can always choose to add more strings to trie, but then you'd have to call this again
	void finalize();
	
	///before using this, finalize has to be called and no inserts were made afterwards
	Node root() { return Node(m_ht.begin(), m_ht.end(), &m_strHandler); }
	
	///you can only call this after finalize()
	bool append(UByteArrayAdapter & dest);
};


template<typename TValue>
HashBasedFlatTrie<TValue>::Node::Iterator::Iterator(const HashBasedFlatTrie::const_iterator & parentBegin, const HashBasedFlatTrie::const_iterator & parentEnd, const HashBasedFlatTrie::CompFunc & compFunc) :
m_childNodeBegin(parentBegin), m_childNodeEnd(parentBegin), m_childrenEnd(parentEnd), m_compFunc(compFunc) {
	if (m_childNodeBegin != m_childrenEnd) {
		++m_childNodeEnd;
		operator++();
	}
}

template<typename TValue>
HashBasedFlatTrie<TValue>::Node::Iterator::~Iterator() {}

template<typename TValue>
typename HashBasedFlatTrie<TValue>::Node::Iterator &
HashBasedFlatTrie<TValue>::Node::Iterator::operator++() {
	m_childNodeBegin = m_childNodeEnd;
	uint32_t cp = utf8::peek_next( m_compFunc.strHandler->strBegin(m_childNodeBegin->first)+m_compFunc.posInStr, m_compFunc.strHandler->strEnd(m_childNodeBegin->first));
	m_childNodeEnd = std::upper_bound(m_childNodeBegin, m_childrenEnd, cp, m_compFunc);
	return *this;
}

template<typename TValue>
bool
HashBasedFlatTrie<TValue>::Node::Iterator::operator!=(const Iterator & other) {
	return m_childNodeBegin != other.m_childNodeBegin ||
			m_childNodeEnd != other.m_childNodeEnd ||
			m_childrenEnd != other.m_childrenEnd ||
			m_compFunc != other.m_compFunc;
}

template<typename TValue>
bool
HashBasedFlatTrie<TValue>::Node::Iterator::operator==(const Iterator & other) {
	return m_childNodeBegin == other.m_childNodeBegin ||
			m_childNodeEnd == other.m_childNodeEnd ||
			m_childrenEnd == other.m_childrenEnd ||
			m_compFunc == other.m_compFunc;
}

template<typename TValue>
typename HashBasedFlatTrie<TValue>::Node
HashBasedFlatTrie<TValue>::Node::Iterator::operator*() const {
	return Node(m_childNodeBegin, m_childNodeEnd, m_compFunc.strHandler);
}


//end of internal classes implementations

template<typename TValue>
typename HashBasedFlatTrie<TValue>::StaticString
HashBasedFlatTrie<TValue>::insert(const std::string & a) {
	m_strHandler.specialString = a.c_str();
	StaticString ret = insert(StaticString(a.size()));
	m_strHandler.specialString = 0;
	return ret;
}


template<typename TValue>
TValue &
HashBasedFlatTrie<TValue>::operator[](const StaticString & a) {
	if (!a.isSpecial()) {
		return m_ht[a];
	}
	else {
		typename StaticString::OffsetType strOff = m_stringData.size();
		m_stringData.push_back(m_strHandler.strBegin(a), m_strHandler.strEnd(a));
		return m_ht[StaticString(strOff, a.size())];
	}
}

template<typename TValue>
typename HashBasedFlatTrie<TValue>::StaticString
HashBasedFlatTrie<TValue>::insert(const StaticString & a) {
	typename HashTable::const_iterator it(m_ht.find(a));
	if (it != m_ht.cend()) {
		return it->first;
	}
	if (!a.isSpecial()) {
		m_ht.insert(a);
		return a;
	}
	else {//special string (comes from outside)
		typename StaticString::OffsetType strOff = m_stringData.size();
		m_stringData.push_back(m_strHandler.strBegin(a), m_strHandler.strEnd(a));
		StaticString ns(strOff, a.size());
		m_ht.insert(ns);
		return ns;
	}
}

template<typename TValue>
void HashBasedFlatTrie<TValue>::finalize(uint64_t nodeBeginOff, uint64_t nodeEndOff, uint32_t posInStr) {
	if (nodeBeginOff != nodeEndOff) {
		{//find the end of our current node
			const_iterator nodeBegin = begin()+nodeBeginOff;
			const_iterator nodeEnd = begin()+nodeEndOff;
			const char * nodeStrEnd = m_strHandler.strEnd(nodeBegin->first);
			const char * beginLookUp = m_strHandler.strBegin(nodeBegin->first) + posInStr;
			const char * endLookUp = beginLookUp;
			while (posInStr < nodeBegin->first.size()) {
				uint32_t cp = utf8::next(endLookUp, nodeStrEnd);
				const_iterator endChildNode = std::upper_bound(nodeBegin, nodeEnd, cp, CompFunc(&m_strHandler, posInStr));
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
		if ((begin()+nodeBeginOff)->first.size() > posInStr) {//our current node has a missing parent, add it to the hash
			m_ht.insert(StaticString((begin()+nodeBeginOff)->first.offset(), posInStr));
		}
		else {//the first string is the correct internal node string
			++nodeBeginOff;
		}
		CompFunc compFunc(&m_strHandler, posInStr);
		//nodeBegin now points to the beginning of the children (the string of the first child)
		while(nodeBeginOff != nodeEndOff) {
			const_iterator nodeBegin = begin()+nodeBeginOff;
			const_iterator nodeEnd = begin()+nodeEndOff;
			const char * childNextCP = m_strHandler.strBegin(nodeBegin->first)+posInStr;
			uint32_t cp = utf8::next(childNextCP, m_strHandler.strEnd(nodeBegin->first));
			const_iterator endChildNode = std::upper_bound(nodeBegin, nodeEnd, cp, compFunc);
			uint64_t childEndOff = endChildNode-begin();
			finalize(nodeBeginOff, childEndOff, childNextCP - m_strHandler.strBegin(nodeBegin->first));
			nodeBeginOff = childEndOff;
		}
	}
}

template<typename TValue>
void HashBasedFlatTrie<TValue>::finalize() {
	const StringHandler * strHandler = &m_strHandler;
	auto sortFunc = [strHandler](const typename HashTable::value_type & a, const typename HashTable::value_type & b) {
		return sserialize::unicodeIsSmaller(strHandler->strBegin(a.first), strHandler->strEnd(a.first), strHandler->strBegin(b.first), strHandler->strEnd(b.first));
	};
	m_ht.sort(sortFunc);
	finalize(0, m_ht.size(), 0);
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
		maxBegin = std::max<const char*>(maxBegin, m_strHandler.strBegin(x.first));
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
		tsCreator.set(count, 0, m_strHandler.strBegin(x.first)-strDataBegin);
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