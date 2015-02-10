#ifndef SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#define SSERIALIZE_OUT_OF_MEMORY_SORTER_H
#include <functional>
#include <type_traits>
#include <vector>
#include <sserialize/templated/MMVector.h>
#include <sserialize/utility/utilcontainerfuncs.h>
#include <sserialize/utility/RangeGenerator.h>

namespace sserialize {
namespace detail {
namespace oom {

template<typename TSourceIterator, typename TValue>
class InputBuffer {
	TSourceIterator m_srcIt;
	TSourceIterator m_srcEnd;
	OffsetType m_bufferSize;
	std::vector<TValue> m_buffer;
	std::vector<TValue>::iterator m_bufferIt;
private:
	void fillBuffer() {
		TSourceIterator bufferBeginIt = m_srcIt;
		for(OffsetType i(0); i < m_bufferSize && m_srcIt != m_srcEnd; ++i, ++m_srcIt) {
			m_buffer[i] = *m_srcIt;
		}
		m_buffer.resize(m_srcIt - bufferBeginIt);
	}
public:
	InputBuffer(TSourceIterator srcBegin, TSourceIterator srcEnd, OffsetType bufferSize) :
	m_srcIt(srcBegin),
	m_srcEnd(srcEnd),
	m_bufferSize(bufferSize)
	{
		fillBuffer();
	}
	InputBuffer(InputBuffer && other);
	InputBuffer & operator=(InputBuffer && other);
	TValue & top() { return *m_bufferIt; }
	void pop() { ++m_bufferIt; }
	bool empty() { return m_bufferIt >= m_buffer.end(); }
};

}}//end namespace detail::oom

template<typename TValue >
class OutOfMemorySorter final {
public:
	typedef TValue value_type;
private:
	MmappedMemoryType m_mmt;
	uint64_t m_mergeBufferSize;
private:
	template<typename TRandomAccessContainer, typename CompFunc>
	void mergeChunks(TRandomAccessContainer & srcdest, CompFunc & comp);
public:
	OutOfMemorySorter(MmappedMemoryType tempStorageType) : m_mmt(tempStorageType) {}
	~OutOfMemorySorter() {}
	template<typename TRandomAccessContainer, typename CompFunc>
	void sort(TRandomAccessContainer & srcdest, OffsetType bufferSize, CompFunc comp, unsigned int numThreads = 0);
};

//------ Implementation ---------------

template<typename TValue>
template<typename TRandomAccessContainer, typename CompFunc>
void
OutOfMemorySorter<TValue>::
sort(TRandomAccessContainer & srcdest, sserialize::OffsetType bufferSize, CompFunc comp, unsigned int numThreads) {
	OffsetType srcSize = srcdest.size();
	std::vector<TValue> buffer(bufferSize);
	for(uint64_t i(0); i < srcSize; i += bufferSize) {
		OffsetType chunkSize = std::min<OffsetType>(bufferSize, srcSize-i);
		buffer.assign(srcdest.cbegin()+i, srcdest.cbegin(i+chunkSize));
		sserialize::mt_sort(buffer.begin(), buffer.begin()+chunkSize, comp, numThreads);
		for(uint64_t j(0); j < chunkSize; ++j) {
			srcdest[j+i] = buffer[j];
		}
	}
	buffer = std::vector<TValue>();
	//chunks are now sorted, we now have to do the merge
	
}


///TODO: real world usage data is not! plain old data
template<typename TValue>
template<typename TRandomAccessContainer, typename CompFunc>
void
OutOfMemorySorter<TValue>::
mergeChunks(TRandomAccessContainer & srcdest, OffsetType begin, OffsetType end, OffsetType chunkSize, CompFunc & comp) {
	typedef detail::oom::InputBuffer<TRandomAccessContainer::iterator, TValue> MyInputBufferType;
	std::vector< MyInputBufferType > inputBuffers;
	{
		OffsetType dataSize = end-begin;
		OffsetType numChunks = dataSize/chunkSize + (dataSize%chunkSize? 1 : 0);
		inputBuffers.reserve(numChunks);
	}
	TRandomAccessContainer::iterator myBegin(srcdest.begin());
	for(OffsetType i(begin); i < end; i += chunkSize) {
		OffsetType realChunkSize = std::min<OffsetType>(chunkSize, end-i);
		inputBuffers.emplace_back(myBegin+i, myBegin+(i+realChunkSize), m_mergeBufferSize);
	}
	auto myComp = [&comp, &inputBuffers](const uint32_t a, const uint32_t b) { return comp(inputBuffers[a], inputBuffers[b]); };
	typedef std::multiset< uint32_t, decltype(myComp)> MyPrioQ;
	MyPrioQ priQ(sserialize::RangeGenerator(0, inputBuffers.size()).cbegin(), sserialize::RangeGenerator(0, inputBuffers.size()).cend(), myComp);
	sserialize::MMVector<TValue> outputBuffer(m_mmt);
	outputBuffer.reserve(end-begin);
	while (inputBuffers.size()) {
		MyPrioQ::iterator pB = priQ.begin();
		MyPrioQ::iterator pE = priQ.upper_bound(*pB);
		outputBuffer.push_back(inputBuffers[*pB].top());
		for(; pB != pE;) {
			MyInputBufferType & x = inputBuffers[pB->second];
			if (x.
		}
		
	}
}



}//end namespace

#endif