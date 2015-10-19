#ifndef SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#define SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#include <functional>
#include <type_traits>
#include <iterator>
#include <vector>
#include <queue>
#include <sserialize/containers/MMVector.h>
#include <sserialize/algorithm/utilcontainerfuncs.h>
#include <sserialize/iterator/RangeGenerator.h>

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
	InputBuffer(TSourceIterator srcBegin, TSourceIterator srcEnd, OffsetType bufferSize) :
	m_srcIt(srcBegin),
	m_srcEnd(srcEnd),
	m_bufferSize(bufferSize)
	{
		m_buffer.reserve(m_bufferSize);
		fillBuffer();
	}
	TValue & get() { return *m_bufferIt; }
	const TValue & get() const { return *m_bufferIt; }
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
				uint32_t threadCount = 0)
{
	typedef TInputOutputIterator SrcIterator;
	typedef typename std::iterator_traits<SrcIterator>::value_type value_type;
	typedef detail::oom::InputBuffer<SrcIterator> InputBuffer;
	
	assert(begin < end);
	
	if (!threadCount) {
		threadCount = std::thread::hardware_concurrency();
	}
	
	OffsetType srcSize = std::distance(begin, end);
	
	std::vector<value_type> buffer(chunkSize);
	std::vector<InputBuffer> chunkBuffers;
	for(uint64_t i(0); i < srcSize; i += chunkSize) {
		OffsetType myChunkSize = std::min<OffsetType>(chunkSize, srcSize-i);
		auto chunkBegin = begin+i;
		auto chunkEnd = chunkBegin+myChunkSize;
		buffer.assign(chunkBegin, chunkEnd);
		chunkBuffers.emplace_back(chunkBegin, chunkEnd, chunkBufferSize);
// 		sserialize::mt_sort(buffer.begin(), buffer.end(), comp, threadCount);
		std::sort(buffer.begin(), buffer.end(), comp);
		//flush back
		for(auto & x : buffer) {
			*chunkBegin = std::move(x);
			++chunkBegin;
		}
		assert(chunkBegin == chunkEnd);
		assert(std::is_sorted(begin+i, chunkEnd));
	}
	
	//now merge the chunks
	sserialize::MMVector<value_type> tmp(mmt);
	tmp.reserve(srcSize);
	
	struct PrioComp {
		CompFunc * comp;
		std::vector<InputBuffer> * chunkBuffers;
		bool operator()(uint32_t a, uint32_t b) { return (*comp)(chunkBuffers->at(a).get(), chunkBuffers->at(b).get()); }
		PrioComp(CompFunc * comp, std::vector<InputBuffer> * chunkBuffers) : comp(comp), chunkBuffers(chunkBuffers) {}
		PrioComp(const PrioComp & other) : comp(other.comp), chunkBuffers(other.chunkBuffers) {}
	};
	
	typedef std::priority_queue<uint32_t, std::vector<uint32_t>, PrioComp> MyPrioQ;
	
	MyPrioQ pq(PrioComp(&comp, &chunkBuffers));
	
	//fill the queue with the initial data
	for(uint32_t i(0), s(chunkBuffers.size()); i < s; ++i) {
		pq.push(i);
	}
	
	while (pq.size()) {
		uint32_t pqMin = pq.top();
		pq.pop();
		tmp.emplace_back(std::move( chunkBuffers.at(pqMin).get() ));
		if (chunkBuffers[pqMin].next()) {
			pq.push(pqMin);
		}
	}
	assert(std::is_sorted(tmp.begin(), tmp.end()));

	///move back to source
	for(auto & x : tmp) {
		*begin = std::move(x);
		++begin;
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