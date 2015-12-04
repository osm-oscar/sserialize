#ifndef SSERIALIZE_OOM_CTC_VALUES_CREATOR_H
#define SSERIALIZE_OOM_CTC_VALUES_CREATOR_H
#include <sserialize/containers/MMVector.h>
#include <sserialize/iterator/TransformIterator.h>
#include <sserialize/algorithm/oom_algorithm.h>
#include <sserialize/containers/RLEStream.h>
#include <limits>

namespace sserialize {
namespace detail {
namespace OOMCTCValuesCreator {

template<typename TIterator, bool TWithProgressInfo>
struct ProgressInfo;

template<typename TIterator>
struct ProgressInfo<TIterator, true> {
	uint64_t targetCount;
	uint64_t lastCounter;
	uint64_t counter;
	sserialize::ProgressInfo pinfo;
	ProgressInfo(const TIterator & begin, const TIterator & end) :
	targetCount(0),
	counter(0)
	{
		using std::distance;
		targetCount = distance(begin, end);
	}
	inline void reset() { counter = 0; }
	inline void begin(const std::string & message) {
		reset();
		pinfo.begin(targetCount, message);
	}
	inline void end() {pinfo.end();}
	
	///like ProgressInfo::operator()()
	inline void operator()(uint64_t currentCount) {
		pinfo(currentCount);
	}
	
	inline void inc(uint64_t delta) {
		counter += delta;
		if (delta > 0x7F || (counter & 0x7F) == 0) {
			pinfo(counter);
		}
	}
};

template<typename TIterator>
struct ProgressInfo<TIterator, false> {
	ProgressInfo(const TIterator &, const TIterator &) {}
	inline void reset() {}
	inline void begin(const std::string &) {}
	inline void end() {}
	inline void operator()(uint64_t) {}
	inline void inc(uint64_t) {}
};

template<typename TNodeIdentifier>
class ValueEntry final {
public:
	typedef TNodeIdentifier NodeIdentifier;
public:
	ValueEntry() : m_cellId(0), m_itemId(0) {}
	~ValueEntry() {}
	uint32_t cellId() const { return m_cellId; }
	void cellId(uint32_t v) { m_cellId = v; }
	
	void setFullMatch() { m_itemId = FULL_MATCH; }
	bool fullMatch() const { return m_itemId == FULL_MATCH; }

	void itemId(uint32_t v) { m_itemId = v; }
	uint32_t itemId() const { return m_itemId; }

	const NodeIdentifier & nodeId() const { return m_nodeId; }
	void nodeId(const NodeIdentifier & v) { m_nodeId = v; }
private:
	static constexpr const uint32_t FULL_MATCH = std::numeric_limits<uint32_t>::max();
	static constexpr const uint32_t NULL_CELL = std::numeric_limits<uint32_t>::max();
private:
	uint32_t m_cellId;
	//stores either the item or FULL_MATCH meaning that this cell is fully matched 
	uint32_t m_itemId;
	NodeIdentifier m_nodeId;
};

template<typename TNodeIdentifier>
class ValueEntryItemIdIteratorMapper {
public:
	uint32_t operator()(const ValueEntry<TNodeIdentifier> & e) const { return e.itemId(); }
};

// template<typename TNodeIdentifier, typename TValueEntryIterator>
// using sserialize::TransformIterator< ValueEntryItemIteratorMapper<TNodeIdentifier>, uint32_t,  TValueEntryIterator> = ValueEntryItemIterator;

}}//end namespace detail::OOMCTCValuesCreator

template<typename TNodeIdentifier>
struct is_trivially_copyable< detail::OOMCTCValuesCreator::ValueEntry<TNodeIdentifier> > {
	static constexpr bool value = sserialize::is_trivially_copyable<TNodeIdentifier>::value;
};

/**
  * This class creates the values stored in a text search data structure
  * The text search has to provide a deterministic mapping of strings to unsigned integers,
  * from now on called nodeId.
  * The fill function takes as argument an ItemDerefer, which has an operator()()
  * taking an OsmKeyValueObjectStore::Item and an output iterator (which should be templateized)
  * The fill() function maybe called multiple times
  * 
  * 
  * struct BaseTraits {
  *   typdef <some type> NodeIdentifier
  *   struct NodeIdentifierLessThanPredicate {
  *     bool operator()(NodeIdentifier a, NodeIdentifier b);
  *   };
  *   struct NodeIdentifierEqualPredicate {
  *     bool operator()(NodeIdentifier a, NodeIdentifier b);
  *   };
  * };
  * typdef InputIterator::value_type item_type; //the value type the input iterators dereference to
  * 
  * struct InsertionTraits {
  *   ///returns if item produces full matches (in essence a region)
  *   struct FullMatchPredicate {
  *     bool operator()(item_type item);
  *   }
  *   struct ItemId {
  *     uint32_t operator()(item_type item);
  *   }
  *   struct ItemCells {
  *     template<typename TOutputIterator>
  *     void operator()(item_type item, TOutputIterator out);
  *   }
  *   struct ItemTextSearchNodes {
  *     template<typename TOutputIterator>
  *     void operator()(item_type item, TOutputIterator out);
  *   }
  *   NodeIdentifierLessThanComparator nodeIdentifierLessThanComparator();
  *   NodeIdentifierEqualComparator nodeIdentifierEqualComparator();
  *   NodeIdentifierHashFunction nodeIdentifierHashFunction();
  *   ItemDerefer itemDerefer();
  * }
  * struct OutputTraits {
  *   struct DataOutput {
  *     void operator()(BaseTraits::NodeIdentifier node, const sserialize::UByteArrayAdapter & nodePayload);
  *   };
  *   struct IndexFactoryOut {
  *     template<typename TInputIterator>
  *     uint32_t operator()(TInputIterator begin, TInputIterator end);
  *   };
  * };
  */

template<typename TBaseTraits>
class OOMCTCValuesCreator {
public:
	typedef TBaseTraits Traits;
	typedef typename Traits::NodeIdentifier NodeIdentifier;
public:
	OOMCTCValuesCreator(const Traits & traits);
	template<typename TItemIterator, typename TInsertionTraits, bool TWithProgressInfo = true>
	bool insert(TItemIterator begin, const TItemIterator & end, TInsertionTraits itraits, uint32_t threadCount = 0);
	template<typename TOutputTraits, bool TWithProgressInfo = true>
	void append(TOutputTraits otraits);
private:
	typedef detail::OOMCTCValuesCreator::ValueEntry<NodeIdentifier> ValueEntry;
	typedef sserialize::MMVector<ValueEntry> TreeValueEntries;
private:
	template<typename TOutputTraits, bool TWithProgressInfo>
	bool finalize(TOutputTraits & otraits);
private:
	Traits m_traits;
	TreeValueEntries m_entries;
};

template<typename TBaseTraits>
OOMCTCValuesCreator<TBaseTraits>::OOMCTCValuesCreator(const TBaseTraits & traits) :
m_traits(traits),
m_entries(sserialize::MM_FAST_FILEBASED)
{}

template<typename TBaseTraits>
template<typename TItemIterator, typename TInputTraits, bool TWithProgressInfo>
bool
OOMCTCValuesCreator<TBaseTraits>::insert(TItemIterator begin, const TItemIterator & end, TInputTraits itraits, uint32_t threadCount)
{
	typedef typename TInputTraits::FullMatchPredicate FullMatchPredicate;
	typedef typename TInputTraits::ItemId ItemIdExtractor;
	typedef typename TInputTraits::ItemCells ItemCellsExtractor;
	typedef typename TInputTraits::ItemTextSearchNodes ItemTextSearchNodesExtractor;
	typedef TItemIterator ItemIterator;
	
	struct State {
		TItemIterator it;
		TItemIterator end;
		std::mutex itLock;
		
		//flush
		TreeValueEntries * entries;
		std::mutex flushLock;
		
		detail::OOMCTCValuesCreator::ProgressInfo<ItemIterator, TWithProgressInfo> pinfo;
		uint32_t counter;
		
		State(TItemIterator begin, TItemIterator end, TreeValueEntries * entries) :
		it(begin), end(end),
		entries(entries),
		pinfo(begin, end),
		counter(0)
		{}
		
		inline void incCounter() {
			++counter;
			if ((counter & 0x1FFFF) == 0) {
				pinfo(counter);
			}
		}
	};
	
	struct Worker {
		State * state;
		std::vector<ValueEntry> outBuffer;
		uint32_t outBufferSize;
		
		//item dependend
		FullMatchPredicate fmPred;
		ItemIdExtractor itemIdE;
		ItemCellsExtractor itemCellsE;
		ItemTextSearchNodesExtractor nodesE;
		
		std::vector<uint32_t> itemCells;
		std::vector<NodeIdentifier> itemNodes;
		std::vector<ValueEntry> itemEntries;
		
		std::back_insert_iterator< std::vector<uint32_t> > itemCellsBI;
		std::back_insert_iterator< std::vector<NodeIdentifier> > itemNodesBI;

		void flush() {
			std::unique_lock<std::mutex> lck(state->itLock);
			state->entries->push_back(outBuffer.begin(), outBuffer.end());
			outBuffer.clear();
		}
		
		void operator()() {
			std::unique_lock<std::mutex> itLock(state->itLock);
			itLock.unlock();
			
			while (true) {
				itLock.lock();
				if (state->it == state->end) {
					itLock.unlock();
					flush();
					return;
				}
				auto item = *(state->it);
				++(state->it);
				itLock.unlock();
				state->incCounter();
				
				itemCells.clear();
				itemNodes.clear();
				itemEntries.clear();
				
				ValueEntry e;
				if (fmPred(item)) {
					e.setFullMatch();
				}
				else {
					e.itemId(itemIdE(item));
				}
				itemCellsE(item, itemCellsBI);
				nodesE(item, itemNodesBI);
				for(const auto & node : itemNodes) {
					e.nodeId(node);
					for(uint32_t cellId : itemCells) {
						e.cellId(cellId);
						itemEntries.push_back(e);
					}
				}
				outBuffer.insert(outBuffer.end(), itemEntries.cbegin(), itemEntries.cend());
				state->incCounter();
				if (outBuffer.size() > outBufferSize) {
					flush();
				}
			}
			flush();
		}
		Worker(State * state, TInputTraits & itraits) :
		state(state),
		outBufferSize(100000),
		fmPred(itraits.fullMatchPredicate()),
		itemIdE(itraits.itemId()),
		itemCellsE(itraits.itemCells()),
		nodesE(itraits.itemTextSearchNodes()),
		itemCellsBI(itemCells),
		itemNodesBI(itemNodes)
		{}
		Worker(Worker && other) :
		state(other.state),
		outBuffer(std::move(other.outBuffer)),
		outBufferSize(10000),
		fmPred(std::move(other.fmPred)),
		itemIdE(std::move(other.itemIdE)),
		itemCellsE(std::move(other.itemCellsE)),
		nodesE(std::move(other.nodesE)),
		itemCells(std::move(other.itemCells)),
		itemNodes(std::move(other.itemNodes)),
		itemEntries(std::move(other.itemEntries)),
		itemCellsBI(itemCells),
		itemNodesBI(itemNodes)
		{}
	};
	
	if (!threadCount) {
		threadCount = std::thread::hardware_concurrency();
	}
	
	State state(begin, end, &m_entries);
	
	state.pinfo.begin("OOMCTCValuesCreator::Inserting");
	std::vector<std::thread> threads;
	for(uint32_t i(0); i < threadCount; ++i) {
		threads.emplace_back( Worker(&state, itraits) );
	}
	for(uint32_t i(0); i < threadCount; ++i) {
		threads[i].join();
	}
	threads.clear();
	state.pinfo.end();
	return true;
}

template<typename TBaseTraits>
template<typename TOutputTraits, bool TWithProgressInfo>
void OOMCTCValuesCreator<TBaseTraits>::append(TOutputTraits otraits)
{
	typedef TOutputTraits OutputTraits;
	typedef typename Traits::NodeIdentifier NodeIdentifier;
	typedef typename Traits::NodeIdentifierEqualPredicate NodeIdentifierEqualPredicate;
	typedef typename OutputTraits::IndexFactoryOut IndexFactoryOut;
	typedef typename OutputTraits::DataOut DataOut;
	
	typedef typename TreeValueEntries::const_iterator TVEConstIterator;
	
	typedef detail::OOMCTCValuesCreator::ValueEntryItemIdIteratorMapper<NodeIdentifier> VEItemIdIteratorMapper;
	typedef sserialize::TransformIterator<VEItemIdIteratorMapper, uint32_t, TVEConstIterator> VEItemIdIterator;
	
	if (TWithProgressInfo) {
		std::cout << "OOMCTCValuesCreator: Finalizing" << std::endl;
	}
	finalize<OutputTraits, TWithProgressInfo>(otraits);
	
	NodeIdentifierEqualPredicate nep(m_traits.nodeIdentifierEqualPredicate());
	IndexFactoryOut ifo(otraits.indexFactoryOut());
	DataOut dout(otraits.dataOut());
	
	struct SingleEntryState {
		std::vector<uint32_t> fmCellIds;
		std::vector<uint32_t> pmCellIds;
		std::vector<uint32_t> pmCellIdxPtrs;
		sserialize::UByteArrayAdapter sd;
		SingleEntryState() : sd(sserialize::UByteArrayAdapter::createCache(0, sserialize::MM_PROGRAM_MEMORY)) {}
		void clear() {
			fmCellIds.clear();
			pmCellIds.clear();
			pmCellIdxPtrs.clear();
			sd.resize(0);
		}
	} ses;
	
	sserialize::OptionalProgressInfo<TWithProgressInfo> pinfo;
	pinfo.begin(std::distance(m_entries.begin(), m_entries.end()), "OOMCTCValueStore::Calculating payload");
	for(TVEConstIterator eIt(m_entries.begin()), eBegin(m_entries.begin()), eEnd(m_entries.end()); eIt != eEnd;) {
		const NodeIdentifier & ni = eIt->nodeId();
		for(; eIt != eEnd && nep(eIt->nodeId(), ni);) {
			//find the end of this cell
			TVEConstIterator cellBegin(eIt);
			uint32_t cellId = eIt->cellId();
			for(;eIt != eEnd && eIt->cellId() == cellId && !eIt->fullMatch() && nep(eIt->nodeId(), ni); ++eIt) {}
			if (cellBegin != eIt) { //there are partial matches
				uint32_t indexId = ifo(VEItemIdIterator(cellBegin), VEItemIdIterator(eIt));
				ses.pmCellIds.push_back(cellId);
				ses.pmCellIdxPtrs.push_back(indexId);
			}
			//check if we have full matches
			if (eIt->cellId() == cellId && eIt->fullMatch() && nep(eIt->nodeId(), ni)) {
				ses.fmCellIds.push_back(cellId);
				//skip this entry, it's the only one since other fm were removed by finalize()
				++eIt;
			}
		}
		//serialize the data
		uint32_t fmIdxPtr = ifo(ses.fmCellIds.begin(), ses.fmCellIds.end());
		uint32_t pmIdxPtr = ifo(ses.pmCellIds.begin(), ses.pmCellIds.end());
		sserialize::RLEStream::Creator rlc(ses.sd);
		rlc.put(fmIdxPtr);
		rlc.put(pmIdxPtr);
		for(auto x : ses.pmCellIdxPtrs) {
			rlc.put(x);
		}
		rlc.flush();
		dout(ni, ses.sd);
		ses.clear();
		//eIt now points to the next node or the end
		pinfo(std::distance(eBegin, eIt));
	}
	pinfo.end();
}

///Sorts the storage and makes it unique
template<typename TBaseTraits>
template<typename TOutputTraits, bool TWithProgressInfo>
bool OOMCTCValuesCreator<TBaseTraits>::finalize(TOutputTraits & otraits) {
	struct LessThan {
		typedef typename Traits::NodeIdentifierLessThanComparator NodeComparator;
		NodeComparator nc;
		LessThan(const NodeComparator & nc) : nc(nc) {}
		bool operator()(const ValueEntry & a, const ValueEntry & b) {
			if (nc(a.nodeId(), b.nodeId())) {
				return true;
			}
			else if (nc(b.nodeId(), a.nodeId())) {
				return false;
			}
			else { //they compare equal, check the cellId and then the nodeId/FullMatch
				if (a.cellId() < b.cellId()) {
					return true;
				}
				else if (b.cellId() < a.cellId()) {
					return false;
				}
				else { //cellId are the same
					return a.itemId() < b.itemId(); //this moves full match cells to the end
				}
			}
		}
	};
	struct Equal {
		typedef typename Traits::NodeIdentifierEqualPredicate NodeComparator;
		NodeComparator m_nc;
		Equal(const NodeComparator & nc) : m_nc(nc) {}
		bool operator()(const ValueEntry & a, const ValueEntry & b) {
			return (m_nc.operator()(a.nodeId(), b.nodeId()) && a.cellId() == b.cellId() && a.itemId() == b.itemId());
		}
	};
	
	typedef typename TreeValueEntries::iterator TVEIterator;
	
	LessThan ltp(m_traits.nodeIdentifierLessThanComparator());
	Equal ep(m_traits.nodeIdentifierEqualPredicate());
	
	sserialize::oom_sort<TVEIterator, LessThan, TWithProgressInfo>(m_entries.begin(), m_entries.end(), ltp, otraits.maxMemoryUsage(), 2, otraits.mmt());
	auto equalEnd = sserialize::oom_unique<TVEIterator, Equal, TWithProgressInfo>(m_entries.begin(), m_entries.end(), otraits.mmt(), ep);
	m_entries.resize(std::distance(m_entries.begin(), equalEnd));
	return true;
}



}//end namespace
#endif