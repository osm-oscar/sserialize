#ifndef SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#define SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#include <functional>
#include <type_traits>
#include <iterator>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <sserialize/containers/OOMArray.h>
#include <sserialize/algorithm/utilcontainerfuncs.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/utility/assert.h>

namespace sserialize {
namespace detail {
namespace oom {

template<typename TSourceIterator, typename TValue = typename std::iterator_traits<TSourceIterator>::value_type>
class InputBuffer {
public:
	typedef typename std::vector<TValue>::iterator iterator;
	typedef TValue value_type;
private:
	TSourceIterator m_srcIt;
	SizeType m_srcPos;
	SizeType m_srcSize;
	//in number of entries
	SizeType m_bufferSize;
	std::vector<TValue> m_buffer;
	iterator m_bufferIt;
private:
	void fillBuffer() {
		m_buffer.clear();
		SizeType copyAmount(0);
		for(; copyAmount < m_bufferSize && m_srcPos < m_srcSize; ++copyAmount, ++m_srcPos, ++m_srcIt) {
			m_buffer.push_back(*m_srcIt);
		}
		m_bufferIt = m_buffer.begin();
	}
public:
	InputBuffer() : m_bufferSize(0), m_bufferIt(m_buffer.begin()) {}
	InputBuffer(const TSourceIterator & srcBegin, const TSourceIterator & srcEnd, SizeType bufferSize) :
	InputBuffer(srcBegin, std::distance(srcBegin, srcEnd), bufferSize)
	{
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(0, std::distance(srcBegin, srcEnd));
	}
	///@param bufferSize bufferSize in bytes
	InputBuffer(const TSourceIterator & srcBegin, SizeType srcSize, SizeType bufferSize) :
	m_srcIt(srcBegin),
	m_srcPos(0),
	m_srcSize(srcSize),
	m_bufferSize(bufferSize/sizeof(value_type))
	{
		m_buffer.reserve(m_bufferSize);
		fillBuffer();
	}
	InputBuffer(InputBuffer && other) :
	m_srcIt(std::move(other.m_srcIt)),
	m_srcPos(other.m_srcPos),
	m_srcSize(other.m_srcSize),
	m_bufferSize(other.m_bufferSize)
	{
		std::size_t bufferItOffset = other.m_bufferIt - other.m_buffer.begin();
		m_buffer = std::move(other.m_buffer);
		m_bufferIt = m_buffer.begin()+bufferItOffset;
	}
	InputBuffer & operator=(InputBuffer && other) {
		m_srcIt = std::move(other.m_srcIt);
		m_srcPos = other.m_srcPos;
		m_srcSize = other.m_srcSize;
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
	template<typename TEqual>
	inline static void uniqe(const TIterator &/*begin*/, const TIterator &/*end*/, const TEqual &/*equal*/) {}
};

template<typename TIterator>
struct InMemorySort<TIterator, std::random_access_iterator_tag> {
	static constexpr bool canSort = true;
	template<typename TCompare>
	inline static void sort(const TIterator & begin, const TIterator & end, const TCompare & compare) {
		std::sort(begin, end, compare);
	}
	template<typename TEqual>
	inline static TIterator unique(const TIterator & begin, const TIterator & end, const TEqual & equal) {
		return std::unique(begin, end, equal);
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


//This class replaces the content and range of the src iterator of oom_sort with content from temporary storage
//This is mainly usefull for OOMArray iterators to perform a data swap operation
template<typename TIterator>
struct IteratorRangeDataSwaper {
	using value_type = typename std::iterator_traits<TIterator>::value_type;
	using SrcIterator = TIterator;
	using TmpIterator = typename sserialize::OOMArray<value_type>::iterator;
	static void swap(const TmpIterator & tmpBegin, const TmpIterator & tmpEnd, SrcIterator & begin, SrcIterator & /*end*/) {
		using std::move;
		move(tmpBegin, tmpEnd, begin);
	}
};

template<typename TValue>
struct IteratorRangeDataSwaper< sserialize::detail::OOMArray::Iterator<TValue> > {
	using value_type = TValue;
	using container_type = sserialize::OOMArray<value_type>;
	using SrcIterator = typename container_type::iterator;
	using TmpIterator = typename container_type::iterator;
	static void swap(const TmpIterator & tmpBegin, const TmpIterator & tmpEnd, SrcIterator & begin, SrcIterator & /*end*/) {
		container_type * tmpd = const_cast<container_type*>(container_type::backend(tmpBegin));
		container_type * srcd = const_cast<container_type*>(container_type::backend(begin));
		
		if (tmpd->mmt() == srcd->mmt()) {
			tmpd->swap_data(*srcd);
		}
		else {
			using std::move;
			move(tmpBegin, tmpEnd, begin);
		}
	}
};

}}//end namespace detail::oom

//TODO: specialise for OOMArray::iterator (special write-back, input buffer size needs to be taken into account)

///A standard out-of-memory sorting algorithm. It first sorts the input in chunks of size maxMemoryUsage/threadCount
///These chunks are then merged together in possibly multiple phases. In a single phase up to queueDepth chunks are merged together.
///@param maxMemoryUsage default is 4 GB
///@param threadCount should be no larger than about 4, 2 should be sufficient for standard io-speeds, used fo the initial chunk sorting, a single chunk then has a size of maxMemoryUsage/threadCount
///@param queueDepth the maximum number of chunks to merge in a single round, this directly influences the number of merge rounds
///@param comp comparisson operator for strict weak order. This functions needs to be thread-safe <=> threadCount > 1
///@param equal equality operator, only used if TUniquify is true. This functions needs to be thread-safe <=> threadCount > 1
///@return points to the last element of the sorted sequence
///In general: Larger chunks result in a smaller number of rounds and can be processed with a smaller queue depth reducing random access
///Thus for very large data sizes it may be better to use only one thread to create the largest chunks possible
template<
	typename TInputOutputIterator,
	typename CompFunc = std::less<typename std::iterator_traits<TInputOutputIterator>::value_type>,
	bool TWithProgressInfo = true,
	bool TUniquify = false,
	typename EqualFunc = std::equal_to<typename std::iterator_traits<TInputOutputIterator>::value_type>
>
TInputOutputIterator oom_sort(TInputOutputIterator begin, TInputOutputIterator end, CompFunc comp = CompFunc(),
				uint64_t maxMemoryUsage = 0x100000000,
				uint32_t maxThreadCount = 2,
				sserialize::MmappedMemoryType mmt = sserialize::MM_FILEBASED,
				uint32_t queueDepth = 64,
				uint32_t maxWait = 10,
				EqualFunc equal = EqualFunc())
{
	typedef TInputOutputIterator SrcIterator;
	typedef typename std::iterator_traits<SrcIterator>::value_type value_type;
	typedef detail::oom::InputBuffer<SrcIterator> InputBuffer;
// 	using std::next;
	
	
	SSERIALIZE_CHEAP_ASSERT(begin < end);
	
	if (!maxThreadCount) {
		maxThreadCount = std::thread::hardware_concurrency();
	}
	
	constexpr uint64_t INVALID_CHUNK_OFFSET = std::numeric_limits<uint64_t>::max();
	struct ChunkDescription {
		uint64_t first;
		uint64_t second;
		ChunkDescription() : first(INVALID_CHUNK_OFFSET), second(INVALID_CHUNK_OFFSET) {}
		ChunkDescription(uint64_t _first, uint64_t _second) : first(_first), second(_second)
		{
			SSERIALIZE_CHEAP_ASSERT(valid());
		}
		ChunkDescription(const ChunkDescription & other) = default;
		ChunkDescription & operator=(const ChunkDescription & other) = default;
		bool valid() const { return first != INVALID_CHUNK_OFFSET && second != INVALID_CHUNK_OFFSET && first < second; }
		uint64_t size() const {
			SSERIALIZE_CHEAP_ASSERT(valid());
			return second-first;
		}
	};
	
	struct Config {
		uint32_t maxThreadCount;
		//maximum time a thread wait before it remove itself from processing
		uint32_t maxWait;
		//chunk size in number of entries
		uint64_t initialChunkSize;
		//max memory size in bytes
		uint64_t maxMemoryUsage;
		//tmp buffer size in bytes
		uint64_t tmpBuffferSize;
		CompFunc * comp;
		EqualFunc * equal;
	};
	
	struct WorkerBalanceInfo {
		WorkerBalanceInfo() : pausedWorkers(0), wantExtraWorker(false) {}
		uint32_t pausedWorkers;
		bool wantExtraWorker;
		std::mutex mtx;
		std::condition_variable  cv;
	};
	
	struct State {
		OptionalProgressInfo<TWithProgressInfo> pinfo;
		uint64_t sortCompleted;
		SrcIterator srcIt;
		uint64_t srcSize;
		uint64_t srcOffset;
		///for TUniquify==false -> resultSize = srcSize
		uint64_t resultSize;
		std::mutex ioLock;
		
		//if TUniquify is true, then these are NOT necessarily contiguous, BUT they are always sorted in ascending order
		std::vector< ChunkDescription > pendingChunks;
		std::vector<InputBuffer> activeChunkBuffers;
	};
	
	Config cfg;
	State state;
	WorkerBalanceInfo wbi;
	
	cfg.maxThreadCount = maxThreadCount;
	cfg.maxWait = maxWait;
	cfg.maxMemoryUsage = maxMemoryUsage;
	cfg.initialChunkSize = maxMemoryUsage/(maxThreadCount*sizeof(value_type));
	cfg.comp = &comp;
	cfg.equal = &equal;
	
	state.srcIt = begin;
	using std::distance;
	state.srcSize = distance(begin, end);
	state.srcOffset = 0;
	state.resultSize = 0;

	//now check if we really have to use out-of-memory sorting
	if (detail::oom::InMemorySort<TInputOutputIterator>::canSort && state.srcSize*sizeof(value_type) <= maxMemoryUsage) {
		if (TWithProgressInfo) {
			std::cout << "Using in-memory sorting..." << std::flush;
		}
		detail::oom::InMemorySort<TInputOutputIterator>::sort(begin, end, comp);
		if (TUniquify) {
			using std::unique;
			return unique(begin, end, equal);
		}
		if (TWithProgressInfo) {
			std::cout << "done" << std::endl;
		}
		return end;
	}

	struct Worker {
		State * state;
		Config * cfg;
		WorkerBalanceInfo * wbi;
		std::vector<value_type> buffer;
		uint64_t currentChunkSize;
		std::size_t currentChunkDescriptionPos;
		Worker(State * state, Config * cfg, WorkerBalanceInfo * wbi) :
		state(state), cfg(cfg), wbi(wbi), currentChunkSize(0), currentChunkDescriptionPos(0)
		{}
		Worker(Worker && other) :
		state(other.state), cfg(other.cfg), wbi(other.wbi),
		buffer(std::move(other.buffer)),
		currentChunkSize(other.currentChunkSize),
		currentChunkDescriptionPos(other.currentChunkDescriptionPos)
		{}
		void operator()() {
			sserialize::TimeMeasurer tm;
			while (true) {
				uint64_t lockTime = 0;
				tm.begin();
				std::unique_lock<std::mutex> ioLock(state->ioLock);
				tm.end();
				lockTime = tm.elapsedSeconds();
				
				if (state->srcOffset >= state->srcSize) {
					ioLock.unlock();
					std::unique_lock<std::mutex> wbiLck(wbi->mtx);
					if (wbi->pausedWorkers) {
						wbiLck.unlock();
						wbi->cv.notify_all();
					}
					return; //we're done
				}
				currentChunkSize = std::min<uint64_t>(cfg->initialChunkSize, state->srcSize-state->srcOffset);
				
				//push the beginning of our chunk
				currentChunkDescriptionPos = state->pendingChunks.size();
				state->pendingChunks.emplace_back(state->srcOffset, INVALID_CHUNK_OFFSET);
				
				auto chunkBegin = state->srcIt;
				auto chunkEnd = chunkBegin+currentChunkSize;
				state->srcOffset += currentChunkSize;
				state->srcIt = chunkEnd;
				buffer.assign(chunkBegin, chunkEnd);
				
				ioLock.unlock();
				
				//check if we want to activate another thread
				if (lockTime < 1) {
					std::unique_lock<std::mutex> wbiLck(wbi->mtx);
					if (wbi->pausedWorkers) {
						wbi->wantExtraWorker = true;
						wbiLck.unlock();
						wbi->cv.notify_one();
					}
				}
				
				using std::sort;
				sort(buffer.begin(), buffer.end(), *(cfg->comp));
				
				if (TUniquify) {
					using std::unique;
					auto lastUnique = unique(buffer.begin(), buffer.end(), *(cfg->equal));
					buffer.resize(std::distance(buffer.begin(), lastUnique));
				}
				
				tm.begin();
				ioLock.lock();
				tm.end();
				
				lockTime += tm.elapsedSeconds();
				
				//correct our ChunkDescription
				ChunkDescription & cd = state->pendingChunks.at(currentChunkDescriptionPos);
				cd.second = cd.first + buffer.size();
				
				//and the result size
				state->resultSize += buffer.size();
				
				//flush back
				using std::move;
				move(buffer.begin(), buffer.end(), chunkBegin);
				
				state->sortCompleted += currentChunkSize;
				state->pinfo(state->sortCompleted);
				
				ioLock.unlock();
				
				//check if we need to pause processing
				if (lockTime > cfg->maxWait) {
					std::unique_lock<std::mutex> wbiLck(wbi->mtx);
					if (state->srcOffset >= state->srcSize) {
						continue;
					}
					if (cfg->maxThreadCount-wbi->pausedWorkers > 2) {
						wbi->pausedWorkers += 1;
						while (true) {
							wbi->cv.wait(wbiLck);
							if (wbi->wantExtraWorker || state->srcOffset >= state->srcSize) {
								wbi->pausedWorkers -= 1;
								break;
							}
						}
					}
				}
			}
		}
	};
	//spawn the threads
	{
		state.sortCompleted = 0;
		state.pinfo.begin(state.srcSize, "Sorting chunks");
		if (maxThreadCount > 1) {
			std::vector<std::thread> ts;
			for(uint32_t i(0); i < maxThreadCount; ++i) {
				ts.emplace_back(std::thread(Worker(&state, &cfg, &wbi)));
			}
			for(std::thread & t : ts) {
				t.join();
			}
		}
		else {
			Worker w(&state, &cfg, &wbi);
			w.operator()();
		}
		state.pinfo.end();
	}
	SSERIALIZE_CHEAP_ASSERT_EQUAL(state.srcOffset, state.srcSize);
	if (TUniquify) {
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(state.resultSize, state.srcSize);
	}
	else {
		SSERIALIZE_CHEAP_ASSERT_EQUAL(state.resultSize, state.srcSize);
	}
	
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
		state.pinfo.begin(state.resultSize, std::string("Merging sorted chunks round ") + std::to_string(queueRound));
		
		std::vector<ChunkDescription> nextRoundPendingChunks;
		
		detail::oom::IteratorSyncer<TInputOutputIterator>::sync(begin);
		
		//setup temporary store
		tmp.clear();
		tmp.reserve(state.resultSize);
		tmp.backBufferSize(cfg.tmpBuffferSize);
		tmp.readBufferSize(sizeof(value_type));
		
		for(uint32_t cbi(0), cbs((uint32_t)state.pendingChunks.size()); cbi < cbs;) {
			{//fill the activeChunkBuffers
				SSERIALIZE_CHEAP_ASSERT(!state.activeChunkBuffers.size());
				uint64_t chunkBufferSize = cfg.maxMemoryUsage/std::min<uint32_t>(queueDepth, cbs-cbi);
				for(; cbi < cbs && state.activeChunkBuffers.size() < queueDepth; ++cbi) {
					const ChunkDescription & pendingChunk = state.pendingChunks.at(cbi);
					if (!TUniquify || pendingChunk.size()) {
						state.activeChunkBuffers.emplace_back(begin+pendingChunk.first, pendingChunk.size(), chunkBufferSize);
						pq.push(state.activeChunkBuffers.size()-1);
					}
				}
			}
			
			//add our result chunk to the queue (we don't know the size yet, just the beginning)
			nextRoundPendingChunks.emplace_back(tmp.size(), INVALID_CHUNK_OFFSET);
			
			while (pq.size()) {
				uint32_t pqMin = pq.top();
				pq.pop();
				InputBuffer & chunkBuffer = state.activeChunkBuffers[pqMin];
				value_type & v = chunkBuffer.get();
				if (!TUniquify || !tmp.size() || !equal(tmp.back(), v)) {
					tmp.emplace_back(std::move(v));
				}
				if (chunkBuffer.next()) {
					pq.push(pqMin);
				}
				if (tmp.size() % 1000 == 0) {
					state.pinfo(tmp.size());
				}
				SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmp.size(), state.srcSize);
			}
			state.pinfo(tmp.size());
			
			if (!TUniquify || nextRoundPendingChunks.back().first != tmp.size()) {
				nextRoundPendingChunks.back().second = tmp.size();
			}
			else {
				//nothing has been added, remove our chunk from the queue 
				//this never happens if TUniquify is false,
				//since then all elements of the chunks are appended to tmp
				nextRoundPendingChunks.pop_back();
			}
			state.activeChunkBuffers.clear();
		}
		
		if (TUniquify) {
			SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmp.size(), state.srcSize);
		}
		else {
			SSERIALIZE_CHEAP_ASSERT_EQUAL(tmp.size(), state.srcSize);
		}
		
		//copy back to source
		tmp.flush();
		tmp.backBufferSize(0);
		tmp.readBufferSize(cfg.tmpBuffferSize/sizeof(value_type));
		
		state.resultSize = tmp.size();
		
		///move back to source
		using std::move;
		detail::oom::IteratorRangeDataSwaper<SrcIterator>::swap(tmp.begin(), tmp.end(), begin, end);
		tmp.clear();

		state.pinfo.end();
		using std::swap;
		swap(state.pendingChunks, nextRoundPendingChunks);
	}
	
	#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	{
		using std::is_sorted;
		using std::is_unique;
		if (TUniquify) {
			
		}
	}
	#endif
	
	#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
	if (TUniquify) {
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(state.resultSize, state.srcSize);
	}
	else {
		SSERIALIZE_CHEAP_ASSERT_EQUAL(state.resultSize, state.srcSize);
	}
	#endif
	
	return begin+state.resultSize;
}


///this assumes that there is enough storage space to accomodate twice the data of container
template<typename TValue, typename TInputOutputIterator, typename TEqual = std::equal_to<TValue>, bool TWithProgressInfo = true>
void oom_unique(sserialize::OOMArray<TValue> & src, uint64_t maxMemoryUsage = 100*1024*1024, TEqual eq = TEqual()) {
	using value_type = TValue;
	
	uint64_t srcSize = src.size();
	sserialize::OOMArray<value_type> tmp(srcSize);
	tmp.backBufferSize(maxMemoryUsage);
	tmp.reserve(srcSize);
	
	OptionalProgressInfo<TWithProgressInfo> pinfo;
	pinfo.begin(srcSize, "OOMUnique");
	
	TInputOutputIterator it(src.begin());
	TInputOutputIterator end(src.end());
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
	
	SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(srcSize, tmp.size());
	
	tmp.swap_data(src);
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
	
	SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(srcSize, tmp.size());
	
	//set larger read buffers
	tmp.backBufferSize(sizeof(value_type));
	tmp.readBufferSize(maxMemoryUsage/2);
	
	//move back
	using std::move;
	return move(tmp.begin(), tmp.end(), begin);
}

}//end namespace

#endif