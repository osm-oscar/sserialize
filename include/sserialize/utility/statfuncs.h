#ifndef SSERIALIZE_STAT_FUNCS_H
#define SSERIALIZE_STAT_FUNCS_H
#include <numeric>
#include <algorithm>
#include <sserialize/utility/utilfuncs.h>

namespace sserialize {
namespace statistics {

template<typename TIterator, typename TValue>
TValue mean(TIterator begin, const TIterator & end, TValue initial) {
	std::size_t count = 0;
	for(; begin != end; ++begin, ++count) {
		initial += *begin;
	}
	return initial/count;
}

template<typename TIterator, typename TValue>
TValue variance(TIterator begin, const TIterator & end, TValue initial) {
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



}}//end namespace

#endif