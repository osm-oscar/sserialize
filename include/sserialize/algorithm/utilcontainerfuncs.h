#ifndef SSERIALIZE_UTIL_CONTAINER_FUNCS_H
#define SSERIALIZE_UTIL_CONTAINER_FUNCS_H
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <vector>
#include <deque>
#include <algorithm>
#include <thread>

namespace sserialize {


template<typename TIterator>
bool is_strong_monotone_ascending(TIterator begin, const TIterator & end) {
	if (begin != end) {
		auto x = *begin;
		for(++begin; begin != end; ++begin) {
			auto y = *begin;
			if (y <= x) {
				return false;
			}
			x = y;
		}
	}
	return true;
}

///the range betwen begin and end needs to be sorted
template<typename TIterator>
bool is_unique(TIterator begin, const TIterator & end) {
	if (begin != end) {
		auto x = *begin;
		for(++begin; begin != end; ++begin) {
			auto y = *begin;
			if (y == x) {
				return false;
			}
			x = y;
		}
	}
	return true;
}

template<typename TIterator1, typename TIterator2, typename BinaryPredicate>
bool equal(TIterator1 begin1, const TIterator1 & end1, TIterator2 begin2, const TIterator2 & end2, BinaryPredicate pred = BinaryPredicate()) {
	for( ;begin1 != end1 && begin2 != end2; ++begin1, ++begin2) {
		if (!pred(*begin1, *begin2)) {
			return false;
		}
	}
	return (begin1 == end1 && begin2 == end2);
}

namespace detail {
	template<typename TIterator, typename T_COMPFUNC>
	void mt_sort_wrap(TIterator begin, TIterator end, T_COMPFUNC compFunc) {
		std::sort(begin, end, compFunc);
	}
}


///multi-threaded sorting, @param numThreads if 0, numThreads = std::thread::hardware_concurrency()
template<typename TIterator, typename CompFunc = std::less<typename std::iterator_traits<TIterator>::value_type> >
void mt_sort(TIterator begin, TIterator end, CompFunc comp, unsigned int numThreads = 0) {
	if (!numThreads) {
		numThreads = std::max<unsigned int>(std::thread::hardware_concurrency(), 1);
	}
	std::size_t numElements = std::distance(begin, end);
	
	//improve check of when to sort as this really doesn't make any sense on small data
	if (numElements > numThreads*10000 && numThreads > 1) {
		std::size_t blockSize = numElements/numThreads;
		std::vector<std::thread> myThreads;
		myThreads.reserve(numThreads+1);
		for(unsigned int i(0); i < numThreads; ++i) {
			std::nth_element(begin, begin+blockSize, end, comp);
			myThreads.push_back( std::thread(detail::mt_sort_wrap<TIterator, CompFunc>, begin, begin+blockSize, comp) );
			begin += blockSize;
		}
		if (begin != end) { //if there's someting left, put the rest into an extra thread (there might very well be another sorting thread completed before the other
			myThreads.push_back( std::thread(detail::mt_sort_wrap<TIterator, CompFunc>, begin, end, comp) );
		}
		for(std::thread & t : myThreads) {
			t.join();
		}
	}
	else {
		std::sort(begin, end, comp);
	}
}

template<typename T, typename T_CONTAINER>
void prependToDeque(const T_CONTAINER & source, std::deque<T> & dest) {
	for(typename T_CONTAINER::const_reverse_iterator it = source.rbegin(); it != source.rend(); ++it) {
		dest.push_front(*it);
	}
}

template<typename T_SOURCE_CONTAINER, typename T_DEST_CONTAINER, typename T_MAP_CONTAINER>
void remap(const T_SOURCE_CONTAINER & source, T_DEST_CONTAINER & dest, const T_MAP_CONTAINER & map) {
	T_DEST_CONTAINER newValues;
	std::insert_iterator<T_DEST_CONTAINER> it(newValues, newValues.end());
	std::transform(source.begin(), source.end(), it,[&map](const typename T_SOURCE_CONTAINER::value_type & x) { return map.at(x);});
	dest = std::move(newValues);
}

template<typename TContainer>
bool haveCommonValueOrdered(const TContainer & orderedContainerA, const TContainer & orderedContainerB) {
	if (orderedContainerA.size() == 0 || orderedContainerB.size() == 0) {
		return false;
	}
	typename TContainer::const_iterator aIt(orderedContainerA.cbegin());
	typename TContainer::const_iterator bIt(orderedContainerB.cbegin());
	typename TContainer::const_iterator aEnd(orderedContainerA.cend());
	typename TContainer::const_iterator bEnd(orderedContainerB.cend());

	while (aIt != aEnd && bIt != bEnd) {
		typename TContainer::const_reference aItemId = *aIt;
		typename TContainer::const_reference bItemId = *bIt;
		if (aItemId == bItemId) {
			return true;
		}
		else if (aItemId < bItemId) {
			++aIt;
		}
		else { //bItemId is smaller
			++bIt;
		}
	}
	return false;
}


/** @param b: b needs to implement operator count() */
template<typename T_CONTAINER, typename T_CONTAINER_B>
bool haveCommonValue(const T_CONTAINER & a, const T_CONTAINER_B & b) {
	if (a.size() == 0 || b.size() == 0) {
		return false;
	}
	typename T_CONTAINER::const_iterator aEnd( a.end() );
	for(typename T_CONTAINER::const_iterator it = a.begin(); it != aEnd; ++it) {
		if (b.count(*it) > 0)
			return true;
	}
	return false;
}

template<typename T>
bool haveCommonValue(const std::set<T> & a, const std::set<T> & b) {
	return haveCommonValueOrdered(a, b);
}

}//end namespace

#endif