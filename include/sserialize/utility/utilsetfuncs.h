#ifndef SSERIALIZE_UTIL_SET_FUNCS_H
#define SSERIALIZE_UTIL_SET_FUNCS_H
#include <algorithm>
#include <sserialize/utility/utilmemory.h>

namespace sserialize {


template<typename TIter1, typename TIter2>
std::size_t set_intersection_size(TIter1 begin1, TIter1 end1, TIter2 begin2, TIter2 end2) {
	std::size_t size = 0;
	while(begin1 != end1 && begin2 != end2) {
		if (*begin1 < *begin2) {
			++begin1;
		}
		else if (*begin2 < *begin1) {
			++begin2;
		}
		else {
			++begin1;
			++begin2;
			++size;
		}
	}
	return size;
}

template<typename T>
void diffSortedContainer(std::set<T> & out, const std::set<T> & a, const std::set<T> & b) {
	std::set<T> result;
	std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::insert_iterator< std::set<T> >(result, result.end()));
	std::swap(out, result);
}

template<typename T, typename T_CONTAINER_A, typename T_CONTAINER_B>
void diffSortedContainer(std::set<T> & out, const T_CONTAINER_A & a, const T_CONTAINER_B & b) {
	std::set<T> result;
	typename T_CONTAINER_A::const_iterator aIt( a.begin() );
	typename T_CONTAINER_A::const_iterator aEnd( a.end() );
	typename T_CONTAINER_B::const_iterator bIt( b.begin() );
	typename T_CONTAINER_B::const_iterator bEnd( b.end() );
	typename std::set<T>::iterator rIt = result.begin();
	while(aIt != aEnd && bIt != bEnd) {
		if (*aIt < *bIt) {
			rIt = result.insert(rIt, *aIt);
			++aIt;
		}
		else if (*bIt < *aIt) {
			++bIt;
		}
		else {
			++aIt;
			++bIt;
		}
	}
	while (aIt != aEnd) {
		rIt = result.insert(rIt, *aIt);
		++aIt;
	}
	
	std::swap(out, result);
}

template<typename T_CONTAINER_DEST, typename T_CONTAINER_A, typename T_CONTAINER_B>
void diffSortedContainer(T_CONTAINER_DEST & out, const T_CONTAINER_A & a, const T_CONTAINER_B & b) {
	T_CONTAINER_DEST result;
	typename T_CONTAINER_A::const_iterator aIt( a.begin() );
	typename T_CONTAINER_A::const_iterator aEnd( a.end() );
	typename T_CONTAINER_B::const_iterator bIt( b.begin() );
	typename T_CONTAINER_B::const_iterator bEnd( b.end() );
	while(aIt != aEnd && bIt != bEnd) {
		if (*aIt < *bIt) {
			result.push_back(*aIt);
			++aIt;
		}
		else if (*bIt < *aIt) {
			++bIt;
		}
		else {
			++aIt;
			++bIt;
		}
	}
	while (aIt != aEnd) {
		result.push_back(*aIt);
		++aIt;
	}
	std::swap(out, result);
}

template<typename T, typename T_CONTAINER>
void inplaceDiffSortedContainer(std::set<T> & a, const T_CONTAINER & b) {
	if (!b.size() || !a.size()) 
		return;
	typename std::set<T>::iterator aIt = a.begin();
	typename std::set<T>::iterator aEnd = a.end();
	typename T_CONTAINER::const_iterator bIt = b.begin();
	typename T_CONTAINER::const_iterator bEnd = b.end();
	while(aIt != aEnd && bIt != bEnd) {
		if (*aIt < *bIt) {
			++aIt;
		}
		else if (*bIt < *aIt) {
			++bIt;
		}
		else {
			++bIt;
			a.erase(aIt++);
		}
	}
}

template<typename T1, typename T2>
void inplaceDiffSortedContainer(T1 & a, const T2 & b) {
	diffSortedContainer(a, a, b);
}

template<typename T_CONTAINER_DEST, typename T_CONTAINER_A, typename T_CONTAINER_B>
void mergeSortedContainer(T_CONTAINER_DEST & out, const T_CONTAINER_A & a, const T_CONTAINER_B & b, uint64_t & mergeComparisonCount) {
	if (a.size() == 0) {
		out = T_CONTAINER_DEST(b.begin(), b.end());
		return;
	}
	if (b.size() == 0) {
		out = T_CONTAINER_DEST(a.begin(), a.end());
		return;
	}
	T_CONTAINER_DEST result;
	reserveMemory(result, std::max(a.size(), b.size()));
	typename T_CONTAINER_A::const_iterator aIndexIt(a.begin());
	typename T_CONTAINER_A::const_iterator aEnd(a.end());
	typename T_CONTAINER_B::const_iterator bIndexIt(b.begin());
	typename T_CONTAINER_B::const_iterator bEnd(b.end());
	while (aIndexIt != aEnd && bIndexIt != bEnd) {
		uint32_t aItemId = *aIndexIt;
		uint32_t bItemId = *bIndexIt;

		mergeComparisonCount += 2; //access penalty
		
		if (aItemId == bItemId) {
			appendOrInsert(aItemId, result);
			mergeComparisonCount += 1;
			++aIndexIt;
			++bIndexIt;
		}
		else if (aItemId < bItemId) {
			appendOrInsert(aItemId, result);
			mergeComparisonCount+=2;
			++aIndexIt;
		}
		else { //bItemId is smaller
			appendOrInsert(bItemId, result);
			mergeComparisonCount += 3;
			++bIndexIt;
		}
	}

	while (aIndexIt != aEnd) { //if there are still some elements left in aindex
		appendOrInsert(*aIndexIt, result);
		++aIndexIt;
		++mergeComparisonCount;
	}

	while (bIndexIt != bEnd) { //if there are still some elements left in bindex
		appendOrInsert(*bIndexIt, result);
		++bIndexIt;
		++mergeComparisonCount;
	}
	std::swap(result, out);
}

template<typename T_CONTAINER_DEST, typename T_CONTAINER_A, typename T_CONTAINER_B>
void mergeSortedContainer(T_CONTAINER_DEST & out, const T_CONTAINER_A & a, const T_CONTAINER_B & b) {
	if (a.size() == 0) {
		out = T_CONTAINER_DEST(b.begin(), b.end());
		return;
	}
	if (b.size() == 0) {
		out = T_CONTAINER_DEST(a.begin(), a.end());
		return;
	}
	T_CONTAINER_DEST result;
	reserveMemory(result, std::max(a.size(), b.size()));
	typename T_CONTAINER_A::const_iterator aIndexIt(a.begin());
	typename T_CONTAINER_A::const_iterator aEnd(a.end());
	typename T_CONTAINER_B::const_iterator bIndexIt(b.begin());
	typename T_CONTAINER_B::const_iterator bEnd(b.end());
	while (aIndexIt != aEnd && bIndexIt != bEnd) {
		uint32_t aItemId = *aIndexIt;
		uint32_t bItemId = *bIndexIt;

		if (aItemId == bItemId) {
			appendOrInsert(aItemId, result);
			++aIndexIt;
			++bIndexIt;
		}
		else if (aItemId < bItemId) {
			appendOrInsert(aItemId, result);
			++aIndexIt;
		}
		else { //bItemId is smaller
			appendOrInsert(bItemId, result);
			++bIndexIt;
		}
	}

	while (aIndexIt != aEnd) { //if there are still some elements left in aindex
		appendOrInsert(*aIndexIt, result);
		++aIndexIt;
	}

	while (bIndexIt != bEnd) { //if there are still some elements left in bindex
		appendOrInsert(*bIndexIt, result);
		++bIndexIt;
	}
	std::swap(result, out);
}

void mergeSortedContainer(std::vector<uint32_t> & out, const std::vector<uint32_t> & a, const std::vector<uint32_t> & b);

template<typename T_SORTED_CONTAINER_A, typename T_SORTED_CONTAINER_B = T_SORTED_CONTAINER_A, typename T_OUT_CONTAINER = T_SORTED_CONTAINER_A>
T_OUT_CONTAINER intersect(const T_SORTED_CONTAINER_A & a, T_SORTED_CONTAINER_B & b) {
	T_OUT_CONTAINER ret;
	std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(ret, ret.end()));
	return ret;
}

}//end namespace

#endif