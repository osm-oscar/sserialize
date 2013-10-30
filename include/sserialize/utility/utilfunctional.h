#ifndef SSERIALIZE_UTIL_FINCTIONAL_H
#define SSERIALIZE_UTIL_FINCTIONAL_H
#include <sserialize/utility/utilmemory.h>
#include <algorithm>
#include <assert.h>

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

namespace ReorderMappers {
	struct Identity {
		std::size_t at(std::size_t pos) const { return pos; }
	};
};

namespace detail {

///@param reorderMap maps new positions to old positions
template<typename T_RANDOM_ACCESS_CONTAINER, typename T_REORDER_MAP, typename SizeType>
void reorder(T_RANDOM_ACCESS_CONTAINER & srcDest, const T_REORDER_MAP & reorderMap) {
	std::vector<SizeType> itemToCurPos;
	itemToCurPos.reserve(srcDest.size());
	for(std::size_t i = 0, s = srcDest.size(); i < s; ++i) {
		itemToCurPos.push_back(i);
	}
	std::vector<SizeType> posToItem(itemToCurPos);
	for(SizeType i = 0, s = srcDest.size(); i < s; ++i) {
		SizeType initialItemPos = reorderMap.at(i);
		SizeType realSrcItemPos = itemToCurPos[initialItemPos];
		SizeType itemInCurDest = posToItem[i];
		std::swap(srcDest[i], srcDest[realSrcItemPos]);
		//now update the information to our swapped items
		itemToCurPos[initialItemPos] = i;
		posToItem[i] = initialItemPos;
		itemToCurPos[itemInCurDest] = realSrcItemPos;
		posToItem[realSrcItemPos] = itemInCurDest;
	}
}

}

///@param reorderMap maps new positions to old positions
template<typename T_RANDOM_ACCESS_CONTAINER, typename T_REORDER_MAP>
void reorder(T_RANDOM_ACCESS_CONTAINER & srcDest, const T_REORDER_MAP & reorderMap) {
	//save some runtime space at the expense of codesize
	if (srcDest.size() < std::numeric_limits<uint16_t>::max()) {
		detail::reorder<T_RANDOM_ACCESS_CONTAINER, T_REORDER_MAP, uint16_t>(srcDest, reorderMap);
	}
	else if (srcDest.size() < std::numeric_limits<uint32_t>::max()) {
		detail::reorder<T_RANDOM_ACCESS_CONTAINER, T_REORDER_MAP, uint32_t>(srcDest, reorderMap);
	}
	else {
		detail::reorder<T_RANDOM_ACCESS_CONTAINER, T_REORDER_MAP, uint64_t>(srcDest, reorderMap);
	}
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