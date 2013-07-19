#ifndef COMMON_UTIL_FUNCS_H
#define COMMON_UTIL_FUNCS_H
#include <stdint.h>
#include <limits>
#include <deque>
#include <set>
#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stddef.h>
#include <cmath>
#include <algorithm>

namespace sserialize {

/** @return [-1, 0, 1]
   * @src http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
   */
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

int inline popCount(unsigned int v) {
	return __builtin_popcount(v);
}

double inline logTo2(double num) {
#ifdef __ANDROID__
	return std::log(num)/0.6931471805599453;;
#else
	return std::log2(num);
#endif
}

inline uint32_t createMask(uint8_t bpn) {
	return ((bpn == 32) ? std::numeric_limits<uint32_t>::max() : ((static_cast<uint32_t>(1) << bpn) - 1));
}

inline uint64_t createMask64(uint8_t bpn) {
	return ((bpn == 64) ? std::numeric_limits<uint64_t>::max() : ((static_cast<uint64_t>(1) << bpn) - 1));
}

//pos is between 0 and 30
inline void setBit(uint32_t & srcDest, uint8_t pos) {
	srcDest |= (0x1 << pos);
}

inline uint32_t saturatedAdd32(const uint32_t a, const uint32_t b) {
	return (a > std::numeric_limits<uint32_t>::max() - b) ? std::numeric_limits<uint32_t>::max() : a + b;
}


///@return position of the most significant bit, returns 0 iff num == 0
inline uint8_t msb(uint32_t num) {
	uint8_t r = 0;
	while (num >>= 1) {
		++r;
	}
	return r;
}

///@return position of the most significant bit, returns 0 if num == 0 or num == 1
inline uint8_t msb(uint64_t num) {
	uint8_t r = 0;
	while (num >>= 1) {
		++r;
	}
	return r;
}

template<typename T>
void appendToDeque(const std::deque<T> & source, std::deque<T> & dest) {
	for(typename std::deque<T>::const_iterator it = source.begin(); it != source.end(); it++) {
		dest.push_back(*it);
	}
}

template<typename T, typename T_CONTAINER>
void prependToDeque(const T_CONTAINER & source, std::deque<T> & dest) {
	for(typename T_CONTAINER::const_reverse_iterator it = source.rbegin(); it != source.rend(); it++) {
		dest.push_front(*it);
	}
}

template<typename T>
void insertDequeIntoSet(const std::deque<T> & source, std::set<T> & dest) {
	if (source.size())
		dest.insert(source.begin(), source.end());
}

template<typename T>
void insertSetIntoDeque(const std::set<T> & source, std::deque<T> & dest) {
	for(typename std::set<T>::const_iterator it = source.begin(); it != source.end(); it++) {
		dest.push_back(*it);
	}
}

template<typename T>
void insertSetIntoSet(const std::set<T> & source, std::set<T> & dest) {
	if (source.size())
		dest.insert(source.begin(), source.end());
}

template<typename T>
void insertSetIntoSet(const std::unordered_set<T> & source, std::set<T> & dest) {
	if (source.size())
		dest.insert(source.begin(), source.end());
}


template<typename T, typename DN>
void insertMapKeysIntoDeque(const std::map<T, DN> & source, std::deque<T> & dest) {
	for(typename std::map<T, DN>::const_iterator it = source.begin(); it != source.end(); it++) {
		dest.push_back(it->first);
	}
}

template<typename T, typename DN>
void insertMapKeysIntoVector(const std::map<T, DN> & source, std::vector<T> & dest) {
	dest.reserve(dest.size() + source.size());
	for(typename std::map<T, DN>::const_iterator it = source.begin(); it != source.end(); it++) {
		dest.push_back(it->first);
	}
}


template<typename TKEY, typename TVALUE>
void insertMapValuesIntoSet(const std::map<TKEY, TVALUE> & source, std::set<TVALUE> & dest) {
	for(typename std::map<TKEY, TVALUE>::const_iterator it = source.begin(); it != source.end(); it++) {
		dest.insert(it->second);
	}
}

template<typename T_SOURCE_ITERATOR, typename T_DEST>
void insertSecondIntoContainer(T_SOURCE_ITERATOR begin, const T_SOURCE_ITERATOR & end, T_DEST & dest) {
	typename T_DEST::iterator destIt(dest.end());
	for(; begin != end; ++begin) {
		destIt = dest.insert(destIt, begin->second);
	}
}

template<typename T_SOURCE_ITERATOR, typename T_DEST>
void insertFirstIntoContainer(T_SOURCE_ITERATOR begin, const T_SOURCE_ITERATOR & end, T_DEST & dest) {
	typename T_DEST::iterator destIt(dest.end());
	for(; begin != end; ++begin) {
		destIt = dest.insert(destIt, begin->first);
	}
}

/** Remaps everything in source with map into dest, source and dest can be equal */
template<typename T1, typename T2>
bool remapSet(const std::set<T1> & source, std::set<T2> & dest, const std::map<T1, T2> & map) {
	std::set<T2> newValues;
	bool allOK = true;
	for(typename std::set<T1>::const_iterator it = source.begin(); it != source.end(); it++) {
		if (map.count(*it)) {
			newValues.insert(map.at(*it));
		}
		else
			allOK = false;
	}
	dest.swap(newValues);
	return allOK;
}

template<typename T1, typename T2>
bool remapSorted(const std::set<T1> & source, std::set<T2> & dest, const std::map<T1, T2> & map) {
	return remapSet(source, dest, map);
}

template<typename T1, typename T2>
bool remapSorted(const std::deque<T1> & source, std::deque<T2> & dest, const std::map<T1, T2> & map) {
	std::set<T2> newValues;
	bool allOK = true;
	for(typename std::deque<T1>::const_iterator it = source.begin(); it != source.end(); it++) {
		if (map.count(*it)) {
			newValues.insert(map.at(*it));
		}
		else
			allOK = false;
	}
	dest = std::deque<T2>(newValues.begin(), newValues.end());
	return allOK;
}

template<typename T1, typename T2>
bool remapSorted(const std::vector<T1> & source, std::vector<T2> & dest, const std::map<T1, T2> & map) {
	std::set<T2> newValues;
	bool allOK = true;
	for(typename std::vector<T1>::const_iterator it = source.begin(); it != source.end(); it++) {
		if (map.count(*it)) {
			newValues.insert(map.at(*it));
		}
		else
			allOK = false;
	}
	dest = std::vector<T2>(newValues.begin(), newValues.end());
	return allOK;
}

/** Remaps everything in source with map into dest, source and dest can be equal */
template<typename T1, typename T2>
bool remapSet(const std::set<T1> & source, std::set<T2> & dest, const std::unordered_map<T1, T2> & map) {
	std::set<T2> newValues;
	bool allOK = true;
	for(typename std::set<T1>::const_iterator it = source.begin(); it != source.end(); it++) {
		if (map.count(*it)) {
			newValues.insert(map.at(*it));
		}
		else
			allOK = false;
	}
	dest.swap(newValues);
	return allOK;
}


/** Remaps everything in source with map into dest, source and dest can be equal */
template<typename T1, typename T2>
bool remapDeque(const std::deque<T1> & source, std::deque<T2> & dest, const std::unordered_map<T1, T2> & map) {
	std::deque<T2> newValues;
	bool allOK = true;
	for(typename std::deque<T1>::const_iterator it = source.begin(); it != source.end(); it++) {
		if (map.count(*it)) {
			newValues.push_back(map.at(*it));
		}
		else
			allOK = false;
	}
	dest.swap(newValues);
	return allOK;
}

template<typename T1, typename T2, template <typename> class SourceDestContainer>
bool remap(const SourceDestContainer<T1> & source, SourceDestContainer<T2> & dest, const std::unordered_map<T1, T2> & map) {
	SourceDestContainer<T2> newValues;
	bool allOK = true;
	for(typename SourceDestContainer<T1>::const_iterator it = source.begin(); it != source.end(); it++) {
		if (map.count(*it)) {
			newValues.insert(map.at(*it));
		}
		else
			allOK = false;
	}
	dest.swap(newValues);
	return allOK;
}

template<typename T1, typename T2>
bool remap(const std::vector<T1> & source, std::vector<T2> & dest, const std::unordered_map<T1, T2> & map) {
	std::vector<T2> newValues;
	newValues.reserve(source.size());
	bool allOK = true;
	for(typename std::vector<T1>::const_iterator it = source.begin(); it != source.end(); it++) {
		if (map.count(*it)) {
			newValues.push_back(map.at(*it));
		}
		else
			allOK = false;
	}
	dest.swap(newValues);
	return allOK;
}

/** @return false if remapping was not bijective */
template<typename T1, typename T2>
bool invert(const std::map<T1, T2> & source, std::map<T2, T1> & dest) {
	std::map<T2, T1> newValues;
	bool allOk = true;
	for(typename std::map<T1, T2>::const_iterator it = source.begin(); it != source.end(); it++) {
		allOk = (allOk && (newValues.count(it->second) == 0));
		newValues[it->second] = it->first;
	}
	dest.swap(newValues);
	return allOk;
}

/** @return false if remapping was not bijective */
template<typename T1, typename T2>
bool invert(const std::unordered_map<T1, T2> & source, std::unordered_map<T2, T1> & dest) {
	std::unordered_map<T2, T1> newValues;
	bool allOk = true;
	for(typename std::unordered_map<T1, T2>::const_iterator it = source.begin(); it != source.end(); it++) {
		allOk = (allOk && (newValues.count(it->second) == 0));
		newValues[it->second] = it->first;
	}
	dest.swap(newValues);
	return allOk;
}


template<typename T>
bool haveCommonValue(const std::set<T> & a, const std::set<T> & b) {
	if (a.size() == 0 || b.size() == 0) {
		return false;
	}
	typename std::set<T>::const_iterator aIndexIt(a.begin());
	typename std::set<T>::const_iterator bIndexIt(b.begin());
	while (aIndexIt != a.end() && bIndexIt != b.end()) {
		T aItemId = *aIndexIt;
		T bItemId = *bIndexIt;
		if (aItemId == bItemId) {
			return true;
		}
		else if (aItemId < bItemId) {
			aIndexIt++;
		}
		else { //bItemId is smaller
			bIndexIt++;
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

// bool haveCommonValue(const std::deque<uint32_t> & a, const ItemIndex & b);

template<typename T>
int find(const std::deque< T > & src, const T & k) {
	for(size_t i = 0; i < src.size(); i++) {
		if (src[i] == k)
			return i;
	}
	return -1;
}

template<typename T_OUT_CONTAINER, typename T, typename TCONTAINER>
T_OUT_CONTAINER toMapTable(const TCONTAINER & s) {
	T_OUT_CONTAINER r;
	for(size_t i = 0; i < s.size(); i++) {
		r[ s[i] ] = i;
	}
	return r;
}

///creates for i in 0..s.size: std::unordered_map[s[i]] = i
template<typename T, typename TCONTAINER>
std::unordered_map<T, uint32_t> unordered_mapTableFromLinearContainer(const TCONTAINER & s) {
	std::unordered_map<T, uint32_t> r;
	for(size_t i = 0; i < s.size(); i++) {
		r[ s[i] ] = i;
	}
	return r;
}

///creates for i in 0..s.size: std::map[s[i]] = i
template<typename T, typename TCONTAINER>
std::map<T, uint32_t> mapTableFromLinearContainer(const TCONTAINER & s) {
	std::map<T, uint32_t> r;
	for(size_t i = 0; i < s.size(); i++) {
		r[ s[i] ] = i;
	}
	return r;
}

template<typename T>
void diffSortedContainer(std::set<T> & out, const std::set<T> & a, const std::set<T> & b) {
	std::set<T> result;
	std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::insert_iterator< std::set<T> >(result, result.end()));
	out.swap(result);
}

template<typename T_CONTAINER>
void compactifyStlContainer(T_CONTAINER & c) {
	T_CONTAINER r(c.begin(), c.end());
	r.swap(c);
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
	
	out.swap(result);
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
	out.swap(result);
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

template<typename T>
void appendOrInsert(const T & src, std::deque<T> & dest) {
	dest.push_back(src);
}

template<typename T>
void appendOrInsert(const T & src, std::vector<T> & dest) {
	dest.push_back(src);
}

template<typename T>
void appendOrInsert(const T & src, std::set<T> & dest) {
	dest.insert(dest.end(), src);
}

template<typename T>
void reserveMemory(std::vector<T> & dest, size_t size) {
	dest.reserve(size);
}

template<typename T_CONTAINER>
void reserveMemory(T_CONTAINER & dest, size_t size) {
	;
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
	result.swap(out);
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
	result.swap(out);
}

void mergeSortedContainer(std::vector<uint32_t> & out, const std::vector<uint32_t> & a, const std::vector<uint32_t> & b);

template<typename T_SORTED_CONTAINER_A, typename T_SORTED_CONTAINER_B = T_SORTED_CONTAINER_A, typename T_OUT_CONTAINER = T_SORTED_CONTAINER_A>
T_OUT_CONTAINER intersect(const T_SORTED_CONTAINER_A & a, T_SORTED_CONTAINER_B & b) {
	T_OUT_CONTAINER ret;
	std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(ret, ret.end()));
	return ret;
}

inline int8_t minStorageBytesOfValue(uint32_t v) {
	if (v <= 0xFF) return 1;
	if (v <= 0xFFFF) return 2;
	if (v <= 0xFFFFFF) return 3;
	return 4;
}

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

template<typename T_ITERATOR, typename T_RETURN = typename std::iterator_traits<T_ITERATOR>::value_type, typename T_FUNC>
T_RETURN treeMap(const T_ITERATOR & begin, const T_ITERATOR & end, T_FUNC mapFunc) {
	if (end - begin == 1) {
		return *begin;
	}
	else if (end - begin == 2) {
		return mapFunc(*begin, *end);
	}
	else {
		return mapFunc( treeMap<T_ITERATOR, T_FUNC>(begin, begin+(end-begin)/2),
						treeMap<T_ITERATOR, T_FUNC>(begin+(end-begin)/2, end)
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