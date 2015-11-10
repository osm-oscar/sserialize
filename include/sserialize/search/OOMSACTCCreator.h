#ifndef SSERIALIZE_OOM_SA_CTC_CREATOR_H
#define SSERIALIZE_OOM_SA_CTC_CREATOR_H
#include <sserialize/containers/MMVector.h>
#include <sserialize/containers/HashBasedFlatTrie.h>
#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/search/OOMCTCValueStore.h>
#include <sserialize/search/StringCompleter.h>

/**

struct Traits {
	typedef ItemType item_type;
	//all strings that should be searchable by exact/prefix search
	struct ExactStrings {
		template<typename TOutputIterator>
		void operator(item_type item, TOutputIterator out);
	};
	//all strings that should be searchable by suffix/substring search
	struct SuffixStrings {
		template<typename TOutputIterator>
		void operator(item_type item, TOutputIterator out);
	};
};

*/
namespace sserialize {
namespace detail {
namespace OOMSACTCCreator {

typedef sserialize::Static::UnicodeTrie::FlatTrieBase MyStaticTrie;

std::vector<uint32_t> calcParents(const sserialize::Static::UnicodeTrie::FlatTrieBase & ft) {
	struct Calc {
		std::vector<uint32_t> parents;
		void calc(const sserialize::Static::UnicodeTrie::FlatTrieBase::Node & node) {
			uint32_t myNodeId = node.id();
			for(auto x : node) {
				parents.at(x.id()) = myNodeId;
				calc(x);
			}
		}
	} c;
	c.parents.resize(ft.size(), 0xFFFFFFFF);
	c.calc(ft.root());
}

class MyStaticTrieInfo {
public:
	MyStaticTrieInfo(MyStaticTrie * t);
	~MyStaticTrieInfo() {}
	const MyStaticTrie * trie() const { return m_trie; }
	const std::vector<uint32_t> & parents() const { return m_parents; }
	
private:
	MyStaticTrie * m_trie;
	std::vector<uint32_t> m_parents;
};

class CTCValueStoreNode {
public:
	CTCValueStoreNode() : m_nodeId(std::numeric_limits<uint32_t>::max()), m_qt(sserialize::StringCompleter::QT_NONE) {}
	CTCValueStoreNode(uint32_t nodeId, sserialize::StringCompleter::QuerryType qt) : m_nodeId(nodeId), m_qt(qt) {}
	CTCValueStoreNode(const CTCValueStoreNode & other) : m_nodeId(other.nodeId()), m_qt(other.qt()) {}
	~CTCValueStoreNode() {}
	inline bool operator==(const CTCValueStoreNode & other) const { return m_nodeId == other.m_nodeId && m_qt == other.m_qt; }
	inline bool operator<(const CTCValueStoreNode & other) const { return (m_nodeId == other.m_nodeId ? m_qt < other.m_qt : m_nodeId < other.m_nodeId); }
	inline uint32_t nodeId() const { return m_nodeId; }
	inline uint8_t qt() const { return m_qt; }
private:
	uint32_t m_nodeId;
	uint8_t m_qt;
};


template<typename TOutPutIterator>
class ExactStringDerefIterator {
public:
	ExactStringDerefIterator() : m_t(0), m_d(0) {}
	ExactStringDerefIterator (MyStaticTrieInfo * t, TOutPutIterator & out) : m_t(t), m_out(out) {}
	ExactStringDerefIterator & operator*() { return *this; }
	ExactStringDerefIterator & operator++() {
		++m_out;
		return *this;
	}
	ExactStringDerefIterator & operator=(const std::string & str) {
		uint32_t p = m_t->trie()->find(str, false);
		assert(p != MyStaticTrie::npos);
		*m_out = p;
		return *this;
	}
private:
	MyStaticTrieInfo * m_t;
	TOutPutIterator m_out;
};

template<typename TOutPutIterator>
class SuffixStringDerefIterator {
public:
	SuffixStringDerefIterator() : m_t(0), m_d(0) {}
	SuffixStringDerefIterator (MyStaticTrieInfo * t, TOutPutIterator & out) : m_t(t), m_out(out) {}
	SuffixStringDerefIterator & operator*() { return *this; }
	SuffixStringDerefIterator & operator++() {
		++m_out;
		return *this;
	}
	SuffixStringDerefIterator & operator=(const std::string & str) {
		uint32_t p = m_t->trie()->find(str, false);
		assert(p != MyStaticTrie::npos);
		*m_out = p;
		if (!str.size()) {
			return *this;
		}
		std::string mstr;
		std::string::const_iterator it(str.cbegin()), end(str.cend());
		for(utf8::next(it, end); it != end; utf8::next(it, end)) {
			mstr.assign(it, end);
			uint32_t p = m_t->trie()->find(str, false);
			assert(p != MyStaticTrie::npos);
			++m_out;
			*m_out = p;
		}
		return *this;
	}
private:
	MyStaticTrieInfo * m_t;
	TOutPutIterator m_out;
};

template<typename TTraits>
class ItemDerefer {
public:
	typedef TTraits Traits;
	typedef typename Traits::item_type item_type;
	typedef typename Traits::ExactStringsDerefer ExactStringsDerefer;
	typedef typename Traits::SuffixStringsDerefer SuffixStringsDerefer;
private:
	typedef std::insert_iterator< std::unordered_set<uint32_t> > MyInsertIterator;
	typedef ExactStringDerefIterator<MyInsertIterator> ExactStringDerefIterator;
	typedef SuffixStringDerefIterator<MyInsertIterator> SuffixStringDerefIterator;
public:
	ItemDerefer(const Traits & traits, MyStaticTrieInfo * ti) : m_traits(traits), m_t(ti) {}
	//This is NOT! thread-safe
	template<typename TOutputIterator>
	void operator()(const item_type & item, TOutputIterator out);
private:
	Traits m_traits;
	MyStaticTrieInfo * m_t;
	ExactStringsDerefer m_ed;
	SuffixStringsDerefer m_sd;
	std::unordered_set<uint32_t> m_exactNodes;
	std::unordered_set<uint32_t> m_suffixNodes;
	ExactStringsDerefer m_edi;
	SuffixStringsDerefer m_sdi;
	std::unordered_set<CTCValueStoreNode> m_allNodes;
};

template<typename TTraits>
template<typename TOutputIterator>
void ItemDerefer<TTraits>::operator()(const item_type & item, TOutputIterator out) {
	m_ed(item, m_edi);
	m_sd(item, m_sdi);
	
	//now create the parents
	for(uint32_t x : m_exactNodes) {
		m_allNodes.emplace(x, sserialize::StringCompleter::QT_EXACT);
		uint32_t nextNode = x;
		while (nextNode != 0xFFFFFFFF) {
			m_allNodes.emplace(nextNode, sserialize::StringCompleter::QT_PREFIX);
			m_allNodes.emplace(nextNode, sserialize::StringCompleter::QT_SUBSTRING);
			nextNode = m_t->parents().at(nextNode);
		}
	}
	for(uint32_t x : m_suffixNodes) {
		m_allNodes.emplace(x, sserialize::StringCompleter::QT_SUFFIX);
		uint32_t nextNode = x;
		while (nextNode != 0xFFFFFFFF) {
			m_allNodes.emplace(nextNode, sserialize::StringCompleter::QT_SUBSTRING);
			nextNode = m_t->parents().at(nextNode);
		}
	}
	for(const auto & x : m_allNodes) {
		*out = x;
		++out;
	}
	m_exactNodes.clear();
	m_suffixNodes.clear();
	m_allNodes.clear();
}

template<typename TTraits>
class Creator {
public:
	typedef TTraits Traits;
public:
	Creator();
	void insertStrings();
	template<typename TInputIterator>
	void insertRegions(TInputIterator begin, TInputIterator end);
private:
	sserialize::HashBasedFlatTrie<uint32_t> m_t;
};

}}//end namespace detail::OOMSACTCCreator

template<typename TInputIterator, typename TTraits>
void appendSACTC(TInputIterator begin, TInputIterator end, sserialize::UByteArrayAdapter & dest,
TTraits traits = TTraits(), uint64_t maxMemoryUsage = static_cast<uint64_t>(1) << 32) {
	
	typedef TTraits Traits;
	typedef typename Traits::ExactStringsDerefer ExactStringsDerefer;
	typedef typename Traits::SuffixStringsDerefer SuffixStringsDerefer;
	
	sserialize::UByteArrayAdapter::OffsetType flatTrieBaseBegin = dest.tellPutPtr();
	{
		sserialize::HashBasedFlatTrie<uint32_t> myTrie;
		
		myTrie.finalize();
		myTrie.append(dest);
	}
	
	detail::OOMSACTCCreator::MyStaticTrie mst(sserialize::UByteArrayAdapter(dest, flatTrieBaseBegin));
	
	OOMCTCValuesCreator vc;
	vc.insert(begin, end);
	
}

}//end namespace

namespace std {

template<>
struct hash<sserialize::detail::OOMSACTCCreator::CTCValueStoreNode> {
	std::hash<uint64_t> hS;
	inline size_t operator()(const sserialize::detail::OOMSACTCCreator::CTCValueStoreNode & v) const {
		return (static_cast<uint64_t>(v.nodeId()) << 8) | v.qt();
	}
};

}//end namespace std

#endif