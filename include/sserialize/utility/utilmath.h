#ifndef SSERIALIZE_UTIL_MATH_H
#define SSERIALIZE_UTIL_MATH_H
#include <cmath>
#include <limits>
#include <complex>
#include <stdint.h>
#include <atomic>
#include <sserialize/utility/types.h>

namespace sserialize {

///does a*b/c correctly if the result is smaller thant uint32_t
inline uint32_t multiplyDiv32(uint32_t a, uint32_t b, uint32_t c) {
	return (static_cast<uint64_t>(a)*b)/c;
}

///does (a*b)%c correctly if the result is smaller than uint32_t
inline uint32_t multiplyMod32(uint32_t a, uint32_t b, uint32_t c) {
	return (static_cast<uint64_t>(a)*b)%c;
}

inline uint64_t multiplyDiv64(uint64_t a, uint64_t b, uint32_t c) {
	uint64_t a_c = a/c;
	uint64_t r_a = a%c;
	uint64_t b_c = b/c;
	uint64_t r_b = b%c;
	return a_c*b_c*c+a_c*r_b+b_c*r_a+(r_a*r_b)/c;
}

///correct if log2(a%c * b%c) <= 64
inline uint64_t multiplyMod64(uint64_t a, uint64_t b, uint32_t c) {
	uint64_t r_a = a%c;
	uint64_t r_b = b%c;
	return (r_a*r_b)%c;
}

/** @return [-1, 0, 1]
   * @src http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
   */
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

template<typename T>
inline int popCount(T v);

template<>
inline int popCount<int>(int v) {
	return __builtin_popcount((unsigned int)v);
}

template<>
inline int popCount<uint8_t>(uint8_t v) {
	return __builtin_popcount(v);
}

template<>
inline int popCount<uint16_t>(uint16_t v) {
	return __builtin_popcount(v);
}

template<>
inline int popCount<uint32_t>(uint32_t v) {
	return __builtin_popcount(v);
}

template<>
inline int popCount<uint64_t>(uint64_t v) {
	return __builtin_popcountll(v);
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

///@return position of the most significant bit, returns 0 iff num == 0 (index starts at 1)
inline uint8_t msb(uint8_t num) {
	uint8_t r = 0;
	while (num >>= 1) {
		++r;
	}
	return r;
}

///@return position of the most significant bit, returns 0 iff num == 0 (index starts at 1)
inline uint8_t msb(uint16_t num) {
	uint8_t r = 0;
	while (num >>= 1) {
		++r;
	}
	return r;
}

///@return position of the most significant bit, returns 0 iff num == 0 (index starts at 1)
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

inline int8_t minStorageBytesOfValue(uint32_t v) {
	int8_t ret = 1;
	v >>= 8;
	while(v) {
		++ret;
		v >>= 8;
	}
	return ret;
}

inline bool geoEq(const double a, const double b) {
	return std::abs<double>(a-b) < EPSILON;
}

inline bool geoNeq(const double a, const double b) {
	return std::abs<double>(a-b) >= EPSILON;
}

template<typename T>
class AtomicMax final {
	std::atomic<T> m_v;
public:
	AtomicMax(const T & initial) : m_v(initial) {}
	AtomicMax() : m_v(std::numeric_limits<T>::min()) {}
	~AtomicMax() {}
	T load() const { return m_v.load(); }
	std::atomic<T> & value() { return m_v; }
	const std::atomic<T> & value() const { return m_v; }
	void update(const T & v) {
		T prevV = m_v;
		while(prevV < v && !m_v.compare_exchange_weak(prevV, v));
	}
};

template<typename T>
class AtomicMin final {
	std::atomic<T> m_v;
public:
	AtomicMin(const T & initial) : m_v(initial) {}
	AtomicMin() : m_v(std::numeric_limits<T>::max()) {}
	~AtomicMin() {}
	T load() const { return m_v.load(); }
	std::atomic<T> & value() { return m_v; }
	const std::atomic<T> & value() const { return m_v; }
	void update(const T & v) {
		T prevV = m_v;
		while(prevV > v && !m_v.compare_exchange_weak(prevV, v));
	}
};

}//end namespace

#endif