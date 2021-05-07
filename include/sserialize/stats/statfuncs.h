#ifndef SSERIALIZE_STAT_FUNCS_H
#define SSERIALIZE_STAT_FUNCS_H
#include <numeric>
#include <algorithm>
#include <ostream>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/checks.h>

namespace sserialize {
namespace statistics {

struct StatPrinting {
	typedef enum {S_MEAN=0x1, S_MIN=0x2, S_MAX=0x4, S_MEDIAN=0x8, S_VARIANCE=0x10, S_STDDEV=0x20, S_ALL=0xFFFFFFFF} Stats;
	template<typename TIterator, typename TValue = typename std::iterator_traits<TIterator>::value_type>
	static void print(std::ostream & out, TIterator begin, const TIterator & end, Stats which = S_ALL, TValue initial = TValue());
};

template<typename TIterator, typename TValue>
TValue min(TIterator begin, const TIterator & end, TValue initial);

template<typename TIterator, typename TValue>
TValue max(TIterator begin, const TIterator & end, TValue initial);

template<typename TIterator, typename TValue>
TValue mean(TIterator begin, const TIterator & end, TValue initial);

template<typename TIterator, typename TValue>
TValue variance(TIterator begin, const TIterator & end, TValue initial);

template<typename TIterator, typename TValue>
TValue stddev(TIterator begin, const TIterator & end, TValue initial);

template<typename TIterator, typename TValue>
TValue median(const TIterator & begin, const TIterator & end, TValue def);


/** Iterators to a item->frequency table */
template<typename TIterator, typename TValue>
TValue entropy(TIterator begin, const TIterator & end, TValue initial, TValue totalCount);

template<typename TIterator, typename TIntegerType = uint64_t, typename TFloatType = double, bool large = false>
void linearRegression(TIterator begin, TIterator end, double & slope, double & yintercept);


}}//end namespace

//now the definition

namespace sserialize {
namespace statistics {

template<typename TIterator, typename TValue>
void
StatPrinting::print(std::ostream & out, TIterator begin, const TIterator & end, StatPrinting::Stats which, TValue initial) {
	if (which & S_MIN) {
		TValue v = sserialize::statistics::min(begin, end, initial);;
		out << "min: " << v << '\n';
	}
	if (which & S_MAX) {
		TValue v = sserialize::statistics::max(begin, end, initial);;
		out << "max: " << v << '\n';
	}
	if (which & S_MEAN) {
		TValue v = sserialize::statistics::mean(begin ,end, initial);
		out << "Mean: " << v << '\n';
	}
	if (which & S_MEDIAN) {
		TValue v = sserialize::statistics::median(begin, end, initial);
		out << "Median: " << v << '\n';
	}
	if (which & S_VARIANCE) {
		TValue v = sserialize::statistics::variance(begin, end, initial);
		out << "Variance: " << v << '\n';
	}
	if (which & S_STDDEV) {
		TValue v = sserialize::statistics::stddev(begin, end, initial);
		out << "Stddev: " << v << '\n';
	}
}

template<typename TIterator, typename TValue>
TValue min(TIterator begin, const TIterator & end, TValue initial) {
	TIterator it = std::min_element(begin, end);
	if (it != end) {
		return *it;
	}
	return initial;
}

template<typename TIterator, typename TValue>
TValue max(TIterator begin, const TIterator & end, TValue initial) {
	TIterator it = std::max_element(begin, end);
	if (it != end) {
		return *it;
	}
	return initial;
}

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
	if (!v.size()) {
		return def;
	}
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


template<typename TIterator, typename TIntegerType, typename TFloatType, bool large>
void linearRegression(TIterator begin, TIterator end, double & slope, double & yintercept) {
	using int_type = TIntegerType;
	using float_type = TFloatType;
	if (begin == end) {
		slope = 1;
		yintercept = 0;
		return;
	}

	int_type size = 0;
	float_type meanY;
	
	if constexpr (large) {
		using std::distance;
		size = distance(begin, end);
		int_type remainder = 0;
		int_type int_part = 0;
		for(TIterator it(begin); it != end; ++it) {
			using namespace checked;
			remainder = add(remainder, *it);
			int_part = add(int_part, remainder/size);
			remainder = remainder%size;
		}
		meanY = float_type(int_part) + float_type(remainder)/size;
	}
	else {
		//This will traverse begin-end only once and use less ops
		int_type cysum = 0;
		for(TIterator it(begin); it != end; ++it, ++size) {
			cysum = checked_add(cysum, *it);
		}
		meanY = (float_type)(cysum/size) + static_cast<float_type>(cysum%size)/static_cast<float_type>(size);
	}
	float_type meanX = (float_type)(size-1)/2.0;
	float_type count = 0.0;
	slope = 0.0;
	for(TIterator it(begin); it != end; ++it, count += 1.0) {
		slope += (count - meanX)*(*it-meanY);
	}
	slope /= (float_type)(size-1); //(n)
	slope *= 12;
	slope /= (float_type)(size+1); //(n+2)
	slope /= (float_type)size; //(n+1)
	yintercept = meanY - slope*meanX;
}

}}//end namespace sserialize::statistics



#endif
