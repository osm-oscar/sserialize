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
	typedef sserialize::OOMArray<ValueEntry> TreeValueEntries;
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
m_entries(sserialize::MM_SLOW_FILEBASED)
{
	//backbuffer should be at least 100MiB to have enough data on a flush
	m_entries.backBufferSize(100*1024*1024);
	m_entries.readBufferSize(10*1024*1024);
}

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
		std::size_t outBufferSize;
		
		//item dependend
		FullMatchPredicate fmPred;
		ItemIdExtractor itemIdE;
		ItemCellsExtractor itemCellsE;
		ItemTextSearchNodesExtractor nodesE;
		
		std::vector<uint32_t> itemCells;
		std::vector<NodeIdentifier> itemNodes;
		
		std::back_insert_iterator< std::vector<uint32_t> > itemCellsBI;
		std::back_insert_iterator< std::vector<NodeIdentifier> > itemNodesBI;

		void flush() {
			std::unique_lock<std::mutex> lck(state->flushLock);
			state->entries->push_back(outBuffer.begin(), outBuffer.end());
			if (outBuffer.size() > 2*outBufferSize) { //outbuffer was way too large
				std::cout << "OOMCTCValueStorage: flushing a buffer that is " << (double)outBuffer.size() / outBufferSize << " times larger than target buffer size" << std::endl;
				outBuffer.clear();
				outBuffer.shrink_to_fit();
			}
			else {
				outBuffer.clear();
			}
		}
		
		inline void flushOnFull() {
			if (UNLIKELY_BRANCH(outBuffer.size() > outBufferSize)) {
				flush();
			}
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
				
				itemCells.clear();
				itemNodes.clear();
				
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
						outBuffer.push_back(e);
					}
					flushOnFull();
				}
				state->incCounter();
			}
			flush();
		}
		Worker(State * state, TInputTraits & itraits) :
		state(state),
		outBufferSize((10*1024*1024)/sizeof(ValueEntry)),
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
		outBufferSize(other.outBufferSize),
		fmPred(std::move(other.fmPred)),
		itemIdE(std::move(other.itemIdE)),
		itemCellsE(std::move(other.itemCellsE)),
		nodesE(std::move(other.nodesE)),
		itemCells(std::move(other.itemCells)),
		itemNodes(std::move(other.itemNodes)),
		itemCellsBI(itemCells),
		itemNodesBI(itemNodes)
		{}
	};
	
	if (!threadCount) {
		threadCount = std::thread::hardware_concurrency();
	}
	
	State state(begin, end, &(this->m_entries));
	
	state.pinfo.begin("OOMCTCValuesCreator::Inserting");
	this->m_entries.syncOnFlush(false);
	std::vector<std::thread> threads;
	for(uint32_t i(0); i < threadCount; ++i) {
		threads.emplace_back( Worker(&state, itraits) );
	}
	for(uint32_t i(0); i < threadCount; ++i) {
		threads[i].join();
	}
	threads.clear();
	this->m_entries.syncOnFlush(true);
	this->m_entries.sync();
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
	
	sserialize::OptionalProgressInfo<TWithProgressInfo> pinfo;
	
	if (TWithProgressInfo) {
		std::cout << "OOMCTCValuesCreator: Finalizing" << std::endl;
	}
	pinfo.begin(1, "Finalizing");
	finalize<OutputTraits, TWithProgressInfo>(otraits);
	pinfo.end();
	
	NodeIdentifierEqualPredicate nep(m_traits.nodeIdentifierEqualPredicate());
	
	struct State {
		std::mutex eItLock;
		TVEConstIterator eBegin;
		TVEConstIterator eIt;
		TVEConstIterator eEnd;
		sserialize::OptionalProgressInfo<TWithProgressInfo> & pinfo = pinfo;
	} state;
	
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
	};
	
	class Worker {
	public:
		Worker(State * s, const NodeIdentifierEqualPredicate & nep, OutputTraits & otraits) :
		state(s),
		nep(nep),
		ifo(otraits.indexFactoryOut()),
		dout(otraits.dataOut()),
		eIt(state->eBegin.copy())
		{
			eIt.bufferSize(s->eIt.bufferSize());
		}
		void operator()() {
			std::unique_lock<std::mutex> lock(state->eItLock);
			lock.unlock();
			while (true) {
				lock.lock();
				if (state->eIt != state->eEnd) {
					getNext();
					using std::distance;
					state->pinfo(distance(state->eBegin, state->eIt));
					lock.unlock();
				}
				else {
					return;
				}
				handle();
			}
		};
		
		void getNext() {
			//reposition to beginning of new entry
			eIt += (state->eIt - eIt);
			
			//now move the global iterator to the next entry
			NodeIdentifier ni = eIt->nodeId();
			for(; state->eIt != state->eEnd && nep(state->eIt->nodeId(), ni); ++state->eIt) {}
		}
		
		void handle() {
			const TVEConstIterator & eEnd = state->eEnd;
			NodeIdentifier ni = eIt->nodeId();
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
				if (eIt != eEnd && eIt->cellId() == cellId && eIt->fullMatch() && nep(eIt->nodeId(), ni)) {
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
		}
		
	private:
		State * state;
		NodeIdentifierEqualPredicate nep;
		IndexFactoryOut ifo;
		DataOut dout;
		SingleEntryState ses;
		TVEConstIterator eIt;
	};
	
	state.eBegin = m_entries.begin();
	state.eIt = m_entries.begin();
	state.eEnd = m_entries.end();
	state.eIt.bufferSize(100*1024*1024);//set a read-buffer size of 100 MiB
	
	pinfo.begin(std::distance(m_entries.begin(), m_entries.end()), "OOMCTCValueStore::Calculating payload");
	std::vector<std::thread> threads;
	for(uint32_t i(0); i < otraits.payloadConcurrency(); ++i) {
		threads.emplace_back(
			Worker(&state, nep, otraits)
		);
	}
	for(std::thread & t : threads) {
		t.join();
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
	
	sserialize::oom_sort<TVEIterator, LessThan, TWithProgressInfo>(
		m_entries.begin(), m_entries.end(), ltp,
		otraits.maxMemoryUsage(), otraits.sortConcurrency(),
		otraits.mmt(), 1024, 30
	);
	
	auto entriesBegin = m_entries.begin();
	entriesBegin.bufferSize(otraits.maxMemoryUsage()/4);
	auto equalEnd = sserialize::oom_unique<TVEIterator, Equal, TWithProgressInfo>(std::move(entriesBegin), m_entries.end(), otraits.mmt(), otraits.maxMemoryUsage()/2, ep);
	m_entries.resize(std::distance(m_entries.begin(), equalEnd));
	m_entries.shrink_to_fit();
	using std::is_sorted;
	SSERIALIZE_EXPENSIVE_ASSERT(is_sorted(m_entries.begin(), m_entries.end(), ltp));
	return true;
}



}//end namespace
#endif