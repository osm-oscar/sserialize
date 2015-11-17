#ifndef SSERIALIZE_OOM_SA_CTC_CREATOR_H
#define SSERIALIZE_OOM_SA_CTC_CREATOR_H
#include <sserialize/containers/MMVector.h>
#include <sserialize/containers/HashBasedFlatTrie.h>
#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/search/OOMCTCValueStore.h>
#include <sserialize/search/StringCompleter.h>
#include <sserialize/containers/ItemIndexFactory.h>

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


class OOM_CTC_VS_BaseTraits {
public:
	typedef CTCValueStoreNode NodeIdentifier;
	struct NodeEqualPredicate {
		bool operator()(const NodeIdentifier & a, const NodeIdentifier & b) {
			return a == b;
		}
	};
	struct NodeIdentifierLessThanComparator {
		bool operator()(const NodeIdentifier & a, const NodeIdentifier & b) {
			return a < b;
		}
	};
public:
	NodeIdentifier nodeIdentifier() { return NodeIdentifier(); }
	NodeEqualPredicate nodeEqualPredicate() { return NodeEqualPredicate(); }
	NodeIdentifierLessThanComparator nodeIdentifierLessThanComparator() { return NodeIdentifierLessThanComparator(); }
};

template<typename TOOMSABaseTraits>
class OOM_CTC_VS_InputTraits: public TOOMSABaseTraits {
public:
	typedef TOOMSABaseTraits MyBaseTraits;
	typedef typename MyBaseTraits::item_type item_type;
	typedef sserialize::detail::OOMSACTCCreator::MyStaticTrieInfo MyStaticTrieInfo;
	
	class FullMatchPredicate {
	private:
		bool m_fullMatch;
	public:
		FullMatchPredicate(bool fullMatch) : m_fullMatch(fullMatch) {}
		bool operator()(const item_type & item) { return m_fullMatch; }
	};
	
	class ItemTextSearchNodes {
	private:
		const MyStaticTrieInfo * m_ti;
		MyBaseTraits::ExactStrings m_es;
		MyBaseTraits::SuffixStrings m_ss;
	public:
		ItemTextSearchNodes(const MyStaticTrieInfo * ti, const ExactStrings & es, const SuffixStrings & ss) :
		m_ti(ti), m_es(es), m_ss(ss) {}
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) {
			
		}
	};
private:
	MyStaticTrieInfo * m_ti;
	bool m_fullMatch;
public:
	OOM_CTC_VS_InputTraits(const MyBaseTraits & baseTraits, MyStaticTrieInfo * ti, bool allAreFullMatch) :
	MyBaseTraits(baseTraits), m_ti(ti), m_fullMatch(allAreFullMatch) {}

	FullMatchPredicate fullMatchPredicate() { return FullMatchPredicate(m_fullMatch); }
	ItemTextSearchNodes itemTextSearchNodes() { return ItemTextSearchNodes(m_ti, MyBaseTraits::exactStrings(), MyBaseTraits::suffixStrings()); }
};

class OOM_CTC_VS_OutputTraits {
public:
	typedef OOM_CTC_VS_BaseTraits::NodeIdentifier NodeIdentifier;
	class IndexFactoryOut {
		sserialize::ItemIndexFactory * m_idxFactory;
	public:
		IndexFactoryOut(sserialize::ItemIndexFactory * idxFactory) : m_idxFactory(idxFactory) {}
		template<typename TIterator>
		uint32_t operator()(const TIterator & begin, const TIterator & end) {
			std::vector<uint32_t> tmp(begin, end);//BUG:this sucks
			return m_idxFactory->addIndex(tmp);
		}
	};
	class DataOut {
	public:
		typedef sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> PayloadCreator;
	private:
		PayloadCreator * m_payloadCreator;
		uint32_t m_curNodeId;
		uint32_t m_curTypes;
		sserialize::UByteArrayAdapter m_curData;
		std::vector<uint32_t> m_curOffsets;
	public:
		DataOut(PayloadCreator * pc) : m_curNodeId(0), m_curTypes(sserialize::StringCompleter::QT_NONE), m_payloadCreator(pc) {}
		void operator()(const NodeIdentifier & ni, const sserialize::UByteArrayAdapter & data) {
			assert(m_curNodeId <= ni.nodeId());
			if (m_curNodeId != ni.nodeId()) {//flush
				//make sure that we push at the right position
				while(m_curNodeId+1 < m_payloadCreator->size()) {
					m_payloadCreator->beginRawPut();
					m_payloadCreator->endRawPut();
				}
				m_payloadCreator->beginRawPut();
				m_payloadCreator->rawPut().put(m_curData);
				m_payloadCreator->endRawPut();
				//reset temp data
				m_curNodeId = ni.nodeId();
				m_curTypes = sserialize::StringCompleter::QT_NONE;
				m_curData = sserialize::UByteArrayAdapter(m_curData, 0, 0);
				m_curOffsets.clear();
			}
			assert(m_curTypes < ni.qt());
			m_curTypes |= ni.qt();
			m_curOffsets += m_curData.tellPutPtr();
			m_curData.put(data);
		}
	};
private:
	sserialize::ItemIndexFactory * m_idxFactory;
	sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> * m_payloadCreator;
public:
	OOM_CTC_VS_OutputTraits(sserialize::ItemIndexFactory * idxFactory, sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> * payloadCreator) :
	m_idxFactory(idxFactory), m_payloadCreator(payloadCreator)
	{}
	
	inline IndexFactoryOut indexFactoryOut() { return IndexFactoryOut(m_idxFactory); }
	inline DataOut dataOut() { return DataOut(m_payloadCreator); }
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

}}//end namespace detail::OOMSACTCCreator

template<typename TItemIterator, typename TRegionIterator, typename TItemTraits, typename TRegionTraits>
void appendSACTC(TItemIterator itemsBegin, TItemIterator itemsEnd, TRegionIterator regionsBegin, TRegionIterator regionsEnd,
					TItemTraits itemTraits, TRegionTraits regionTraits,
					uint64_t maxMemoryUsage,
					sserialize::ItemIndexFactory & idxFactory, sserialize::UByteArrayAdapter & dest)
{
	typedef TItemTraits ItemTraits;
	typedef TRegionTraits RegionTraits;
	typedef detail::OOMSACTCCreator::OOM_CTC_VS_InputTraits<ItemTraits> ItemInputTraits;
	typedef detail::OOMSACTCCreator::OOM_CTC_VS_InputTraits<RegionTraits> RegionInputTraits;

	typedef typename ItemTraits::ExactStrings ItemExactStrings;
	typedef typename ItemTraits::SuffixStrings ItemSuffixStrings;
	typedef typename RegionTraits::ExactStrings RegionExactStrings;
	typedef typename RegionTraits::SuffixStrings RegionSuffixStrings;
	
	typedef detail::OOMSACTCCreator::OOM_CTC_VS_OutputTraits OutputTraits;
	
	sserialize::UByteArrayAdapter::OffsetType flatTrieBaseBegin = dest.tellPutPtr();
	{
		sserialize::HashBasedFlatTrie<uint32_t> myTrie;
		
		struct ExactStringsInserter {
			sserialize::HashBasedFlatTrie<uint32_t> * m_t;
			ExactStringsInserter(sserialize::HashBasedFlatTrie<uint32_t> * t) : m_t(t) {}
			ExactStringsInserter & operator*() { return *this;}
			ExactStringsInserter & operator=(const std::string & str) {
				m_t->insert(str);
			}
			ExactStringsInserter & operator++() { return *this; }
		};
		
		struct SuffixStringsInserter {
			sserialize::HashBasedFlatTrie<uint32_t> * m_t;
			SuffixStringsInserter(sserialize::HashBasedFlatTrie<uint32_t> * t) : m_t(t) {}
			SuffixStringsInserter & operator*() { return *this;}
			SuffixStringsInserter & operator=(const std::string & str) {
				auto sstr = m_t->insert(str);
				
			}
			SuffixStringsInserter & operator++() { return *this; }
		};

		ExactStringsInserter esi(&myTrie);
		SuffixStringsInserter ssi(&myTrie);

		//insert the item strings
		ItemExactStrings itemES(itemTraits.exactStrings());
		ItemSuffixStrings itemSS(itemTraits.suffixStrings());
		for(auto it(itemsBegin); it != itemsEnd; ++it) {
			itemES(*it, esi);
			itemSS(*it, ssi);
		}
		
		RegionExactStrings regionES(regionTraits.exactStrings());
		RegionSuffixStrings regionSS(regionTraits.suffixStrings());
		for(auto it(regionsBegin); it != regionsEnd; ++it) {
			regionES(*it, esi);
			regionSS(*it, ssi);
		}
		
		myTrie.finalize();
		myTrie.append(dest);
	}
	
	detail::OOMSACTCCreator::MyStaticTrie mst(sserialize::UByteArrayAdapter(dest, flatTrieBaseBegin));
	
	detail::OOMSACTCCreator::MyStaticTrieInfo ti(&mst);
	
	OOMCTCValuesCreator vc;
	
	//insert the regions
	{
		RegionInputTraits regionInputTraits(regionTraits, &ti, false);
		vc.insert(regionsBegin, regionsEnd, regionInputTraits);
	}
	//insert the items
	{
		ItemInputTraits itemInputTraits(itemTraits, &ti, false);
		vc.insert(itemsBegin, itemsEnd, itemInputTraits);
	}
	
	//now serialize it
	{
		dest.putUint8(1); //version of sserialize::Static::UnicodeTrie::FlatTrie
		sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> pc(dest);
		OutputTraits outPutTraits(&idxFactory, &pc);
		vc.append(outPutTraits);
		pc.flush();
	}
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