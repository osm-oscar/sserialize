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

//default sort traits
template<
	bool TWithProgressInfo,
	typename TCompare,
	typename TEqual
>
class SortTraits {
public:
	static constexpr bool withProgressInfo = TWithProgressInfo;
	using Compare = TCompare;
	using Equal = TEqual;
	using Self = SortTraits<withProgressInfo, Compare, Equal>;
private: //need to come first in order to access them with decltype
	Compare m_compare;
	Equal m_equal;
	bool m_makeUnique{false};
	bool m_ioFetchLock{true};
	bool m_ioFlushLock{true};
	uint64_t m_maxMemoryUsage{0x100000000};
	uint32_t m_maxThreadCount{2};
	sserialize::MmappedMemoryType m_mmt{sserialize::MM_FILEBASED};
	uint32_t m_queueDepth{64};
	uint32_t m_maxWait{10};
public:
	SortTraits(Compare c = Compare(), Equal e = Equal()) : m_compare(c), m_equal(e) {}
	~SortTraits() {}
#define SSERIALIZE_OOM_SORT_TRAITS_GET_SET(__NAME) \
	Self & __NAME(decltype(Self::m_##__NAME) v) { m_##__NAME = v; return *this;} \
	auto __NAME() const { return m_##__NAME;}
	
	SSERIALIZE_OOM_SORT_TRAITS_GET_SET(makeUnique)
	SSERIALIZE_OOM_SORT_TRAITS_GET_SET(ioFetchLock)
	SSERIALIZE_OOM_SORT_TRAITS_GET_SET(ioFlushLock)
	//max memory size in bytes
	SSERIALIZE_OOM_SORT_TRAITS_GET_SET(maxMemoryUsage)
	SSERIALIZE_OOM_SORT_TRAITS_GET_SET(maxThreadCount)
	SSERIALIZE_OOM_SORT_TRAITS_GET_SET(mmt)
	SSERIALIZE_OOM_SORT_TRAITS_GET_SET(queueDepth)
	//maximum time a thread waits before it (temporarily) removes itself from processing
	SSERIALIZE_OOM_SORT_TRAITS_GET_SET(maxWait)
#undef SSERIALIZE_OOM_SORT_TRAITS_GET_SET
public:
	Compare compare() const { return m_compare; }
	Equal equal() const { return m_equal; }
};

}}//end namespace detail::oom



///A standard out-of-memory sorting algorithm. It first sorts the input in chunks of size maxMemoryUsage/threadCount
///These chunks are then merged together in possibly multiple phases. In a single phase up to queueDepth chunks are merged together.
///@param maxMemoryUsage default is 4 GB
///@param threadCount should be no larger than about 4, 2 should be sufficient for standard io-speeds, used fo the initial chunk sorting, a single chunk then has a size of maxMemoryUsage/threadCount
///@param queueDepth the maximum number of chunks to merge in a single round, this directly influences the number of merge rounds
///@param comp comparisson operator. This functions needs to be thread-safe <=> threadCount > 1
///@param equal equality operator, only used if TUniquify is true. This functions needs to be thread-safe <=> threadCount > 1
///@return points to the last element of the sorted sequence
///In general: Larger chunks result in a smaller number of rounds and can be processed with a smaller queue depth reducing random access
///Thus for very large data sizes it may be better to use only one thread to create the largest chunks possible
///TODO: use mt_sort for large chunks if threadCount == 1 or add ability to define a sort operator
template<
	typename TInputOutputIterator,
	typename Traits =
		detail::oom::SortTraits<
			true,
			std::less<typename std::iterator_traits<TInputOutputIterator>::value_type>,
			std::equal_to<typename std::iterator_traits<TInputOutputIterator>::value_type>
		>
>
TInputOutputIterator oom_sort(TInputOutputIterator begin, TInputOutputIterator end, Traits traits = Traits())
{
	typedef TInputOutputIterator SrcIterator;
	typedef typename std::iterator_traits<SrcIterator>::value_type value_type;
	typedef detail::oom::InputBuffer<SrcIterator> InputBuffer;
// 	using std::next;
	
	
	SSERIALIZE_CHEAP_ASSERT(begin < end);
	
	if (!traits.maxThreadCount()) {
		throw sserialize::ConfigurationException("sserialize::oom::oom_sort","maxThreadCount == 0");
	}
	
	static constexpr uint64_t INVALID_CHUNK_OFFSET = std::numeric_limits<uint64_t>::max();
	struct ChunkDescription {
		uint64_t first;
		uint64_t second;
		ChunkDescription() : first(INVALID_CHUNK_OFFSET), second(INVALID_CHUNK_OFFSET) {}
		ChunkDescription(uint64_t _first) : first(_first), second(INVALID_CHUNK_OFFSET) {}
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
		Traits & traits;
		//chunk size in number of entries
		uint64_t initialChunkSize;
		//tmp buffer size in bytes
		uint64_t tmpBuffferSize;
		//merge buffer size in number of entries
		uint64_t mergeBufferEntries;
		Config(Traits & traits) :
			traits(traits),
			initialChunkSize(this->traits.maxMemoryUsage()/(traits.maxThreadCount()*sizeof(value_type)))
		{}
	};
	
	struct WorkerBalanceInfo {
		WorkerBalanceInfo() : pausedWorkers(0), wantExtraWorker(false) {}
		uint32_t pausedWorkers;
		bool wantExtraWorker;
		std::mutex mtx;
		std::condition_variable  cv;
	};
	
	struct State {
		OptionalProgressInfo<Traits::withProgressInfo> pinfo;
		std::atomic<uint64_t> sortCompleted;
		SrcIterator srcIt;
		uint64_t srcSize;
		uint64_t srcOffset;
		///for TUniquify==false -> resultSize = srcSize
		uint64_t resultSize;
		std::mutex ioLock;

		//if TUniquify is true, then these are NOT necessarily contiguous, BUT they are always sorted in ascending order
		std::vector< ChunkDescription > pendingChunks;
		std::vector<InputBuffer> activeChunkBuffers;
		std::vector<value_type> activeChunkBufferValues;
	};
	
	Config cfg(traits);
	State state;
	WorkerBalanceInfo wbi;
	
	state.srcIt = begin;
	using std::distance;
	state.srcSize = distance(begin, end);
	state.srcOffset = 0;
	state.resultSize = 0;

	//now check if we really have to use out-of-memory sorting
	if (detail::oom::InMemorySort<TInputOutputIterator>::canSort && state.srcSize*sizeof(value_type) <= traits.maxMemoryUsage()) {
		if (Traits::withProgressInfo) {
			std::cout << "Using in-memory sorting..." << std::flush;
		}
		detail::oom::InMemorySort<TInputOutputIterator>::sort(begin, end, traits.compare());
		if (traits.makeUnique()) {
			using std::unique;
			return unique(begin, end, traits.equal());
		}
		if (Traits::withProgressInfo) {
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
				state->pendingChunks.emplace_back(state->srcOffset);
				
				auto chunkBegin = state->srcIt;
				auto chunkEnd = chunkBegin+currentChunkSize;
				state->srcOffset += currentChunkSize;
				state->srcIt = chunkEnd;
				
				if (!cfg->traits.ioFetchLock()) {
					ioLock.unlock();
				}
				
				buffer.assign(chunkBegin, chunkEnd);
				
				if (cfg->traits.ioFetchLock()) {
					ioLock.unlock();
				}
				
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
				sort(buffer.begin(), buffer.end(), cfg->traits.compare());
				
				if (cfg->traits.makeUnique()) {
					using std::unique;
					auto lastUnique = unique(buffer.begin(), buffer.end(), cfg->traits.equal());
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
				
				if (!cfg->traits.ioFlushLock()) {
					ioLock.unlock();
				}
				
				//flush back
				using std::move;
				move(buffer.begin(), buffer.end(), chunkBegin);
				
				if (cfg->traits.ioFlushLock()) {
					ioLock.unlock();
				}
				
				state->sortCompleted += currentChunkSize;
				state->pinfo(state->sortCompleted);
				
				//check if we need to pause processing
				if (lockTime > cfg->traits.maxWait()) {
					std::unique_lock<std::mutex> wbiLck(wbi->mtx);
					if (state->srcOffset >= state->srcSize) {
						continue;
					}
					if (cfg->traits.maxThreadCount()-wbi->pausedWorkers > 2) {
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
		if (traits.maxThreadCount() > 1) {
			std::vector<std::thread> ts;
			for(uint32_t i(0), s(traits.maxThreadCount()); i < s; ++i) {
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
	if (traits.makeUnique()) {
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(state.resultSize, state.srcSize);
	}
	else {
		SSERIALIZE_CHEAP_ASSERT_EQUAL(state.resultSize, state.srcSize);
	}
	
	//now merge the chunks, use at most 1/4 or 1 GiB  of memory for the temporary storage
	//Using more than 1 GiB will likely reduce the write performance since then we are writing very large chunks while the queue is full
	cfg.tmpBuffferSize = std::min<uint64_t>(1024*1024*1024, cfg.traits.maxMemoryUsage()/4);
	if (traits.maxThreadCount() > 1) {
		cfg.mergeBufferEntries = std::max<uint64_t>(128, cfg.traits.maxMemoryUsage()/(4*sizeof(value_type)*(traits.maxThreadCount()-1)));
	}
	sserialize::OOMArray<value_type> tmp(traits.mmt());
	
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
				SSERIALIZE_CHEAP_ASSERT(!state.activeChunkBufferValues.size());
				uint64_t chunkBufferSize = traits.maxMemoryUsage();
				chunkBufferSize -= cfg.tmpBuffferSize;
				chunkBufferSize -= sizeof(value_type)*cfg.mergeBufferEntries*(traits.maxThreadCount()-1);
				chunkBufferSize /= std::min<uint32_t>(traits.queueDepth(), cbs-cbi);
				for(; cbi < cbs && state.activeChunkBuffers.size() < traits.queueDepth(); ++cbi) {
					const ChunkDescription & pendingChunk = state.pendingChunks.at(cbi);
					if (!traits.makeUnique() || pendingChunk.size()) {
						state.activeChunkBuffers.emplace_back(begin+pendingChunk.first, pendingChunk.size(), chunkBufferSize);
						state.activeChunkBufferValues.emplace_back(state.activeChunkBuffers.back().get());
					}
				}
			}
			
			//add our result chunk to the queue (we don't know the size yet, just the beginning)
			nextRoundPendingChunks.emplace_back(tmp.size());
			
			struct PrioComp {
				typename Traits::Compare comp;
				std::vector<value_type> * chunkBufferValues;
				bool operator()(uint32_t a, uint32_t b) {
					const value_type & av = (*chunkBufferValues)[a];
					const value_type & bv = (*chunkBufferValues)[b];
					return !( comp(av, bv) );
				}
				PrioComp(typename Traits::Compare comp, std::vector<value_type> * chunkBufferValues) : comp(comp), chunkBufferValues(chunkBufferValues) {}
				PrioComp(const PrioComp & other) : comp(other.comp), chunkBufferValues(other.chunkBufferValues) {}
			};
			
			typedef std::priority_queue<uint32_t, std::vector<uint32_t>, PrioComp> MyPrioQ;
			
			if (traits.maxThreadCount() > 1) {
				//use 1/4 of max memory for all buffers, but at least 128 entries per buffer
				//We have a single fetch and multiple push threads
				class PreQueue {
				public:
					PreQueue(Config * cfg, State * state, uint32_t chunkBegin, uint32_t chunkEnd) : 
					m_cfg(cfg),
					m_state(state),
					m_queue(PrioComp(m_cfg->traits.compare(), &(m_state->activeChunkBufferValues))),
					m_buffer(m_cfg->mergeBufferEntries)
					{
						for(; chunkBegin != chunkEnd; ++chunkBegin) {
							m_queue.push(chunkBegin);
						}
						m_lock.clear();
					}
					PreQueue(PreQueue const &) = delete;
					PreQueue(PreQueue && other) :
					m_cfg(other.m_cfg),
					m_state(other.m_state),
					m_queue(std::move(other.m_queue)),
					m_buffer(std::move(other.m_buffer)),
					m_begin(other.m_begin.load()),
					m_end(other.m_end.load()),
					m_eof(other.m_eof.load())
					{
						m_lock.clear();
					}
					PreQueue & operator=(PreQueue &&) = delete;
					PreQueue & operator=(PreQueue const &) = delete;
				public:
					bool eof() const { return m_begin.load(std::memory_order_consume) == m_end.load(std::memory_order_acquire) && m_eof.load(std::memory_order_acquire); }
					///Since we don't use locking only a single consumer is allowed
					///if this returns true, then result holds a new value
					///Otherwise no new value is available yet
					///Check eof() whether new values may be produced
					bool pop(value_type & result) {
						//Consumer increases the begin pointer
						//thus we have to make sure that loads of m_begin are ordered within the consumer thread)
						//Producers increase the end pointer
						//thus we have to make sure that all writes to the end have finished with their respective data writes
						if (m_begin.load(std::memory_order_consume) != m_end.load(std::memory_order_acquire)) {
							uint32_t myBegin = m_begin.load(std::memory_order_acquire);
							result = std::move(m_buffer[myBegin]);
							m_begin.store((myBegin+1) % m_cfg->mergeBufferEntries, std::memory_order_release);
							return true;
						}
						return false;
					}
					void run() {
						//try locking
						if (m_lock.test_and_set(std::memory_order_acquire)) {
							return;
						}
						//Fetch thread increases begin. We keep a sentinel value between end and begin (and empty entry)
						//We can simply check whether the next entry modulo the buffer size is the begin pointer. The result is that m_end is still a one-passed-the-end iterator.
						//In the following load only begin may increase.
						//if it happens that m_begin == m_end (the buffer is empty) we can safely advance m_end
						//However if m_end points to the element before begin (the sentinel value) then we cannot fetch another value and have to wait for the fetch thread to advance the begin pointer
						//changes to end from other threads have to show up here
						//Note that we are now the only thread changing value in this queue
						uint32_t end = m_end.load(std::memory_order_acquire);
						while (m_queue.size() && m_begin.load(std::memory_order_acquire) != ((end+1)%m_cfg->mergeBufferEntries)) {
							uint32_t pqMin = m_queue.top();
							m_queue.pop();
							InputBuffer & chunkBuffer = m_state->activeChunkBuffers[pqMin];
							value_type & v = m_state->activeChunkBufferValues[pqMin];
							m_buffer[end] = std::move(v);
							end = (end+1)%m_cfg->mergeBufferEntries;
							m_end.store(end, std::memory_order_release);
							if (chunkBuffer.next()) {
								m_state->activeChunkBufferValues[pqMin] = chunkBuffer.get();
								m_queue.push(pqMin);
							}
						}
						m_eof.store(!m_queue.size(), std::memory_order_release);
						m_lock.clear(std::memory_order_release);
					}
				private:
					Config * m_cfg;
					State * m_state;
					MyPrioQ m_queue;
					std::vector<value_type> m_buffer;
					std::atomic_flag m_lock;
					std::atomic<uint32_t> m_begin{0};
					std::atomic<uint32_t> m_end{0};
					std::atomic<bool> m_eof{false};
				};
				
				std::vector<PreQueue> queues;
				std::vector<value_type> queueValues;
				std::vector<std::thread> queueWorkers;
				{
					std::size_t chunksPerQueue = state.activeChunkBuffers.size()/(traits.maxThreadCount()-1)+std::size_t(state.activeChunkBuffers.size()%(traits.maxThreadCount()-1) > 0);
					for(std::size_t i(0); i < state.activeChunkBuffers.size(); i += chunksPerQueue) {
						queues.emplace_back(
								&cfg,
								&state,
								uint32_t(i),
								std::min<uint32_t>(state.activeChunkBuffers.size(), i+chunksPerQueue)
						);
					}
				}
				for(std::size_t i(0), s(traits.maxThreadCount()-1); i < s; ++i) {
					queueWorkers.emplace_back([&]() {
						bool hasActive = true;
						while(hasActive) {
							hasActive = false;
							for(std::size_t i(0), s(queues.size()); i < s; ++i) {
								queues[i].run();
								hasActive = !queues[i].eof() || hasActive;
							}
						}
					});
				}
				
				MyPrioQ pq(PrioComp(traits.compare(), &queueValues));
				queueValues.resize(queues.size());
				
				for(std::size_t i(0); i < queues.size(); ++i) {
					while (true) {
						if (queues[i].pop(queueValues[i])) { 
							pq.push(i);
							break;
						}
						else if (queues[i].eof()) {
							break;
						}
					}
				}
				
				while(pq.size()) {
					uint32_t pqMin = pq.top();
					pq.pop();
					value_type & v = queueValues[pqMin];
					if (!traits.makeUnique() || !tmp.size() || !traits.equal()(tmp.back(), v)) {
						tmp.emplace_back(std::move(v));
						state.resultSize += 1;
					}
					//get our next element
					while (true) {
						if (queues[pqMin].pop(v)) {
							pq.push(pqMin);
							break;
						}
						else if (queues[pqMin].eof()) {
							break;
						}
					}
					if (tmp.size() % 1000 == 0) {
						state.pinfo(tmp.size());
					}
					SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmp.size(), state.srcSize);
				}
				for(std::size_t i(0), s(queueWorkers.size()); i < s; ++i) {
					queueWorkers[i].join();
				}
			}
			else {
				MyPrioQ pq(PrioComp(traits.compare(), &(state.activeChunkBufferValues)));
			
				for(std::size_t i(0); i < state.activeChunkBuffers.size(); ++i) {
					pq.push(i);
				}
			
				while (pq.size()) {
					uint32_t pqMin = pq.top();
					pq.pop();
					InputBuffer & chunkBuffer = state.activeChunkBuffers[pqMin];
					value_type & v = state.activeChunkBufferValues[pqMin];
					if (!traits.makeUnique() || !tmp.size() || !traits.equal()(tmp.back(), v)) {
						tmp.emplace_back(std::move(v));
						state.resultSize += 1;
					}
					if (chunkBuffer.next()) {
						state.activeChunkBufferValues[pqMin] = chunkBuffer.get();
						pq.push(pqMin);
					}
					if (tmp.size() % 1000 == 0) {
						state.pinfo(tmp.size());
					}
					SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmp.size(), state.srcSize);
				}
			}
			state.pinfo(tmp.size());
			
			if (!traits.makeUnique() || nextRoundPendingChunks.back().first != tmp.size()) {
				nextRoundPendingChunks.back().second = tmp.size();
			}
			else {
				//nothing has been added, remove our chunk from the queue 
				//this never happens if TUniquify is false,
				//since then all elements of the chunks are appended to tmp
				nextRoundPendingChunks.pop_back();
			}
			state.activeChunkBuffers.clear();
			state.activeChunkBufferValues.clear();
		}
		
		if (traits.makeUnique()) {
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
	
	#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
	if (traits.makeUnique()) {
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
