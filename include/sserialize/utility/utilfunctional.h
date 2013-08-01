#ifndef SSERIALIZE_UTIL_FINCTIONAL_H
#define SSERIALIZE_UTIL_FINCTIONAL_H
#include <sserialize/utility/utilmemory.h>
#include <algorithm>

namespace sserialize {

template<typename T_OUT_TYPE, typename T_INPUT_IT, typename T_MAP_FUNC>
T_OUT_TYPE transform(T_INPUT_IT begin, const T_INPUT_IT & end, T_MAP_FUNC func) {
	T_OUT_TYPE out;
	typename T_OUT_TYPE::iterator outIt(out.end());
	while(begin  != end) {
		outIt = out.insert(outIt, func(*begin));
		++begin;
	};
	return out;
};

template<typename T_CONTAINER>
T_CONTAINER sort(T_CONTAINER a) {
	std::sort(a.begin(), a.end());
	return a;
}

/** @param begin iterator pointing to the first element
  * @param end iterator pointing past the last element
  * @param func function that maps two iterator::value_type to a new one
  */
template<typename T_ITERATOR, typename T_RETURN = typename std::iterator_traits<T_ITERATOR>::value_type, typename T_FUNC>
T_RETURN treeMap(T_ITERATOR begin, T_ITERATOR end, T_FUNC mapFunc) {
	if (end - begin == 0) {
		return T_RETURN();
	}
	else if (end - begin == 1) {
		return *begin;
	}
	else if (end - begin == 2) {
		return mapFunc(*begin, *(begin+1));
	}
	else {
		return mapFunc( treeMap<T_ITERATOR, T_RETURN, T_FUNC>(begin, begin+(end-begin)/2, mapFunc),
						treeMap<T_ITERATOR, T_RETURN, T_FUNC>(begin+(end-begin)/2, end, mapFunc)
					);
	}
}

///@param reorderMap maps new positions to old positions
template<typename T_RANDOM_ACCESS_CONTAINER, typename T_REORDER_MAP>
void reorder(T_RANDOM_ACCESS_CONTAINER & srcDest, const T_REORDER_MAP & reorderMap) {
	T_RANDOM_ACCESS_CONTAINER tmp(srcDest.size());
	for(uint32_t i = 0, s = srcDest.size(); i < s; ++i) {
		std::swap(srcDest[reorderMap.at(i)], tmp[i]);
	}
	srcDest.swap(tmp);
}

///creates a range starting with begin and ending with end exclusive (if begin + m*inc < end)
template<typename T_OUT_CONTAINER, typename T_TYPE, typename T_INC = T_TYPE>
T_OUT_CONTAINER range(T_TYPE begin, const T_TYPE & end, const T_INC & inc) {
	T_OUT_CONTAINER ret;
	for(; begin < end; begin += inc) {
		appendOrInsert(begin, ret);
	}
	return ret;
}

}//end namespace


#endif