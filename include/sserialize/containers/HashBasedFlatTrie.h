#ifndef SSERIALIZE_HASH_BASED_FLAT_TRIE_H
#define SSERIALIZE_HASH_BASED_FLAT_TRIE_H
#include <sserialize/templated/OADHashTable.h>
#include <sserialize/templated/MMVector.h>
#include <sserialize/templated/TransformIterator.h>
#include <sserialize/utility/MmappedMemory.h>
#include <sserialize/utility/stringfunctions.h>
#include <sserialize/utility/hashspecializations.h>
#include <sserialize/utility/CompactUintArray.h>
#include <sserialize/utility/ProgressInfo.h>
#include <sserialize/utility/TimeMeasuerer.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/DynamicVector.h>


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
		~StaticString() {}
		inline uint32_t offset() const { return m_off; }
		inline uint32_t size() const { return m_size; }
		inline bool isSpecial() const { return m_off == special; }
		inline bool isInvalid() const { return m_off == special && m_size == special; }
		StaticString addOffset(OffsetType off) const { return StaticString(m_off + off, m_size-off); }
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
		inline bool operator!=(const CompFunc & other) const { return posInStr != other.posInStr && strHandler != other.strHandler; }
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
	class Node;
	typedef std::shared_ptr<Node> NodePtr;
	typedef std::shared_ptr<const Node> ConstNodePtr;
	class Node {
		friend class HashBasedFlatTrie<TValue>;
	public:
		///This only works if the range is sorted and inner nodes are in the range as well.
		///An inner node is a node where a trie splits into children
		class Iterator {
		public:
			typedef Node value_type;
		private:
			HashBasedFlatTrie::iterator m_childNodeBegin;
			HashBasedFlatTrie::iterator m_childNodeEnd;
			HashBasedFlatTrie::iterator m_childrenEnd;
			HashBasedFlatTrie::CompFunc m_compFunc;
		public:
			Iterator(const HashBasedFlatTrie::iterator & parentBegin, const HashBasedFlatTrie::iterator & parentEnd, const HashBasedFlatTrie::CompFunc & compFunc);
			~Iterator();
			Iterator & operator++();
			bool operator!=(const Iterator & other);
			bool operator==(const Iterator & other);
			NodePtr operator*() const;
		};
		typedef Iterator const_iterator;
		typedef Iterator iterator;
	private:
		HashBasedFlatTrie::iterator m_begin;
		HashBasedFlatTrie::iterator m_end;
		const StringHandler * m_strHandler;
	public:
		Node() {}
		Node(const Node & other) : m_begin(other.m_begin), m_end(other.m_end), m_strHandler(other.m_strHandler) {}
		Node(const HashBasedFlatTrie::iterator & begin, const HashBasedFlatTrie::iterator & end, const StringHandler * strHandler) :
		m_begin(begin), m_end(end), m_strHandler(strHandler) {}
		~Node() {}
		///empty dummy parent
		NodePtr parent() { return NodePtr(); }
		ConstNodePtr parent() const { return ConstNodePtr(); }
		bool hasChildren() const { return (m_end - m_begin) > 1; }
		const StaticString & str() const { return m_begin->first; }
		const_iterator begin() const { return const_iterator(m_begin, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		const_iterator end() const { return const_iterator(m_end, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		iterator begin() { return iterator(m_begin, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		iterator end() { return iterator(m_end, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		TValue & value() {return m_begin->second;}
		const TValue & value() const {return m_begin->second;}
		///Apply functoid fn to all nodes in-order (sorted by keys)
		template<typename TFunc>
		void apply(TFunc & fn) const;
		///Apply functoid fn to all nodes in-order (sorted by keys)
		template<typename TFunc>
		void apply(TFunc & fn);
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
	struct HashFunc2 {
		const StringHandler * strHandler;
		HashFunc2(const StringHandler * strHandler = 0) : strHandler(strHandler) {}
		std::size_t operator()(const key_type & a) const {
			uint64_t seed = 0;
			typedef const char * const_iterator;
			for(const_iterator rit(strHandler->strEnd(a)-1), rend(strHandler->strBegin(a)-1); rit != rend; --rit) {
				hash_combine(seed, *rit);
			}
			return seed;
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
	HashBasedFlatTrie(sserialize::MmappedMemoryType stringsMMT = sserialize::MM_SHARED_MEMORY, sserialize::MmappedMemoryType hashMMT = sserialize::MM_SHARED_MEMORY) :
	m_stringData(stringsMMT),
	m_ht(HTValueStorage(sserialize::MM_PROGRAM_MEMORY), HTStorage(sserialize::MM_PROGRAM_MEMORY))
	{
		m_strHandler.specialString = 0;
		m_strHandler.strStorage = &m_stringData;
		const StringHandler * strHandlerPtr = &m_strHandler;
		m_ht = HashTable(HashFunc1(strHandlerPtr), HashFunc2(strHandlerPtr), StringEq(strHandlerPtr), 0.8, HTValueStorage(hashMMT), HTStorage(hashMMT));
	}
	~HashBasedFlatTrie() {}
	///reserve @param count strings
	void reserve(uint32_t count) { m_ht.reserve(count); }
	
	const HashTable & hashTable() const { return m_ht; }
	
	uint32_t size() const { return m_ht.size(); }
	bool count(const StaticString & str) const;
	bool count(const std::string & str);
	
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
	template<typename T_OCTET_ITERATOR>
	StaticString insert(T_OCTET_ITERATOR begin, const T_OCTET_ITERATOR & end);
	TValue & operator[](const StaticString & str);
	///You have to call finalize() before using this @param prefixMatch strIt->strEnd can be a prefix of the path
	template<typename T_OCTET_ITERATOR>
	NodePtr findNode(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch);
	
	///call this if you want to navigate the trie
	///you can always choose to add more strings to trie, but then you'd have to call this again
	void finalize();
	
	///before using this, finalize has to be called and no inserts were made afterwards
	NodePtr root() { return std::make_shared<Node>(m_ht.begin(), m_ht.end(), &m_strHandler); }
	ConstNodePtr root() const {
		HashBasedFlatTrie<TValue>* tmp = const_cast<HashBasedFlatTrie<TValue>*>(this);
		return std::make_shared<const Node>( tmp->m_ht.begin(), tmp->m_ht.end(), &m_strHandler);
	}
	
	///you can only call this after finalize()

	template<typename T_PH, typename T_STATIC_PAYLOAD = TValue>
	bool append(UByteArrayAdapter & dest, T_PH payloadHandler);
	
	static NodePtr make_nodeptr(Node & node) { return std::make_shared<Node>(node); }
	static ConstNodePtr make_nodeptr(const Node & node) { return std::make_shared<Node>(node); }
	
	template<typename T>
	bool checkTrieEquality(T /*t*/) const { return false; }
	
	template<typename TPayloadComparator, typename TNode>
	bool checkPayloadEquality(TNode /*node*/, TPayloadComparator /*pc*/) const { return false; }
};

template<typename TValue>
HashBasedFlatTrie<TValue>::Node::Iterator::Iterator(const HashBasedFlatTrie::iterator & parentBegin, const HashBasedFlatTrie::iterator & parentEnd, const HashBasedFlatTrie::CompFunc & compFunc) :
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
	if (m_childNodeBegin != m_childrenEnd) {
		uint32_t cp = utf8::peek_next( m_compFunc.strHandler->strBegin(m_childNodeBegin->first)+m_compFunc.posInStr, m_compFunc.strHandler->strEnd(m_childNodeBegin->first));
		m_childNodeEnd = std::upper_bound(m_childNodeBegin, m_childrenEnd, cp, m_compFunc);
	}
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
typename HashBasedFlatTrie<TValue>::NodePtr
HashBasedFlatTrie<TValue>::Node::Iterator::operator*() const {
	return std::make_shared<Node>(m_childNodeBegin, m_childNodeEnd, m_compFunc.strHandler);
}

template<typename TValue>
template<typename TFunc>
void
HashBasedFlatTrie<TValue>::Node::apply(TFunc & fn) const {
	fn(*this);
	for(const_iterator it(begin()), myEnd(end()); it != myEnd; ++it) {
		(*it)->apply(fn);
	}
}

template<typename TValue>
template<typename TFunc>
void
HashBasedFlatTrie<TValue>::Node::apply(TFunc & fn) {
	fn(*this);
	for(iterator it(begin()), myEnd(end()); it != myEnd; ++it) {
		(*it)->apply(fn);
	}
}

//end of internal classes implementations

template<typename TValue>
bool
HashBasedFlatTrie<TValue>::count(const std::string & a) {
	m_strHandler.specialString = a.c_str();
	bool ret = count(StaticString(a.size()));
	m_strHandler.specialString = 0;
	return ret;
}

template<typename TValue>
bool
HashBasedFlatTrie<TValue>::count(const StaticString & a) const {
	return m_ht.count(a);
}

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
template<typename T_OCTET_ITERATOR>
typename HashBasedFlatTrie<TValue>::NodePtr
HashBasedFlatTrie<TValue>::findNode(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) {
	if (prefixMatch) {
		throw sserialize::UnimplementedFunctionException("sserialize::HashBasedFlatTrie::findNode with prefixMatch==true");
	}
	std::string tmp(strIt, strEnd);
	m_strHandler.specialString = tmp.c_str();
	StaticString sstr(tmp.size());
	typename HashTable::iterator nodeBegin = m_ht.find(sstr);
	if (nodeBegin != m_ht.end()) {
		struct MyComp {
			inline bool operator()(uint32_t a, const std::pair<StaticString, TValue> & b) const {
				return a < b.first.size();
			}
			inline bool operator()(const std::pair<StaticString, TValue> & a, uint32_t b) const {
				return a.first.size() < b;
			}
		};
		typename HashTable::iterator nodeEnd = std::upper_bound(nodeBegin, m_ht.end(), nodeBegin->first.size(), MyComp());
		return std::make_shared<Node>(nodeBegin, nodeEnd, &m_strHandler);
	}
	return NodePtr();
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
template<typename T_OCTET_ITERATOR>
typename HashBasedFlatTrie<TValue>::StaticString
HashBasedFlatTrie<TValue>::insert(T_OCTET_ITERATOR begin, const T_OCTET_ITERATOR & end) {
	return insert(std::string(begin, end));
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
	m_ht.mt_sort(sortFunc);
	finalize(0, m_ht.size(), 0);
	m_ht.mt_sort(sortFunc);
}

template<typename TValue>
template<typename T_PH, typename T_STATIC_PAYLOAD>
bool HashBasedFlatTrie<TValue>::append(UByteArrayAdapter & dest, T_PH payloadHandler) {
	sserialize::ProgressInfo pinfo;
	sserialize::TimeMeasurer tm;

	dest.putUint8(1); //version of FlatTrieBase
	dest.putOffset(m_stringData.size());
	
	std::cout << "Copying string data..." << std::flush;
	tm.begin();
	dest.put(reinterpret_cast<const uint8_t*>(m_stringData.begin()), m_stringData.size());
	tm.end();
	std::cout << tm.elapsedSeconds() << " seconds" << std::endl;
	
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
	pinfo.begin(m_ht.size(), "sserialize::HashBasedFlatTrie serializing trie");
	for(const auto & x : m_ht) {
		tsCreator.set(count, 0, m_strHandler.strBegin(x.first)-strDataBegin);
		tsCreator.set(count, 1, x.first.size());
		++count;
		pinfo(count);
	}
	tsCreator.flush();
	pinfo.end();

	std::vector<NodePtr> nodesInLevelOrder;
	nodesInLevelOrder.reserve(size());
	{
		std::cout << "sserialize::HashBasedFlatTrie collecting trie nodes..." << std::flush;
		tm.begin();
		nodesInLevelOrder.push_back(root());
		uint32_t i = 0;
		while (i < nodesInLevelOrder.size()) {
			NodePtr & n = nodesInLevelOrder[i];
			for(NodePtr cn : *n) {
				nodesInLevelOrder.push_back(cn);
			}
			++i;
		}
		tm.end();
		std::cout << tm.elapsedSeconds() << " seconds" << std::endl;
	}
	//serialize the payload in bottom-up level-order (may be this should be done in parallel)

	sserialize::Static::DynamicVector<UByteArrayAdapter, UByteArrayAdapter> tmpPayload(nodesInLevelOrder.size(), nodesInLevelOrder.size());
	std::vector<uint32_t> nodeIdToData(nodesInLevelOrder.size(), std::numeric_limits<uint32_t>::max());
	count = 0;
	
	pinfo.begin(nodesInLevelOrder.size(), "sserialize::HashBasedFlatTrie serializing payload");
	while (nodesInLevelOrder.size()) {
		NodePtr & n = nodesInLevelOrder.back();
		uint32_t id = n->m_begin - m_ht.begin();
		nodeIdToData[id] = tmpPayload.size();
		tmpPayload.beginRawPush() << payloadHandler(n);
		tmpPayload.endRawPush();
		nodesInLevelOrder.pop_back();
		pinfo(++count);
	}
	pinfo.end();
	
	std::cout << "sserialize::HashBasedFlatTrie copying payload..." << std::flush;
	tm.begin();
	dest.putUint8(1);//version of FlatTrie
	Static::ArrayCreator<UByteArrayAdapter> vsCreator(dest);
	vsCreator.reserveOffsets(m_ht.size());
	for(uint32_t i(0), s(m_ht.size()); i < s; ++i) {
		vsCreator.put(tmpPayload.dataAt(nodeIdToData[i]));
	}
	vsCreator.flush();
	tm.end();
	std::cout << tm.elapsedSeconds() << " seconds" << std::endl;
	return true;
}


}//end namespace


template<typename TValue>
inline bool operator!=(const typename sserialize::HashBasedFlatTrie<TValue>::NodePtr & a, const typename sserialize::HashBasedFlatTrie<TValue>::NodePtr & b) { return *a != *b; }


#endif