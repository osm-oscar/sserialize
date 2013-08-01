#ifndef SSERIALIZE_UTIL_MEMORY_H
#define SSERIALIZE_UTIL_MEMORY_H
#include <set>
#include <deque>
#include <vector>
#include <stdint.h>


namespace sserialize {

template<typename T_CONTAINER>
void compactifyStlContainer(T_CONTAINER & c) {
	T_CONTAINER r(c.begin(), c.end());
	r.swap(c);
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
void reserveMemory(std::vector<T> & dest, std::size_t size) {
	dest.reserve(size);
}

template<typename T_CONTAINER>
void reserveMemory(T_CONTAINER & dest, std::size_t size) {
	;
}

}//end namespace

#endif