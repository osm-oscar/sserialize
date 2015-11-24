#ifndef SSERIALIZE_OOM_SA_CTC_CREATOR_H
#define SSERIALIZE_OOM_SA_CTC_CREATOR_H
#include <sserialize/containers/MMVector.h>
#include <sserialize/containers/HashBasedFlatTrie.h>
#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/search/OOMCTCValueStore.h>
#include <sserialize/search/StringCompleter.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <unordered_set>

/**

struct Traits {
	typedef ItemType item_type;
	//all strings that should be searchable by exact and possibly prefix search
	struct ExactStrings {
		template<typename TOutputIterator>
		void operator(item_type item, TOutputIterator out);
	};
	//all strings that should be searchable by suffix and possibly substring search
	struct SuffixStrings {
		template<typename TOutputIterator>
		void operator(item_type item, TOutputIterator out);
	};
};

*/
namespace sserialize {
namespace detail {
namespace OOMSACTCCreator {

class CTCValueStoreNode final {
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

}}

template<>
struct is_trivially_copyable<detail::OOMSACTCCreator::CTCValueStoreNode> {
	static constexpr bool value = true;
};

} //end namespace sserialize::detail::OOMSACTCCreator

namespace std {

template<>
struct hash<sserialize::detail::OOMSACTCCreator::CTCValueStoreNode> {
	std::hash<uint64_t> hS;
	inline size_t operator()(const sserialize::detail::OOMSACTCCreator::CTCValueStoreNode & v) const {
		return (static_cast<uint64_t>(v.nodeId()) << 8) | v.qt();
	}
};

}//end namespace std

namespace sserialize {
namespace detail {
namespace OOMSACTCCreator {

typedef sserialize::Static::UnicodeTrie::FlatTrieBase MyStaticTrie;

class MyStaticTrieInfo {
public:
	typedef MyStaticTrie MyTrieType;
private:
	inline std::vector<uint32_t> calcParents(const MyTrieType & ft) {
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
		return c.parents;
	}
public:
	MyStaticTrieInfo(MyStaticTrie * t) : m_trie(t), m_parents(calcParents(*t)) {}
	~MyStaticTrieInfo() {}
	inline const MyTrieType * trie() const { return m_trie; }
	inline const std::vector<uint32_t> & parents() const { return m_parents; }
private:
	MyTrieType * m_trie;
	std::vector<uint32_t> m_parents;
};


template<typename TOutPutIterator>
class ExactStringDerefIterator {
public:
	ExactStringDerefIterator() : m_t(0) {}
	ExactStringDerefIterator (const MyStaticTrieInfo * t, TOutPutIterator out) : m_t(t), m_out(out) {}
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
	const MyStaticTrieInfo * m_t;
	TOutPutIterator m_out;
};

template<typename TOutPutIterator>
class SuffixStringDerefIterator {
public:
	SuffixStringDerefIterator() : m_t(0) {}
	SuffixStringDerefIterator (const MyStaticTrieInfo * t, TOutPutIterator out) : m_t(t), m_out(out) {}
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
	const MyStaticTrieInfo * m_t;
	TOutPutIterator m_out;
};

class BaseTraits {
public:
	typedef CTCValueStoreNode NodeIdentifier;
	struct NodeIdentifierEqualPredicate {
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
	NodeIdentifierEqualPredicate nodeIdentifierEqualPredicate() { return NodeIdentifierEqualPredicate(); }
	NodeIdentifierLessThanComparator nodeIdentifierLessThanComparator() { return NodeIdentifierLessThanComparator(); }
};

template<typename TBaseTraits>
class InputTraits: public TBaseTraits {
public:
	typedef TBaseTraits MyBaseTraits;
	typedef typename MyBaseTraits::item_type item_type;
	typedef sserialize::detail::OOMSACTCCreator::MyStaticTrieInfo MyStaticTrieInfo;
	
	typedef typename MyBaseTraits::ExactStrings ExactStrings;
	typedef typename MyBaseTraits::SuffixStrings SuffixStrings;
	
	class FullMatchPredicate {
	private:
		bool m_fullMatch;
	public:
		FullMatchPredicate(bool fullMatch) : m_fullMatch(fullMatch) {}
		bool operator()(const item_type & /*item*/) { return m_fullMatch; }
	};
	
	class ItemTextSearchNodes {
	private:
		typedef std::insert_iterator< std::unordered_set<uint32_t> > MyInsertIterator;
		typedef ExactStringDerefIterator<MyInsertIterator> MyExactStringInsertIterator;
		typedef SuffixStringDerefIterator<MyInsertIterator> MySuffixStringInsertIterator;
	private:
		const MyStaticTrieInfo * m_ti;
		ExactStrings m_es;
		SuffixStrings m_ss;
		std::unordered_set<uint32_t> m_exactNodes;
		std::unordered_set<uint32_t> m_suffixNodes;
		MyExactStringInsertIterator m_esi;
		MySuffixStringInsertIterator m_ssi;
		std::unordered_set<CTCValueStoreNode> m_allNodes;
		sserialize::StringCompleter::SupportedQuerries m_sq;
	public:
		ItemTextSearchNodes(const MyStaticTrieInfo * ti, const ExactStrings & es, const SuffixStrings & ss, sserialize::StringCompleter::SupportedQuerries sq) :
		m_ti(ti), m_es(es), m_ss(ss),
		m_esi(ti, MyInsertIterator(m_exactNodes, m_exactNodes.begin())),
		m_ssi(ti, MyInsertIterator(m_suffixNodes, m_suffixNodes.begin())),
		m_sq(sq)
		{}
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) {
			if (m_sq & sserialize::StringCompleter::SQ_EP) {
				m_es(item, m_esi);
			}
			if (m_sq & sserialize::StringCompleter::SQ_SSP) {
				m_ss(item, m_ssi);
			}
			
			//now create the parents
			if (m_sq & sserialize::StringCompleter::SQ_PREFIX) {
				for(uint32_t x : m_exactNodes) {
					m_allNodes.emplace(x, sserialize::StringCompleter::QT_EXACT);
					uint32_t nextNode = x;
					while (nextNode != 0xFFFFFFFF) {
						m_allNodes.emplace(nextNode, sserialize::StringCompleter::QT_PREFIX);
						nextNode = m_ti->parents().at(nextNode);
					}
				}
			}
			else {
				for(uint32_t x : m_exactNodes) {
					m_allNodes.emplace(x, sserialize::StringCompleter::QT_EXACT);
				}
			}
			if (m_sq & StringCompleter::SQ_SUBSTRING) {
				for(uint32_t x : m_suffixNodes) {
					m_allNodes.emplace(x, sserialize::StringCompleter::QT_SUFFIX);
					uint32_t nextNode = x;
					while (nextNode != 0xFFFFFFFF) {
						m_allNodes.emplace(nextNode, sserialize::StringCompleter::QT_SUBSTRING);
						nextNode = m_ti->parents().at(nextNode);
					}
				}
			}
			else {
				for(uint32_t x : m_suffixNodes) {
					m_allNodes.emplace(x, sserialize::StringCompleter::QT_SUFFIX);
				}
			}
			//and out it goes
			for(const auto & x : m_allNodes) {
				*out = x;
				++out;
			}
			m_exactNodes.clear();
			m_suffixNodes.clear();
			m_allNodes.clear();
		}
	};
private:
	MyStaticTrieInfo * m_ti;
	bool m_fullMatch;
	sserialize::StringCompleter::SupportedQuerries m_sq;
public:
	InputTraits(const MyBaseTraits & baseTraits, MyStaticTrieInfo * ti, bool allAreFullMatch, sserialize::StringCompleter::SupportedQuerries sq) :
	MyBaseTraits(baseTraits), m_ti(ti), m_fullMatch(allAreFullMatch), m_sq(sq) {}

	FullMatchPredicate fullMatchPredicate() { return FullMatchPredicate(m_fullMatch); }
	ItemTextSearchNodes itemTextSearchNodes() { return ItemTextSearchNodes(m_ti, MyBaseTraits::exactStrings(), MyBaseTraits::suffixStrings(), m_sq); }
};

class OutputTraits {
public:
	typedef BaseTraits::NodeIdentifier NodeIdentifier;
	class IndexFactoryOut {
		sserialize::ItemIndexFactory * m_idxFactory;
	public:
		IndexFactoryOut(sserialize::ItemIndexFactory * idxFactory) : m_idxFactory(idxFactory) {}
		template<typename TIterator>
		uint32_t operator()(TIterator begin, TIterator end) {
			std::vector<uint32_t> tmp(begin, end);
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
		DataOut(PayloadCreator * pc) : m_payloadCreator(pc), m_curNodeId(0), m_curTypes(sserialize::StringCompleter::QT_NONE) {}
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
			m_curOffsets.emplace_back( m_curData.tellPutPtr() );
			m_curData.put(data);
		}
	};
	typedef sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> PayloadCreator;
	typedef sserialize::ItemIndexFactory ItemIndexFactory;
private:
	ItemIndexFactory * m_idxFactory;
	PayloadCreator * m_payloadCreator;
	uint64_t m_maxMemoryUsage;
	sserialize::MmappedMemoryType m_mmt;
public:
	OutputTraits(ItemIndexFactory * idxFactory, PayloadCreator * payloadCreator, uint64_t maxMemoryUsage, MmappedMemoryType mmt) :
	m_idxFactory(idxFactory), m_payloadCreator(payloadCreator), m_maxMemoryUsage(maxMemoryUsage), m_mmt(mmt)
	{}
	
	inline IndexFactoryOut indexFactoryOut() { return IndexFactoryOut(m_idxFactory); }
	inline DataOut dataOut() { return DataOut(m_payloadCreator); }
	
	inline uint64_t maxMemoryUsage() const { return m_maxMemoryUsage; }
	inline sserialize::MmappedMemoryType mmt() const { return m_mmt; }
};

}}//end namespace detail::OOMSACTCCreator

template<typename TItemIterator, typename TRegionIterator, typename TItemTraits, typename TRegionTraits>
void appendSACTC(TItemIterator itemsBegin, TItemIterator itemsEnd, TRegionIterator regionsBegin, TRegionIterator regionsEnd,
					TItemTraits itemTraits, TRegionTraits regionTraits,
					uint64_t maxMemoryUsage, sserialize::StringCompleter::SupportedQuerries sq,
					sserialize::ItemIndexFactory & idxFactory, sserialize::UByteArrayAdapter & dest)
{
	typedef detail::OOMSACTCCreator::BaseTraits BaseTraits;
	typedef TItemTraits ItemTraits;
	typedef TRegionTraits RegionTraits;
	typedef detail::OOMSACTCCreator::InputTraits<ItemTraits> ItemInputTraits;
	typedef detail::OOMSACTCCreator::InputTraits<RegionTraits> RegionInputTraits;

	typedef typename ItemTraits::ExactStrings ItemExactStrings;
	typedef typename ItemTraits::SuffixStrings ItemSuffixStrings;
	typedef typename RegionTraits::ExactStrings RegionExactStrings;
	typedef typename RegionTraits::SuffixStrings RegionSuffixStrings;
	
	typedef detail::OOMSACTCCreator::OutputTraits OutputTraits;
	
	sserialize::UByteArrayAdapter::OffsetType flatTrieBaseBegin = dest.tellPutPtr();
	{
		sserialize::HashBasedFlatTrie<uint32_t> myTrie;
		
		struct ExactStringsInserter {
			sserialize::HashBasedFlatTrie<uint32_t> * m_t;
			ExactStringsInserter(sserialize::HashBasedFlatTrie<uint32_t> * t) : m_t(t) {}
			ExactStringsInserter & operator*() { return *this;}
			ExactStringsInserter & operator=(const std::string & str) {
				m_t->insert(str);
				return *this;
			}
			ExactStringsInserter & operator++() { return *this; }
		};
		
		struct SuffixStringsInserter {
			sserialize::HashBasedFlatTrie<uint32_t> * m_t;
			SuffixStringsInserter(sserialize::HashBasedFlatTrie<uint32_t> * t) : m_t(t) {}
			SuffixStringsInserter & operator*() { return *this;}
			SuffixStringsInserter & operator=(const std::string & str) {
				auto sstr = m_t->insert(str);
				if (!str.size()) {
					return *this;
				}
				std::string::const_iterator strPrev(str.cbegin());
				std::string::const_iterator strIt(strPrev), strEnd(str.cend());
				for(utf8::next(strIt, strEnd); strIt != strEnd; utf8::next(strIt, strEnd)) {
					sstr.addOffset(std::distance(strPrev, strIt));
					strPrev = strIt;
				}
				return *this;
			}
			SuffixStringsInserter & operator++() { return *this; }
		};

		ExactStringsInserter esi(&myTrie);
		SuffixStringsInserter ssi(&myTrie);

		//insert the item strings
		ItemExactStrings itemES(itemTraits.exactStrings());
		ItemSuffixStrings itemSS(itemTraits.suffixStrings());
		for(auto it(itemsBegin); it != itemsEnd; ++it) {
			if (sq & sserialize::StringCompleter::SQ_EP) {
				itemES(*it, esi);
			}
			if (sq & sserialize::StringCompleter::SQ_SSP) {
				itemSS(*it, ssi);
			}
		}
		
		RegionExactStrings regionES(regionTraits.exactStrings());
		RegionSuffixStrings regionSS(regionTraits.suffixStrings());
		for(auto it(regionsBegin); it != regionsEnd; ++it) {
			if (sq & sserialize::StringCompleter::SQ_EP) {
				regionES(*it, esi);
			}
			if (sq & sserialize::StringCompleter::SQ_SSP) {
				regionSS(*it, ssi);
			}
		}
		
		myTrie.finalize();
		myTrie.append(dest);
	}
	
	detail::OOMSACTCCreator::MyStaticTrie mst(sserialize::UByteArrayAdapter(dest, flatTrieBaseBegin));
	
	detail::OOMSACTCCreator::MyStaticTrieInfo ti(&mst);
	
	BaseTraits baseTraits;
	OOMCTCValuesCreator<BaseTraits> vc(baseTraits);
	
	//insert the regions
	{
		RegionInputTraits regionInputTraits(regionTraits, &ti, true, sq);
		vc.insert(regionsBegin, regionsEnd, regionInputTraits);
	}
	//insert the items
	{
		ItemInputTraits itemInputTraits(itemTraits, &ti, false, sq);
		vc.insert(itemsBegin, itemsEnd, itemInputTraits);
	}
	
	//now serialize it
	{
		dest.putUint8(1); //version of sserialize::Static::UnicodeTrie::FlatTrie
		sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> pc(dest);
		OutputTraits outPutTraits(&idxFactory, &pc, maxMemoryUsage, sserialize::MM_SLOW_FILEBASED);
		vc.append(outPutTraits);
		pc.flush();
		assert(pc.size() == mst.size());
	}
}

}//end namespace


#endif