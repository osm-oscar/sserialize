#ifndef SSERIALIZE_STAT_FUNCS_H
#define SSERIALIZE_STAT_FUNCS_H
#include <numeric>
#include <algorithm>
#include <sserialize/utility/utilfuncs.h>

namespace sserialize {
namespace statistics {

template<typename TIterator, typename TValue>
TValue mean(TIterator begin, const TIterator & end, TValue initial) {
	if (begin == end) {
		return initial;
	}
	std::size_t count = 0;
	for(; begin != end; ++begin, ++count) {
		initial += *begin;
	}
	return initial/count;
}

template<typename TIterator, typename TValue>
TValue variance(TIterator begin, const TIterator & end, TValue initial) {
	if (begin == end) {
		return initial;
	}
	TValue m = mean(begin, end, initial);
	std::size_t count = 0;
	for(;begin != end; ++begin, ++count) {
		TValue v = *begin - m;
		initial += v*v;
	}
	return initial / count;
}

template<typename TIterator, typename TValue>
TValue stddev(TIterator begin, const TIterator & end, TValue initial) {
	return std::sqrt( variance(begin, end, initial) );
}

template<typename TIterator, typename TValue>
TValue median(const TIterator & begin, const TIterator & end, TValue def) {
	std::vector<TValue> v(begin, end);
	if (!v.size())
		return def;
	std::sort(v.begin(), v.end());
	return v.at( v.size() / 2);
}


/** Iterators to a item->frequency table */
template<typename TIterator, typename TValue>
TValue entropy(TIterator begin, const TIterator & end, TValue initial, TValue totalCount) {
	for(; begin != end; ++begin) {
		TValue wn = begin->second/totalCount;
		initial += wn * logTo2(wn) ;
	}
	return - initial;
}


template<typename TIterator>
void linearRegression(TIterator begin, const TIterator & end, double & slope, double & yintercept) {
	if (begin == end) {
		slope = 1;
		yintercept = 0;
		return;
	}

	uint64_t size = 0;
	uint64_t cysum = 0.0;
	for(; begin != end; ++begin, ++size) {
		cysum += *begin;
	}
	double meanY = (double)(cysum/size) + static_cast<double>(cysum%size)/size;
	double meanX = (double)(size-1)/2.0;
	double count = 0.0;
	slope = 0.0;
	for(TIterator it(begin); it != end; ++it, count += 1.0) {
		slope += (count - meanX)*(*it-meanY);
	}
	slope /= size-1; //(n)
	slope *= 12;
	slope /= size+1; //(n+2)
	slope /= size; //(n+1)
	yintercept = meanY - slope*meanX;//BUG: this sometimes produces very wrong results
}


}}//end namespace

#endif