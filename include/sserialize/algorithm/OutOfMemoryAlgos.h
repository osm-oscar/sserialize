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

template<typename TSourceIterator, typename TValue = std::remove_reference<TSourceIterator::value_type>::type >
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
		OffsetType copyAmount(0);
		for(; copyAmount < m_bufferSize && m_srcIt != m_srcEnd; ++copyAmount, ++m_srcIt) {
			m_buffer[copyAmount] = *m_srcIt;
		}
		m_buffer.resize(copyAmount);
		m_bufferIt = m_buffer.begin();
	}
public:
	InputBuffer(TSourceIterator srcBegin, TSourceIterator srcEnd, OffsetType bufferSize) :
	m_srcIt(srcBegin),
	m_srcEnd(srcEnd),
	m_bufferSize(bufferSize)
	{
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

template<typename TRandomAccessContainer>
class OutOfMemorySorter final {
public:
	typedef TRandomAccessContainer container;
	typedef typename container value_type;
	typedef typename container::iterator container_iterator;
private:
	typedef detail::oom::InputBuffer<container_iterator, value_type> InputBuffer;
private:
	MmappedMemoryType m_mmt;
	OffsetType m_mergeBufferSize;
private:
	template<typename CompFunc>
	void mergeChunks(std::vector<InputBuffer> & chunks, sserialize::MMVector<value_type> & dest, CompFunc comp);
public:
	OutOfMemorySorter(MmappedMemoryType tempStorageType, OffsetType mergeBufferSize) : m_mmt(tempStorageType), m_mergeBufferSize(mergeBufferSize) {}
	~OutOfMemorySorter() {}
	template<typename CompFunc>
	void sort(container & srcdest, OffsetType bufferSize, CompFunc comp, unsigned int numThreads = 0);
};

//------ Implementation ---------------

template<typename TRandomAccessContainer>
template<typename CompFunc>
void
OutOfMemorySorter<TRandomAccessContainer>::
sort(TRandomAccessContainer & srcdest, sserialize::OffsetType bufferSize, CompFunc comp, unsigned int numThreads) {
	OffsetType srcSize = srcdest.size();
	std::vector<TValue> buffer(bufferSize);
	std::vector<InputBuffer> chunkBuffers;
	for(uint64_t i(0); i < srcSize; i += bufferSize) {
		OffsetType chunkSize = std::min<OffsetType>(bufferSize, srcSize-i);
		auto srcBegin = srcdest.cbegin()+i;
		auto srcEnd = srcBegin+chunkSize;
		buffer.assign(srcBegin, srcEnd);
		chunkBuffers.emplace_back(srcBegin, srcEnd, m_mergeBufferSize);
		sserialize::mt_sort(buffer.begin(), buffer.begin()+chunkSize, comp, numThreads);
		//flush back
		for(uint64_t j(0); j < chunkSize; ++j) {
			srcdest[j+i] = buffer[j];
		}
	}
	sserialize::MMVector<value_type> tmp(sserialize::MM_FILEBASED);
	tmp.reserve(srcSize);
	mergeChunks(srcdest, tmp, comp);
	{
		auto srcIt = srcdest.begin();
		for(auto & x : tmp) {
			*srcIt = std::move(x);
			++srcIt;
		}
	}
}


///TODO: real world usage data is not! plain old data
///This does a merge without! unique
template<typename TRandomAccessContainer>
template<typename CompFunc>
void
OutOfMemorySorter<TRandomAccessContainer>::
mergeChunks(std::vector<InputBuffer> & chunks, sserialize::MMVector<value_type> & dest, CompFunc comp) {
	auto myComp = [&comp, &chunks](const uint32_t a, const uint32_t b) { return comp(chunks[a], chunks[b]); };
	typedef std::priority_queue<uint32_t, decltype(myComp)> MyPrioQ;
	
	MyPrioQ pq;
	while (pq.size()) {
		uint32_t pqMin = pq.top();
		pq.pop();
		dest.emplace_back(chunks.at(pqMin).get());
		if (chunks[pqMin].next()) {
			pq.push(pqMin);
		}
	}
}

///@param chunkBufferSize: 8 MebiByte
///@param chunkSize: 256 MebiByte
template<typename TRandomAccessContainer, typename CompFunc>
void oom_sort(const TRandomAccessContainer & srcDest,
				CompFunc comp, OffsetType chunkSize = 0x2000000,
				OffsetType chunkBufferSize = 0x100000,
				sserialize::MmappedMemoryType mmt = sserialize::MM_FILEBASED,
				uint32_t threadCount = 0
				)
{
	OutOfMemorySorter<TRandomAccessContainer> sorter(mmt, chunkBufferSize);
	sorter.sort(srcDest, chunkSize, comp, threadCount);
}

///srcdest needs to be sorted
template<typename TInputOutputIterator, typename TEqual = std::equal_to< std::iterator_traits<TInputOutputIterator>::value_type > >
TInputOutputIterator oom_unique(TInputOutputIterator begin, TInputOutputIterator end, sserialize::MmappedMemoryType mmt = sserialize::MM_FILEBASED, TEqual eq = TEqual()) {
	if (begin == end) {
		return begin;
	}
	sserialize::MMVector<TRandomAccessContainer> tmp;
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