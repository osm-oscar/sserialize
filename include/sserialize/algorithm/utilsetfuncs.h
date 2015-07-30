#ifndef SSERIALIZE_UTIL_SET_FUNCS_H
#define SSERIALIZE_UTIL_SET_FUNCS_H
#include <algorithm>

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

///Check if begin1->end1 is a subseteq of begin2->end2
template<typename TIter1, typename TIter2>
bool subseteq_of(TIter1 begin1, TIter1 end1, TIter2 begin2, TIter2 end2) {
	while(begin1 != end1 && begin2 != end2) {
		if (*begin1 < *begin2) {
			return false;
		}
		else if (*begin2 < *begin1) {
			++begin2;
		}
		else {
			++begin1;
			++begin2;
		}
	}
	return (begin1 == end1);
}

///Check if begin1->end1 is a subset of begin2->end2
template<typename TIter1, typename TIter2>
bool subset_of(TIter1 begin1, TIter1 end1, TIter2 begin2, TIter2 end2) {
	bool set2lg = false;
	while(begin1 != end1 && begin2 != end2) {
		if (*begin1 < *begin2) {
			return false;
		}
		else if (*begin2 < *begin1) {
			++begin2;
			set2lg = true;
		}
		else {
			++begin1;
			++begin2;
		}
	}
	return (begin1 == end1) && (set2lg || begin2 != end2);
}

template<typename T_CONTAINER_DEST, typename T_CONTAINER_A, typename T_CONTAINER_B>
void mergeSortedContainer(T_CONTAINER_DEST & out, const T_CONTAINER_A & a, const T_CONTAINER_B & b) {
	T_CONTAINER_DEST tmp;
	std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::inserter(tmp, tmp.end()));
	out = std::move(tmp);
}

template<typename T_SORTED_CONTAINER_A, typename T_SORTED_CONTAINER_B = T_SORTED_CONTAINER_A, typename T_OUT_CONTAINER = T_SORTED_CONTAINER_A>
T_OUT_CONTAINER intersect(const T_SORTED_CONTAINER_A & a, const T_SORTED_CONTAINER_B & b) {
	T_OUT_CONTAINER ret;
	std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(ret, ret.end()));
	return ret;
}

}//end namespace

#endif