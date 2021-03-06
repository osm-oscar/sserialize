#pragma once
#ifndef SSERIALIZE_HASH_BASED_FLAT_TRIE_H
#define SSERIALIZE_HASH_BASED_FLAT_TRIE_H
#include <sserialize/storage/Size.h>
#include <sserialize/containers/OADHashTable.h>
#include <sserialize/containers/MMVector.h>
#include <sserialize/iterator/TransformIterator.h>
#include <sserialize/mt/GuardedVariable.h>
#include <sserialize/storage/MmappedMemory.h>
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/algorithm/hashspecializations.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/mt/ThreadPool.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/utility/debug.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/DynamicVector.h>
#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/utility/printers.h>

///WARNING: USING a hash of NodePtr need explicit initialization of the hash function (see bottom of this file)

namespace sserialize {

template<typename TValue>
class HashBasedFlatTrie;

namespace detail {
namespace HashBasedFlatTrie {

using StaticString = sserialize::Static::UnicodeTrie::detail::FlatTrie::StaticString;

}}//end namespace detail::HashBasedFlatTrie

template<typename TValue>
class HashBasedFlatTrie {
private:
	struct StringHandler;
public:
	typedef detail::HashBasedFlatTrie::StaticString StaticString;
	using SizeType = sserialize::Size;
	typedef TValue mapped_type;
	typedef StaticString key_type;
	typedef MMVector<char> StringStorage;
	typedef MMVector< std::pair<key_type, TValue> > HTValueStorage;
	typedef MMVector<SizeType> HTStorage;
	typedef typename HTValueStorage::const_iterator const_iterator;
	typedef typename HTValueStorage::iterator iterator;
private:
	using CodePoint = uint32_t;

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
		using CodePoint = uint32_t;
		const StringHandler * strHandler;
		StaticString::SizeType posInStr;
		CompFunc(const StringHandler * strHandler, StaticString::SizeType posInStr) : strHandler(strHandler), posInStr(posInStr) {}
		inline bool operator()(CodePoint a, const std::pair<StaticString, TValue> & b) const {
			return a < utf8::peek_next(strHandler->strBegin(b.first)+posInStr, strHandler->strEnd(b.first));
		}
		inline bool operator()(const std::pair<StaticString, TValue> & a, CodePoint b) const {
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
	struct NodePtrBase;
	class NodePtr {
	private:
		sserialize::RCPtrWrapper<NodePtrBase> m_priv;
	public:
		NodePtr();
		NodePtr(const HashBasedFlatTrie::iterator & begin, const HashBasedFlatTrie::iterator & end, const StringHandler * strHandler);
		NodePtr(const Node & node);
		Node& operator*();
		const Node& operator*() const;
		Node* operator->();
		const Node* operator->() const;
		bool operator==(const NodePtr & other) const;
		bool operator!=(const NodePtr & other) const;
		bool operator!() const { return !m_priv.get(); }
		explicit operator bool() const { return m_priv.get(); }
	};
	
	class Node final {
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
			bool operator!=(const Iterator & other) const;
			bool operator==(const Iterator & other) const;
			NodePtr operator*() const;
		};
		typedef Iterator const_iterator;
		typedef Iterator iterator;
		typedef HashBasedFlatTrie::iterator RawIterator;
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
		NodePtr parent() const { return NodePtr(); }
		bool hasChildren() const { return (m_end - m_begin) > 1; }
		const StaticString & str() const { return m_begin->first; }
		const_iterator begin() const { return const_iterator(m_begin, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		const_iterator cbegin() const { return const_iterator(m_begin, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		const_iterator end() const { return const_iterator(m_end, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		const_iterator cend() const { return const_iterator(m_end, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		iterator begin() { return iterator(m_begin, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		iterator end() { return iterator(m_end, m_end, HashBasedFlatTrie::CompFunc(m_strHandler, str().size())); }
		//raw access iterator, a node is defined as a interval in an sorted array. these iterators return the begin/end to this interval
		RawIterator & rawBegin() { return m_begin; }
		const RawIterator & rawBegin() const { return m_begin; }
		RawIterator & rawEnd() { return m_end; }
		const RawIterator & rawEnd() const { return m_end; }
		
		inline bool operator==(const Node & other) const { return m_begin == other.m_begin && m_end == other.m_end && m_strHandler == other.m_strHandler; }
		inline bool operator!=(const Node & other) const { return m_begin != other.m_begin || m_end != other.m_end || m_strHandler != other.m_strHandler; }
		
		TValue & value() {return m_begin->second;}
		const TValue & value() const {return m_begin->second;}
		///Apply functoid fn to all nodes in-order (sorted by keys)
		template<typename TFunc>
		void apply(TFunc & fn) const;
		///Apply functoid fn to all nodes in-order (sorted by keys)
		template<typename TFunc>
		void apply(TFunc & fn);
	};
	
	struct NodePtrBase: public sserialize::RefCountObject {
		Node node;
		NodePtrBase(const Node & node) : node(node) {}
		NodePtrBase(const HashBasedFlatTrie::iterator & begin, const HashBasedFlatTrie::iterator & end, const StringHandler * strHandler) :
		node(begin, end, strHandler)
		{}
	};
	
	struct NodePtrHash {
		std::hash<typename Node::RawIterator> hasher;
		inline std::size_t operator()(const NodePtr & v) const {
			std::size_t seed = 0;
			::hash_combine(seed, v->rawBegin(), hasher);
			::hash_combine(seed, v->rawEnd(), hasher);
			return seed;
		}
	};
private:
	struct HashFunc1 {
		const StringHandler * strHandler;
		HashFunc1(const StringHandler * strHandler = 0) : strHandler(strHandler) {}
		std::size_t operator()(const key_type & a) const {
			std::size_t seed = 0;
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
			std::size_t seed = 0;
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
	mutable StringHandler m_strHandler;
	mutable std::mutex m_specStrLock;
	HashTable m_ht;
private:
	void finalize(uint64_t nodeBegin, uint64_t nodeEnd, StaticString::SizeType posInStr);
	uint32_t depth(const NodePtr & n) {
		uint32_t mD = 0;
		for(auto c : *n) {
			mD = std::max<uint32_t>(mD, depth(c));
		}
		return mD + 1;
	}
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
	HashBasedFlatTrie(HashBasedFlatTrie const&) = delete;
	~HashBasedFlatTrie() {}
	HashBasedFlatTrie & operator=(HashBasedFlatTrie const&) = delete;
	HashBasedFlatTrie & operator=(HashBasedFlatTrie && o) {
		if (&o == this) {
			return *this;
		}
		std::lock_guard<std::mutex> lck1(m_specStrLock);
		std::lock_guard<std::mutex> lck2(o.m_specStrLock);
		m_stringData = std::move(o.m_stringData);
		m_strHandler.specialString = 0;
		m_strHandler.strStorage = &m_stringData;
		m_ht = std::move(o.m_ht);
		const StringHandler * strHandlerPtr = &m_strHandler;
		m_ht.hash1() = HashFunc1(strHandlerPtr);
		m_ht.hash2() = HashFunc2(strHandlerPtr);
		m_ht.keyEq() = StringEq(strHandlerPtr);
		return *this;
	}
	
	UByteArrayAdapter::SizeType minStorageSize() const { return m_stringData.size() + m_ht.storageCapacity()*sizeof(typename HTValueStorage::value_type) + m_ht.capacity()*sizeof(HTStorage::value_type);}
	///reserve @param count strings
	void reserve(SizeType count) { m_ht.reserve(count); }
	
	const HashTable & hashTable() const { return m_ht; }
	
	SizeType size() const { return m_ht.size(); }
	bool count(const StaticString & str) const;
	bool count(const std::string & str);
	
	iterator begin() { return m_ht.begin(); }
	const_iterator begin() const { return m_ht.cbegin(); }
	const_iterator cbegin() const { return m_ht.cbegin(); }
	
	iterator end() { return m_ht.end(); }
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
	TValue & at(std::string const & str);
	TValue const & at(std::string const & str) const;
	TValue & operator[](const StaticString & str);
	///You have to call finalize() before using this @param prefixMatch strIt->strEnd can be a prefix of the path
	template<typename T_OCTET_ITERATOR>
	NodePtr findNode(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch);
	
	///call this if you want to navigate the trie
	///you can always choose to add more strings to trie, but then you'd have to call this again
	void finalize(std::size_t threadCount = 0);
	
	///before using this, finalize has to be called and no inserts were made afterwards
	NodePtr root() { return NodePtr(m_ht.begin(), m_ht.end(), &m_strHandler); }
	NodePtr root() const {
		HashBasedFlatTrie<TValue>* tmp = const_cast<HashBasedFlatTrie<TValue>*>(this);
		return NodePtr( tmp->m_ht.begin(), tmp->m_ht.end(), &m_strHandler);
	}
	
	///append just the trie, no payload
	bool append(UByteArrayAdapter & dest);
	
	///you can only call this after finalize(), calls payloadHandler in in-order
	template<typename T_PH, typename T_STATIC_PAYLOAD = typename std::result_of<T_PH(NodePtr)>::type>
	bool append(UByteArrayAdapter & dest, T_PH payloadHandler, std::size_t threadCount = 1);
	
	static NodePtr make_nodeptr(Node & node) { return NodePtr(node); }
	static NodePtr make_nodeptr(const Node & node) { return NodePtr(node); }

	bool valid(SizeType & offendingString) const;
	
	bool checkTrieEquality(const sserialize::Static::UnicodeTrie::FlatTrieBase & sft) const;
	
	///not implemented yet
	template<typename TPayloadComparator, typename TNode>
	bool checkPayloadEquality(TNode /*node*/, TPayloadComparator /*pc*/) const { return false; }
};

template<typename TValue>
HashBasedFlatTrie<TValue>::NodePtr::NodePtr() {}

template<typename TValue>
HashBasedFlatTrie<TValue>::NodePtr::NodePtr(const typename HashBasedFlatTrie<TValue>::iterator & begin,
											const typename HashBasedFlatTrie<TValue>::iterator & end,
											const typename HashBasedFlatTrie<TValue>::StringHandler* strHandler) :
m_priv(new HashBasedFlatTrie<TValue>::NodePtrBase(begin, end, strHandler))
{}

template<typename TValue>
HashBasedFlatTrie<TValue>::NodePtr::NodePtr(const typename HashBasedFlatTrie<TValue>::Node & node) :
m_priv(new HashBasedFlatTrie<TValue>::NodePtrBase(node))
{}

template<typename TValue>
typename HashBasedFlatTrie<TValue>::Node &
HashBasedFlatTrie<TValue>::NodePtr::operator*() {
	return m_priv->node;
}

template<typename TValue>
const typename HashBasedFlatTrie<TValue>::Node &
HashBasedFlatTrie<TValue>::NodePtr::operator*() const {
	return m_priv->node;
}

template<typename TValue>
const typename HashBasedFlatTrie<TValue>::Node*
HashBasedFlatTrie<TValue>::NodePtr::operator->() const {
	return &(m_priv->node);
}

template<typename TValue>
typename HashBasedFlatTrie<TValue>::Node*
HashBasedFlatTrie<TValue>::NodePtr::operator->() {
	return &(m_priv->node);
}

template<typename TValue>
bool HashBasedFlatTrie<TValue>::NodePtr::operator==(const HashBasedFlatTrie<TValue>::NodePtr & other) const {
	if (!(bool)m_priv.get() && !(bool)other.m_priv.get()) //both are null
		return true;
	else if (!(bool)m_priv.get() xor !(bool)m_priv.get()) //one is null
		return false;
	else
		return m_priv->node == other.m_priv->node;
}

template<typename TValue>
bool HashBasedFlatTrie<TValue>::NodePtr::operator!=(const HashBasedFlatTrie<TValue>::NodePtr & other) const {
	if (!(bool)m_priv.get() && !(bool)other.m_priv.get()) //both are null
		return false;
	else if (!(bool)m_priv.get() xor !(bool)m_priv.get()) //one is null
		return true;
	else
		return m_priv->node != other.m_priv->node;
}

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
		CodePoint cp = utf8::peek_next( m_compFunc.strHandler->strBegin(m_childNodeBegin->first)+m_compFunc.posInStr, m_compFunc.strHandler->strEnd(m_childNodeBegin->first));
		m_childNodeEnd = std::upper_bound(m_childNodeBegin, m_childrenEnd, cp, m_compFunc);
	}
	return *this;
}

template<typename TValue>
bool
HashBasedFlatTrie<TValue>::Node::Iterator::operator!=(const Iterator & other) const {
	return m_childNodeBegin != other.m_childNodeBegin ||
			m_childNodeEnd != other.m_childNodeEnd ||
			m_childrenEnd != other.m_childrenEnd ||
			m_compFunc != other.m_compFunc;
}

template<typename TValue>
bool
HashBasedFlatTrie<TValue>::Node::Iterator::operator==(const Iterator & other) const {
	return m_childNodeBegin == other.m_childNodeBegin ||
			m_childNodeEnd == other.m_childNodeEnd ||
			m_childrenEnd == other.m_childrenEnd ||
			m_compFunc == other.m_compFunc;
}

template<typename TValue>
typename HashBasedFlatTrie<TValue>::NodePtr
HashBasedFlatTrie<TValue>::Node::Iterator::operator*() const {
	return NodePtr(m_childNodeBegin, m_childNodeEnd, m_compFunc.strHandler);
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
	std::lock_guard<std::mutex> lck(m_specStrLock);
	m_strHandler.specialString = a.c_str();
	bool ret = count(StaticString::make_special(a.size()));
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
	if (a.size() > StaticString::MaxStringSize) {
		throw sserialize::OutOfBoundsException("HashBasedFlatTrie::insert: string is too long");
	}
	std::lock_guard<std::mutex> lck(m_specStrLock);
	m_strHandler.specialString = a.c_str();
	StaticString ret = insert(StaticString::make_special(a.size()));
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
		typename StaticString::OffsetType strOff;
		narrow_check_assign(strOff) = m_stringData.size();
		m_stringData.push_back(m_strHandler.strBegin(a), m_strHandler.strEnd(a));
		return m_ht[StaticString(strOff, a.size())];
	}
}

template<typename TValue>
TValue &
HashBasedFlatTrie<TValue>::at(std::string const & str) {
	std::lock_guard<std::mutex> lck(m_specStrLock);
	m_strHandler.specialString = str.c_str();
	auto sstr = StaticString::make_special(str.size());
	try {
		TValue & v = m_ht.at(sstr);
		m_strHandler.specialString = 0;
		return v;
	}
	catch (std::out_of_range const & e) {
		m_strHandler.specialString = 0;
		throw e;
	}
}

template<typename TValue>
TValue const &
HashBasedFlatTrie<TValue>::at(std::string const & str) const {
	std::lock_guard<std::mutex> lck(m_specStrLock);
	m_strHandler.specialString = str.c_str();
	auto sstr = StaticString::make_special(str.size());
	try {
		TValue const & v = m_ht.at(sstr);
		m_strHandler.specialString = 0;
		return v;
	}
	catch (std::out_of_range const & e) {
		m_strHandler.specialString = 0;
		throw e;
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
	typename HashTable::iterator nodeBegin;
	{
		std::lock_guard<std::mutex> lck(m_specStrLock);
		m_strHandler.specialString = tmp.c_str();
		auto sstr = StaticString::make_special(tmp.size());
		nodeBegin = m_ht.find(sstr);
		m_strHandler.specialString = 0;
	}
	
	if (nodeBegin != m_ht.end()) {
		struct MyComp {
			inline bool operator()(CodePoint a, const std::pair<StaticString, TValue> & b) const {
				return a < b.first.size();
			}
			inline bool operator()(const std::pair<StaticString, TValue> & a, CodePoint b) const {
				return a.first.size() < b;
			}
		};
		typename HashTable::iterator nodeEnd = std::upper_bound(nodeBegin, m_ht.end(), nodeBegin->first.size(), MyComp());
		return NodePtr(nodeBegin, nodeEnd, &m_strHandler);
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
		typename StaticString::OffsetType strOff;
		narrow_check_assign(strOff) = m_stringData.size();
		m_stringData.push_back(m_strHandler.strBegin(a), m_strHandler.strEnd(a));
		SSERIALIZE_NORMAL_ASSERT(utf8::is_valid(m_strHandler.strBegin(a), m_strHandler.strEnd(a)));
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
void HashBasedFlatTrie<TValue>::finalize(uint64_t nodeBeginOff, uint64_t nodeEndOff, StaticString::SizeType posInStr) {
	if (nodeBeginOff != nodeEndOff) {
		{//find the end of our current node
			const_iterator nodeBegin = begin()+nodeBeginOff;
			const_iterator nodeEnd = begin()+nodeEndOff;
			const char * nodeStrEnd = m_strHandler.strEnd(nodeBegin->first);
			const char * beginLookUp = m_strHandler.strBegin(nodeBegin->first) + posInStr;
			const char * endLookUp = beginLookUp;
			while (posInStr < nodeBegin->first.size()) {
				CodePoint cp = utf8::next(endLookUp, nodeStrEnd);
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
			CodePoint cp = utf8::next(childNextCP, m_strHandler.strEnd(nodeBegin->first));
			const_iterator endChildNode = std::upper_bound(nodeBegin, nodeEnd, cp, compFunc);
			uint64_t childEndOff = endChildNode-begin();
			finalize(nodeBeginOff, childEndOff, (StaticString::SizeType) (childNextCP - m_strHandler.strBegin(nodeBegin->first)));
			nodeBeginOff = childEndOff;
		}
	}
}

template<typename TValue>
bool HashBasedFlatTrie<TValue>::valid(SizeType & offendingString) const {
	SizeType counter = 0;
	for(const auto & x : *this) {
		if (!utf8::is_valid(m_strHandler.strBegin(x.first), m_strHandler.strEnd(x.first))) {
			offendingString = counter;
			return false;
		}
		++counter;
	}
	return true;
}

template<typename TValue>
void HashBasedFlatTrie<TValue>::finalize(std::size_t threadCount) {
	const StringHandler * strHandler = &m_strHandler;
	#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
	std::cout << "Finalizing HashBasedFlatTrie with size=" << size() << std::endl;
	SizeType brokenString = 0;
	if (!valid(brokenString)) {
		throw sserialize::CorruptDataException("String is not valid at position=" + sserialize::toString<SizeType>(brokenString));
	}
	#endif
	auto sortFunc = [strHandler](const typename HashTable::value_type & a, const typename HashTable::value_type & b) {
		return sserialize::unicodeIsSmaller(strHandler->strBegin(a.first), strHandler->strEnd(a.first), strHandler->strBegin(b.first), strHandler->strEnd(b.first));
	};
	m_ht.mt_sort(sortFunc, threadCount);
	finalize(0, m_ht.size(), 0);
	m_ht.mt_sort(sortFunc, threadCount);
}

template<typename TValue>
bool HashBasedFlatTrie<TValue>::checkTrieEquality(const Static::UnicodeTrie::FlatTrieBase& sft) const {
	if (m_ht.size() != sft.size()) {
		return false;
	}
	
	if (sft.strData().size() != m_stringData.size()) {
		return false;
	}
	UByteArrayAdapter sftStrData = sft.strData();
	for(UByteArrayAdapter::OffsetType i(0), s(m_stringData.size()); i < s; ++i) {
		if (sftStrData.at(i) != (uint8_t) m_stringData.at(i)) {
			return false;
		}
	}
	const_iterator rIt(m_ht.cbegin()), rEnd(m_ht.cend());
	SizeType sI(0), sS(sft.size());
	for(; sI < sS && rIt != rEnd; ++sI, ++rIt) {
		auto sftX = sft.sstr(sI);
		if (rIt->first.size() != sftX.size() || rIt->first.offset() != sftX.offset()) {
			return false;
		}
	}

	rIt = m_ht.cbegin();
	sI = 0;
	for(; sI < sS && rIt != rEnd; ++sI, ++rIt) {
		if (toStr(rIt->first) != sft.strAt(sI)) {
			return false;
		}
	}
	return true;
}

template<typename TValue>
bool HashBasedFlatTrie<TValue>::append(UByteArrayAdapter & dest) {
	sserialize::ProgressInfo pinfo;
	sserialize::TimeMeasurer tm;

#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
	UByteArrayAdapter::OffsetType flatTrieBaseBeginOffset = dest.tellPutPtr();
#endif
	dest.putUint8(1); //version of FlatTrieBase
	dest.putOffset(m_stringData.size());
	
	std::cout << "Copying string data(" << sserialize::prettyFormatSize(m_stringData.size()) << ")..." << std::flush;
	tm.begin();
	dest.putData(reinterpret_cast<const uint8_t*>(m_stringData.begin()), m_stringData.size());
	tm.end();
	std::cout << tm.elapsedSeconds() << " seconds" << std::endl;
	
	const char * maxBegin = m_stringData.begin();
	uint64_t maxLen = 0;
	for(const auto & x : m_ht) {
		maxBegin = std::max<const char*>(maxBegin, m_strHandler.strBegin(x.first));
		maxLen = std::max<uint64_t>(maxLen, x.first.size());
	}
	uint64_t maxOffset = maxBegin - m_stringData.begin();
	if (maxOffset > std::numeric_limits<SizeType>::max() || maxLen > std::numeric_limits<SizeType>::max()) {
		throw sserialize::OutOfBoundsException("String data is too large");
	}
	
	MultiVarBitArrayCreator tsCreator(std::vector<uint8_t>({(uint8_t)CompactUintArray::minStorageBits(maxOffset), (uint8_t)CompactUintArray::minStorageBits(maxLen)}), dest);
	SizeType count = 0;
	const char * strDataBegin = m_stringData.begin();
	pinfo.begin(m_ht.size(), "sserialize::HashBasedFlatTrie serializing trie");
#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
	uint64_t debugCheckOffsetEntry = m_strHandler.strBegin(m_ht.begin()->first)-strDataBegin;
	uint64_t debugCheckSizeEntry = m_ht.begin()->first.size();
#endif
	for(const auto & x : m_ht) {
		tsCreator.set(count, 0, SizeType(m_strHandler.strBegin(x.first)-strDataBegin));
		tsCreator.set(count, 1, x.first.size());
		++count;
		pinfo(count);
#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(tsCreator.at(0,0), debugCheckOffsetEntry);
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(tsCreator.at(0,1), debugCheckSizeEntry);
#endif
	}
	tsCreator.flush();
	pinfo.end();
#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
	{
		UByteArrayAdapter tmp(dest);
		tmp.setPutPtr(flatTrieBaseBeginOffset);
		tmp.shrinkToPutPtr();
		sserialize::Static::UnicodeTrie::FlatTrieBase ftb(tmp);
		SSERIALIZE_EXPENSIVE_ASSERT(checkTrieEquality(ftb));
		std::cout << "Tries are equal" << std::endl;
	}
#endif
	return true;
}

template<typename TValue>
template<typename T_PH, typename T_STATIC_PAYLOAD>
bool HashBasedFlatTrie<TValue>::append(UByteArrayAdapter & dest, T_PH payloadHandler, std::size_t threadCount) {
	if (size() > std::numeric_limits<SizeType>::max()) {
		throw sserialize::CreationException("HashBasedFlatTrie: unable to serialize. Too many nodes.");
	}

	if (!append(dest)) {
		return false;
	}
	sserialize::ProgressInfo pinfo;
	sserialize::TimeMeasurer tm;
	sserialize::Static::DynamicVector<UByteArrayAdapter, UByteArrayAdapter> tmpPayload(size(), size());
	std::vector<SizeType> nodeIdToData(size(), std::numeric_limits<SizeType>::max());
	SizeType count = 0;
	if (threadCount <= 1) {
		std::vector<NodePtr> nodesInLevelOrder;
		nodesInLevelOrder.reserve(size());
		{
			std::cout << "sserialize::HashBasedFlatTrie collecting trie nodes..." << std::flush;
			tm.begin();
			nodesInLevelOrder.push_back(root());
			SizeType i = 0;
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

		pinfo.begin(nodesInLevelOrder.size(), "sserialize::HashBasedFlatTrie serializing payload");
		while (nodesInLevelOrder.size()) {
			NodePtr & n = nodesInLevelOrder.back();
			SizeType id = SizeType(n->m_begin - m_ht.begin());
			nodeIdToData[id] = tmpPayload.size();
			tmpPayload.beginRawPush() << payloadHandler(n);
			tmpPayload.endRawPush();
			nodesInLevelOrder.pop_back();
			pinfo(++count);
		}
		pinfo.end();
	}
	else {
// 		threadCount = 1;
		typename HashTable::const_iterator htBegin = m_ht.begin();
		SSERIALIZE_CHEAP_ASSERT_EXEC(SizeType htSize = size());
		std::vector< std::vector<NodePtr> > nodesInLevelOrder(depth(root()));
		nodesInLevelOrder[0].push_back(root());
		for(SizeType i(0), s(SizeType(nodesInLevelOrder.size()-1)); i < s; ++i) {
			const std::vector<NodePtr> & levelNodes = nodesInLevelOrder[i];
			std::vector<NodePtr> & destLevelNodes = nodesInLevelOrder[i+1];
			for(const NodePtr & n : levelNodes) {
				for(typename Node::const_iterator it(n->cbegin()), end(n->cend()); it != end; ++it) {
					destLevelNodes.push_back(*it);
					#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
					int64_t id = destLevelNodes.back()->rawBegin() - htBegin;
					#endif
					SSERIALIZE_CHEAP_ASSERT(id >= 0 && uint64_t(id) < htSize);
					SSERIALIZE_CHEAP_ASSERT(id >= 0 && uint64_t(id) < size());
				}
			}
		}
		SSERIALIZE_CHEAP_ASSERT(nodesInLevelOrder.back().size());
		sserialize::ThreadPool threadPool(threadCount);
		std::vector<T_PH> payloadHandlers(threadPool.numThreads(), payloadHandler);
		GuardedVariable<int32_t> runningBlockTasks(0);
		std::mutex dataAccessMtx;
		std::mutex nodeFetchMtx;
		pinfo.begin(nodesInLevelOrder.size(), "sserialize::HashBasedFlatTrie serializing payload");
		while(nodesInLevelOrder.size()) {
			SSERIALIZE_CHEAP_ASSERT(runningBlockTasks.unsyncedValue() == 0);
			std::vector<NodePtr> & levelNodes = nodesInLevelOrder.back();
			NodePtr * levelNodesIt = &levelNodes[0];
			NodePtr * levelNodesEnd = levelNodesIt+levelNodes.size();
			std::size_t blockSize = levelNodes.size()/threadCount;
			
			for(std::size_t i(0); i < threadCount; ++i) {
				T_PH * pH = &payloadHandlers[i];
				runningBlockTasks.syncedWithoutNotify([](int32_t & v) { v += 1; });
				threadPool.sheduleTask(
					[=, &levelNodesIt, &levelNodesEnd, &nodeFetchMtx, &runningBlockTasks, &dataAccessMtx, &nodeIdToData, &tmpPayload, &htBegin]() {
						Static::DynamicVector<UByteArrayAdapter, UByteArrayAdapter> myTmpPayload(blockSize, blockSize*16, sserialize::MM_PROGRAM_MEMORY);
						std::vector<SizeType> nodeIds;
						nodeIds.reserve(blockSize);
						while(true) {
							nodeFetchMtx.lock();
							if (levelNodesIt != levelNodesEnd) {
								NodePtr & n = *levelNodesIt;
								++levelNodesIt;
								nodeFetchMtx.unlock();
								SizeType id = SizeType(n->rawBegin() - htBegin);
								SSERIALIZE_CHEAP_ASSERT_SMALLER(id, htSize);
								
								myTmpPayload.beginRawPush() << (*pH)(n);
								myTmpPayload.endRawPush();
								nodeIds.push_back(id);
								SSERIALIZE_CHEAP_ASSERT_EQUAL(nodeIds.size(), myTmpPayload.size());
							}
							else {
								nodeFetchMtx.unlock();
								break;
							}
						}
						{//do the real push
							std::lock_guard<std::mutex> dALck(dataAccessMtx);
							for(std::size_t i(0), s(SizeType(nodeIds.size())); i < s; ++i) {
								nodeIdToData.at(nodeIds.at(i)) = tmpPayload.size();
								tmpPayload.beginRawPush().putData(myTmpPayload.dataAt(i));
								tmpPayload.endRawPush();
							}
						}
						runningBlockTasks.syncedWithNotifyOne([](int32_t & v) { v -= 1; });
					}
				);
			}
			SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(runningBlockTasks.unsyncedValue(), (int32_t)(threadCount+1));
			{
				GuardedVariable<int32_t>::UniqueLock lck(runningBlockTasks.uniqueLock());
				while (runningBlockTasks.unsyncedValue() > 0) {
					runningBlockTasks.wait_for(lck, 1000000);//wait for 1 second
				}
				SSERIALIZE_CHEAP_ASSERT_EQUAL(runningBlockTasks.unsyncedValue(), 0);
			}
			//wait for jobs to deallocate memory
			nodesInLevelOrder.pop_back();
			pinfo(pinfo.targetCount-nodesInLevelOrder.size());
		}
		pinfo.end();
	}
	
	std::cout << "sserialize::HashBasedFlatTrie copying payload..." << std::flush;
	tm.begin();
	dest.putUint8(1);//version of FlatTrie
	Static::ArrayCreator<T_STATIC_PAYLOAD> vsCreator(dest);
	vsCreator.reserveOffsets(m_ht.size());
	for(SizeType i(0), s(m_ht.size()); i < s; ++i) {
		vsCreator.beginRawPut();
		vsCreator.rawPut() << tmpPayload.dataAt(nodeIdToData[i]);
		vsCreator.endRawPut();
	}
	vsCreator.flush();
	tm.end();
	std::cout << tm.elapsedSeconds() << " seconds" << std::endl;
	return true;
}

}//end namespace


#endif
