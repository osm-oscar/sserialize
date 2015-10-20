#ifndef SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#define SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#include <functional>
#include <type_traits>
#include <iterator>
#include <vector>
#include <queue>
#include <mutex>
#include <sserialize/containers/MMVector.h>
#include <sserialize/algorithm/utilcontainerfuncs.h>
#include <sserialize/iterator/RangeGenerator.h>
#include <sserialize/stats/ProgressInfo.h>

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
	InputBuffer(TSourceIterator srcBegin, TSourceIterator srcEnd, OffsetType bufferSize) :
	m_srcIt(srcBegin),
	m_srcEnd(srcEnd),
	m_bufferSize(bufferSize)
	{
		m_buffer.reserve(m_bufferSize);
		fillBuffer();
	}
	InputBuffer(InputBuffer && other) : m_srcIt(other.m_srcIt), m_srcEnd(other.m_srcEnd), m_bufferSize(other.m_bufferSize) {
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

}}//end namespace detail::oom

///@param chunkBufferSize: 8 MebiByte
///@param chunkSize: 256 MebiByte
template<typename TInputOutputIterator, typename CompFunc>
void oom_sort(TInputOutputIterator begin, TInputOutputIterator end,
				CompFunc comp, OffsetType chunkSize = 0x2000000,
				OffsetType chunkBufferSize = 0x100000,
				sserialize::MmappedMemoryType mmt = sserialize::MM_FILEBASED,
				uint32_t threadCount = 0, uint32_t queueDepth = 32)
{
	typedef TInputOutputIterator SrcIterator;
	typedef typename std::iterator_traits<SrcIterator>::value_type value_type;
	typedef detail::oom::InputBuffer<SrcIterator> InputBuffer;
	
	assert(begin < end);
	
	if (!threadCount) {
		threadCount = std::thread::hardware_concurrency();
	}
	
	struct Config {
		uint64_t chunkSize;
		uint64_t chunkBufferSize;
		CompFunc * comp;
	};
	
	struct State {
		SrcIterator srcIt;
		OffsetType srcSize;
		uint64_t srcOffset;
		std::mutex ioLock;
		
		std::vector<InputBuffer> chunkBuffers;
	};
	
	Config cfg;
	State state;
	
	cfg.chunkSize = chunkSize;
	cfg.chunkBufferSize = chunkBufferSize;
	cfg.comp = &comp;
	
	state.srcIt = begin;
	state.srcSize = std::distance(begin, end);
	state.srcOffset = 0;
	
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
				OffsetType myChunkSize = std::min<OffsetType>(cfg->chunkSize, state->srcSize-state->srcOffset);
				auto chunkBegin = state->srcIt;
				auto chunkEnd = chunkBegin+myChunkSize;
				state->srcOffset += myChunkSize;
				state->srcIt = chunkEnd;
				buffer.assign(chunkBegin, chunkEnd);
				
				//reserve space for our chunk in chunkBuffer, chunkbuffers need to be contigous
				//otherwise the multi-level merging would not work
				uint32_t chunkBufferEntry = state->chunkBuffers.size();
				state->chunkBuffers.resize(chunkBufferEntry+1);
				
				ioLock.unlock();

				std::sort(buffer.begin(), buffer.end(), *(cfg->comp));
				
				ioLock.lock();
				//flush back
				auto chunkIt = chunkBegin;
				for(auto & x : buffer) {
					*chunkIt = std::move(x);
					++chunkIt;
				}
				assert(chunkIt == chunkEnd);
				
				//push the chunk buffer, this needs to be AFTER sorting since the buffer buffers!
				state->chunkBuffers.at(chunkBufferEntry) = InputBuffer(chunkBegin, chunkEnd, cfg->chunkBufferSize);
			}
		}
	};
	//spawn the threads
	{
		std::vector<std::thread> ts;
		for(uint32_t i(0); i < threadCount; ++i) {
			ts.emplace_back(std::thread(Worker(&state, &cfg)));
		}
		for(std::thread & t : ts) {
			t.join();
		}
	}
	
	//now merge the chunks
	sserialize::MMVector<value_type> tmp(mmt);
	tmp.reserve(state.srcSize);
	
	struct PrioComp {
		CompFunc * comp;
		std::vector<InputBuffer> * chunkBuffers;
		bool operator()(uint32_t a, uint32_t b) { return !( (*comp)(chunkBuffers->at(a).get(), chunkBuffers->at(b).get()) ); }
		PrioComp(CompFunc * comp, std::vector<InputBuffer> * chunkBuffers) : comp(comp), chunkBuffers(chunkBuffers) {}
		PrioComp(const PrioComp & other) : comp(other.comp), chunkBuffers(other.chunkBuffers) {}
	};
	
	typedef std::priority_queue<uint32_t, std::vector<uint32_t>, PrioComp> MyPrioQ;
	
	MyPrioQ pq(PrioComp(&comp, &(state.chunkBuffers)));
	sserialize::ProgressInfo pinfo;
	
	for(uint32_t queueRound(0); state.chunkBuffers.size() > 1; ++queueRound) {
		SrcIterator srcIt = begin;
		std::vector<InputBuffer> nextRoundChunkBuffer;
		OffsetType count = 0;
		pinfo.begin(state.srcSize, std::string("Merging sorted chunks round ") + std::to_string(queueRound));

		for(uint32_t cbi(0), cbs(state.chunkBuffers.size()); cbi < cbs; cbi += queueDepth) {
			assert(!tmp.size());

			//Fill the queue with the buffers that need to be merged
			for(uint32_t i(cbi), s(cbi+std::min<uint32_t>(queueDepth, cbs-cbi)); i < s; ++i) {
				pq.push(i);
			}
			
			while (pq.size()) {
				uint32_t pqMin = pq.top();
				pq.pop();
				InputBuffer & chunkBuffer = state.chunkBuffers[pqMin];
				value_type & v = chunkBuffer.get();
				tmp.emplace_back(std::move(v));
				if (chunkBuffer.next()) {
					pq.push(pqMin);
				}
				if (tmp.size() % 1000 == 0) {
					pinfo(count+tmp.size());
				}
			}
			
			assert(std::is_sorted(tmp.begin(), tmp.end()));
			
			///move back this part to source
			auto chunkBegin = srcIt;
			for(auto & x : tmp) {
				*srcIt = std::move(x);
				++srcIt;
			}
			count += tmp.size();
			tmp.clear();
			nextRoundChunkBuffer.emplace_back(chunkBegin, srcIt, cfg.chunkBufferSize);
			assert(std::is_sorted(chunkBegin, srcIt));
		}
		assert(srcIt == end);
		pinfo.end();
		using std::swap;
		swap(state.chunkBuffers, nextRoundChunkBuffer);
	}
}

///iterators need to point to sorted range
template<typename TInputOutputIterator, typename TEqual = std::equal_to< typename std::iterator_traits<TInputOutputIterator>::value_type > >
TInputOutputIterator oom_unique(TInputOutputIterator begin, TInputOutputIterator end, sserialize::MmappedMemoryType mmt = sserialize::MM_FILEBASED, TEqual eq = TEqual()) {
	if (begin == end) {
		return begin;
	}
	typedef typename std::iterator_traits<TInputOutputIterator>::value_type value_type;
	
	sserialize::MMVector<value_type> tmp(mmt);
	tmp.reserve(std::distance(begin, end));
	
	auto it = begin;
	tmp.emplace_back(std::move(*it));
	for(++it; it != end; ++it) {
		if (!eq(*it, tmp.back())) {
			tmp.emplace_back(std::move(*it));
		}
	}
	
	auto inIt = tmp.begin();
	auto inEnd = tmp.end();
	auto outIt = begin;
	for(; inIt != inEnd; ++inIt, ++outIt) {
		*outIt = std::move(*inIt);
	}
	return outIt;
}

}//end namespace

#endif