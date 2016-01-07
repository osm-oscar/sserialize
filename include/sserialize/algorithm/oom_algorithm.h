#ifndef SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#define SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#include <functional>
#include <type_traits>
#include <iterator>
#include <vector>
#include <queue>
#include <mutex>
#include <sserialize/containers/OOMArray.h>
#include <sserialize/algorithm/utilcontainerfuncs.h>
#include <sserialize/iterator/RangeGenerator.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/utility/assert.h>

namespace sserialize {
namespace detail {
namespace oom {

template<typename TSourceIterator, typename TValue = typename std::iterator_traits<TSourceIterator>::value_type>
class InputBuffer {
public:
	typedef typename std::vector<TValue>::iterator iterator;
private:
	TSourceIterator m_srcIt;
	TSourceIterator m_srcEnd;
	OffsetType m_bufferSize;
	std::vector<TValue> m_buffer;
	iterator m_bufferIt;
private:
	void fillBuffer() {
		m_buffer.clear();
		OffsetType copyAmount(0);
		for(; copyAmount < m_bufferSize && m_srcIt != m_srcEnd; ++copyAmount, ++m_srcIt) {
			m_buffer.push_back(*m_srcIt);
		}
		m_bufferIt = m_buffer.begin();
	}
public:
	InputBuffer() : m_bufferSize(0), m_bufferIt(m_buffer.begin()) {}
	InputBuffer(const TSourceIterator & srcBegin, const TSourceIterator & srcEnd, OffsetType bufferSize) :
	m_srcIt(srcBegin),
	m_srcEnd(srcEnd),
	m_bufferSize(bufferSize)
	{
		m_buffer.reserve(m_bufferSize);
		fillBuffer();
	}
	InputBuffer(InputBuffer && other) : m_srcIt(std::move(other.m_srcIt)), m_srcEnd(std::move(other.m_srcEnd)), m_bufferSize(other.m_bufferSize) {
		std::size_t bufferItOffset = other.m_bufferIt - other.m_buffer.begin();
		m_buffer = std::move(other.m_buffer);
		m_bufferIt = m_buffer.begin()+bufferItOffset;
	}
	InputBuffer & operator=(InputBuffer && other) {
		m_srcIt = std::move(other.m_srcIt);
		m_srcEnd = std::move(other.m_srcEnd);
		m_bufferSize = other.m_bufferSize;
		std::size_t bufferItOffset = other.m_bufferIt - other.m_buffer.begin();
		m_buffer = std::move(other.m_buffer);
		m_bufferIt = m_buffer.begin()+bufferItOffset;
		return *this;
	}
	TValue & get() { return *m_bufferIt; }
	const TValue & get() const { return *m_bufferIt; }
	///return true if has next
	bool next() {
		if (m_bufferIt != m_buffer.end()) {
			++m_bufferIt;
		}
		if (m_bufferIt == m_buffer.end()) {
			fillBuffer();
			return m_buffer.size();
		}
		return true;
	}
};

template<typename TIterator, typename TIteratorCategory = typename std::iterator_traits<TIterator>::iterator_category>
struct InMemorySort {
	static constexpr bool canSort = false;
	template<typename TCompare>
	inline static void sort(const TIterator &/*begin*/, const TIterator &/*end*/, const TCompare &/*compare*/) {}
};

template<typename TIterator>
struct InMemorySort<TIterator, std::random_access_iterator_tag> {
	static constexpr bool canSort = true;
	template<typename TCompare>
	inline static void sort(const TIterator & begin, const TIterator & end, const TCompare & compare) {
		std::sort(begin, end, compare);
	}
};

///This class is needed for buffered iterators 
template<typename TIterator>
struct IteratorSyncer {
	static void sync(TIterator & /*it*/) {}
};

template<typename TValue>
struct IteratorSyncer< detail::OOMArray::ConstIterator<TValue> > {
	static void sync(detail::OOMArray::ConstIterator<TValue> & it) { it.sync(); }
};


template<typename TValue>
struct IteratorSyncer< detail::OOMArray::Iterator<TValue> > {
	static void sync(detail::OOMArray::Iterator<TValue> & it) { it.sync(); }
};


}}//end namespace detail::oom

//TODO: specialise for OOMArray::iterator (special write-back, input buffer size needs to be taken into account)

///A standard out-of-memory sorting algorithm. It first sorts the input in chunks of size maxMemoryUsage/threadCount
///These chunks are then merged together in possibly multiple phases. In a single phase up to queueDepth chunks are merged together.
///@param maxMemoryUsage default is 4 GB
///@param threadCount should be no larger than about 4, 2 should be sufficient for standard io-speeds, used fo the initial chunk sorting, a single chunk then has a size of maxMemoryUsage/threadCount
///@param queueDepth the maximum number of chunks to merge in a single round, this directly influences the number of merge rounds
///@param comp comparisson operator for strict weak order. This functions needs to be thread-safe <=> threadCount > 1
///In general: Larger chunks result in a smaller number of rounds and can be processed with a smaller queue depth reducing random access
///Thus for very large data sizes it may be better to use only one thread to create the largest chunks possible
template<typename TInputOutputIterator, typename CompFunc = std::less<typename std::iterator_traits<TInputOutputIterator>::value_type>, bool TWithProgressInfo = true>
void oom_sort(TInputOutputIterator begin, TInputOutputIterator end, CompFunc comp = CompFunc(),
				uint64_t maxMemoryUsage = 0x100000000,
				uint32_t threadCount = 2,
				sserialize::MmappedMemoryType mmt = sserialize::MM_FILEBASED,
				uint32_t queueDepth = 64)
{
	typedef TInputOutputIterator SrcIterator;
	typedef typename std::iterator_traits<SrcIterator>::value_type value_type;
	typedef detail::oom::InputBuffer<SrcIterator> InputBuffer;
	
	SSERIALIZE_CHEAP_ASSERT(begin < end);
	
	if (!threadCount) {
		threadCount = std::thread::hardware_concurrency();
	}
	
	struct Config {
		//chunk size in number of entries
		uint64_t initialChunkSize;
		//max memory size in bytes
		uint64_t maxMemoryUsage;
		//tmp buffer size in bytes
		uint64_t tmpBuffferSize;
		CompFunc * comp;
	};
	
	struct State {
		OptionalProgressInfo<TWithProgressInfo> pinfo;
		uint64_t sortCompleted;
		SrcIterator srcIt;
		uint64_t srcSize;
		uint64_t srcOffset;
		std::mutex ioLock;
		
		std::vector< std::pair<uint64_t, uint64_t> > pendingChunks;
		std::vector<InputBuffer> activeChunkBuffers;
	};
	
	Config cfg;
	State state;
	
	cfg.maxMemoryUsage = maxMemoryUsage;
	cfg.initialChunkSize = maxMemoryUsage/(threadCount*sizeof(value_type));
	cfg.comp = &comp;
	
	state.srcIt = begin;
	using std::distance;
	state.srcSize = distance(begin, end);
	state.srcOffset = 0;

	//now check if we really have to use out-of-memory sorting
	if (detail::oom::InMemorySort<TInputOutputIterator>::canSort && state.srcSize*sizeof(value_type) <= maxMemoryUsage) {
		if (TWithProgressInfo) {
			std::cout << "Using in-memory sorting..." << std::flush;
		}
		detail::oom::InMemorySort<TInputOutputIterator>::sort(begin, end, comp);
		if (TWithProgressInfo) {
			std::cout << "done" << std::endl;
		}
		return;
	}

	struct Worker {
		State * state;
		Config * cfg;
		std::vector<value_type> buffer;
		Worker(State * state, Config * cfg) : state(state), cfg(cfg) {}
		Worker(Worker && other) : state(other.state), cfg(other.cfg), buffer(std::move(other.buffer)) {}
		void operator()() {
			while (true) {
				std::unique_lock<std::mutex> ioLock(state->ioLock);
				if (state->srcOffset >= state->srcSize) {
					return; //we're done
				}
				uint64_t myChunkSize = std::min<uint64_t>(cfg->initialChunkSize, state->srcSize-state->srcOffset);
				
				//push the chunk 
				state->pendingChunks.emplace_back(state->srcOffset, state->srcOffset+myChunkSize);
				
				auto chunkBegin = state->srcIt;
				auto chunkEnd = chunkBegin+myChunkSize;
				state->srcOffset += myChunkSize;
				state->srcIt = chunkEnd;
				buffer.assign(chunkBegin, chunkEnd);
				
				ioLock.unlock();
				
				using std::sort;
				sort(buffer.begin(), buffer.end(), *(cfg->comp));
				
				ioLock.lock();
				//flush back
				using std::move;
				//capture return value if need be
				#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
				auto chunkIt =
				#endif
				
				move(buffer.begin(), buffer.end(), chunkBegin);
				
				SSERIALIZE_CHEAP_ASSERT(chunkIt == chunkEnd);
				#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
				{
					detail::oom::IteratorSyncer<TInputOutputIterator>::sync(chunkIt);
					using std::is_sorted;
					SSERIALIZE_EXPENSIVE_ASSERT(is_sorted(chunkIt, chunkEnd, *(cfg->comp)));
				}
				#endif
				
				state->sortCompleted += myChunkSize;
				state->pinfo(state->sortCompleted);
			}
		}
	};
	//spawn the threads
	{
		state.sortCompleted = 0;
		state.pinfo.begin(state.srcSize, "Sorting chunks");
		if (threadCount > 1) {
			std::vector<std::thread> ts;
			for(uint32_t i(0); i < threadCount; ++i) {
				ts.emplace_back(std::thread(Worker(&state, &cfg)));
			}
			for(std::thread & t : ts) {
				t.join();
			}
		}
		else {
			Worker w(&state, &cfg);
			w.operator()();
		}
		state.pinfo.end();
	}
	SSERIALIZE_CHEAP_ASSERT_EQUAL(state.srcOffset, state.srcSize);
	
	//now merge the chunks, use about 1/4 of memory for the temporary storage
	cfg.tmpBuffferSize = cfg.maxMemoryUsage/4;
	cfg.maxMemoryUsage -= cfg.tmpBuffferSize;
	sserialize::OOMArray<value_type> tmp(mmt);
	
	struct PrioComp {
		CompFunc * comp;
		std::vector<InputBuffer> * chunkBuffers;
		bool operator()(uint32_t a, uint32_t b) { return !( (*comp)(chunkBuffers->at(a).get(), chunkBuffers->at(b).get()) ); }
		PrioComp(CompFunc * comp, std::vector<InputBuffer> * chunkBuffers) : comp(comp), chunkBuffers(chunkBuffers) {}
		PrioComp(const PrioComp & other) : comp(other.comp), chunkBuffers(other.chunkBuffers) {}
	};
	
	typedef std::priority_queue<uint32_t, std::vector<uint32_t>, PrioComp> MyPrioQ;
	
	MyPrioQ pq(PrioComp(&comp, &(state.activeChunkBuffers)));
	
	for(uint32_t queueRound(0); state.pendingChunks.size() > 1; ++queueRound) {
		detail::oom::IteratorSyncer<TInputOutputIterator>::sync(begin);
		SrcIterator srcIt(begin);
		
		std::vector< std::pair<uint64_t, uint64_t> > nextRoundPendingChunks;
		state.srcOffset = 0;
		
		state.pinfo.begin(state.srcSize, std::string("Merging sorted chunks round ") + std::to_string(queueRound));
		for(uint32_t cbi(0), cbs(state.pendingChunks.size()); cbi < cbs; cbi += queueDepth) {
			SSERIALIZE_CHEAP_ASSERT(!tmp.size());
			
			//set the buffer sizes
			tmp.backBufferSize(cfg.tmpBuffferSize);
			tmp.readBufferSize(sizeof(value_type));
			
			{//fill the activeChunkBuffers and create the chunk for the next round
				SSERIALIZE_CHEAP_ASSERT(!state.activeChunkBuffers.size());
				SSERIALIZE_CHEAP_ASSERT_EQUAL(state.srcOffset, state.pendingChunks.at(cbi).first);
				uint32_t myQueueSize = std::min<uint32_t>(queueDepth, cbs-cbi);
				uint64_t chunkBufferSize = cfg.maxMemoryUsage/(myQueueSize);
				uint64_t resultChunkBeginOffset = state.srcOffset;
				uint64_t resultChunkEndOffset = state.pendingChunks.at(cbi+myQueueSize-1).second;
				for(uint32_t i(0); i < myQueueSize; ++i) {
					const std::pair<uint64_t, uint64_t> & pendingChunk = state.pendingChunks.at(cbi+i);
					SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(pendingChunk.second, state.srcSize);
					SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(pendingChunk.first, pendingChunk.second);
					state.activeChunkBuffers.emplace_back(begin+pendingChunk.first, begin+pendingChunk.second, chunkBufferSize);
					pq.push(i);
				}
				nextRoundPendingChunks.emplace_back(resultChunkBeginOffset, resultChunkEndOffset);
			}
			
			while (pq.size()) {
				uint32_t pqMin = pq.top();
				pq.pop();
				InputBuffer & chunkBuffer = state.activeChunkBuffers[pqMin];
				value_type & v = chunkBuffer.get();
				tmp.emplace_back(std::move(v));
				if (chunkBuffer.next()) {
					pq.push(pqMin);
				}
				if (tmp.size() % 1000 == 0) {
					state.pinfo(state.srcOffset+tmp.size());
				}
				SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmp.size(), state.srcSize);
			}
			state.pinfo(state.srcOffset+tmp.size());
			
			//flush tmp and set a larger read buffer
			tmp.flush();
			tmp.backBufferSize(0);
			tmp.readBufferSize(cfg.tmpBuffferSize/sizeof(value_type));
			
			///move back this part to source
			using std::move;
			srcIt = move(tmp.begin(), tmp.end(), srcIt);
			state.srcOffset += tmp.size();
			tmp.clear();
			state.activeChunkBuffers.clear();;
		}
		SSERIALIZE_CHEAP_ASSERT(srcIt == end);
		SSERIALIZE_CHEAP_ASSERT_EQUAL(state.srcOffset, state.srcSize);
		state.pinfo.end();
		using std::swap;
		swap(state.pendingChunks, nextRoundPendingChunks);
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(state.srcSize, tmp.size());
	}
}

///iterators need to point to sorted range
///Has one additional copy to begin
///Total memory size is then about 3*sizeof(TInputOutputIterator)+maxMemoryUsage+c
template<typename TInputOutputIterator, typename TEqual = std::equal_to< typename std::iterator_traits<TInputOutputIterator>::value_type >, bool TWithProgressInfo = true>
TInputOutputIterator oom_unique(TInputOutputIterator begin, TInputOutputIterator end, sserialize::MmappedMemoryType mmt = sserialize::MM_FILEBASED, uint64_t maxMemoryUsage = 100*1024*1024, TEqual eq = TEqual()) {
	if (begin == end) {
		return begin;
	}
	typedef typename std::iterator_traits<TInputOutputIterator>::value_type value_type;
	
	using std::distance;
	uint64_t srcSize = distance(begin, end);
	
	sserialize::OOMArray<value_type> tmp(mmt);
	tmp.backBufferSize(maxMemoryUsage);
	tmp.reserve(srcSize);
	
	OptionalProgressInfo<TWithProgressInfo> pinfo;
	pinfo.begin(srcSize, "OOMUnique");
	
	TInputOutputIterator it(begin);
	uint64_t count = 0;
	value_type prev = *it;
	tmp.emplace_back(std::move(*it));
	for(++it; it != end; ++it, ++count) {
		if (!eq(*it, prev)) {
			prev = *it;
			tmp.emplace_back(std::move(*it));
		}
		pinfo(count);
	}
	pinfo.end();
	
	tmp.flush();
	
	SSERIALIZE_CHEAP_ASSERT_MESSAGE(srcSize >= tmp.size(), "tmp.size() > srcSize");
	
	//set larger read buffers
	tmp.backBufferSize(sizeof(value_type));
	tmp.readBufferSize(maxMemoryUsage);
	
	//move back
	using std::move;
	return move(tmp.begin(), tmp.end(), begin);
}

}//end namespace

#endif