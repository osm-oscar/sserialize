#ifndef SSERIALIZE_HASH_SPECIALIZATIONS_H
#define SSERIALIZE_HASH_SPECIALIZATIONS_H
#include <string>
#include <vector>
#include <functional>


//from http://stackoverflow.com/questions/7222143/unordered-map-hash-function-c, which is from boost

template <class T>
inline void hash_combine(std::size_t & seed, const T & v) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline void hash_combine(uint64_t & seed, const uint8_t v) {
	seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace sserialize {

template<typename TPtr, typename T>
struct DerefHasher {
	std::hash<T> hasher;
	inline size_t operator()(const TPtr & t) const {
		return hasher(*t);
	}
};

template<typename TPtr>
struct DerefEq {
	inline bool operator()(const TPtr & t1, const TPtr & t2) const {
		return *t1 == *t2;
	}
};

template<typename TPtr>
struct DerefSmaler {
	inline bool operator()(const TPtr & t1, const TPtr & t2) const {
		return *t1 < *t2;
	}
};

}//end namespace sserialize


namespace std {

	template<>
	struct hash< std::pair<std::string, std::string> > {
		inline size_t operator()(const std::pair<std::string, std::string> & v) const {
			size_t seed = 0;
			::hash_combine(seed, v.first);
			::hash_combine(seed, v.second);
			return seed;
		}
	};

	template<typename S, typename V> 
	struct hash< std::pair<S, V> > {
		inline size_t operator()(const pair<S, V> & v) const {
			size_t seed = 0;
			::hash_combine(seed, v.first);
			::hash_combine(seed, v.second);
			return seed;
		}
	};

	template<typename T>
	struct hash< std::vector<T> > {
		inline size_t operator()(const std::vector<T> & v) const {
			size_t seed = 0;
			for(typename std::vector<T>::const_iterator it(v.begin()), end(v.end()); it != end; ++it) {
				::hash_combine(seed, *it);
			}
			return seed;
		}
	};

}

#endif