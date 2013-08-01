#ifndef SSERIALIZE_UTIL_CONTAINER_FUNCS_H
#define SSERIALIZE_UTIL_CONTAINER_FUNCS_H
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <vector>
#include <deque>

namespace sserialize {

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


}//end namespace

#endif