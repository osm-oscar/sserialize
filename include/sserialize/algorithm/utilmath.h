#ifndef SSERIALIZE_UTIL_MATH_H
#define SSERIALIZE_UTIL_MATH_H
#include <cmath>
#include <limits>
#include <complex>
#include <stdint.h>
#include <atomic>
#include <functional>
#include <type_traits>
#include <sserialize/utility/types.h>

namespace sserialize {

///does a*b/c correctly if the result is smaller than uint32_t
inline uint32_t multiplyDiv32(uint32_t a, uint32_t b, uint32_t c) {
	return (uint32_t)((static_cast<uint64_t>(a)*b)/c);
}

///does (a*b)%c correctly if the result is smaller than uint32_t
inline uint32_t multiplyMod32(uint32_t a, uint32_t b, uint32_t c) {
	return (uint32_t)((static_cast<uint64_t>(a)*b)%c);
}

inline uint64_t multiplyDiv64(uint64_t a, uint64_t b, uint32_t c) {
	uint64_t ab;
	if (!__builtin_mul_overflow(a, b, &ab)) {
		return ab / c;
	}
	else {
		#if defined(__SIZEOF_INT128__) && defined(__GNUC__) && ! defined(__clang__)
		using int128 = __int128_t;
		return ( int128(a)*int128(b) ) / uint64_t(c);
		#else 
		uint64_t a_c = a/c;
		uint64_t r_a = a%c;
		uint64_t b_c = b/c;
		uint64_t r_b = b%c;
		return a_c*b_c*c+a_c*r_b+b_c*r_a+(r_a*r_b)/c;
		#endif
	}
}

///correct if log2(a%c * b%c) <= 64
inline uint64_t multiplyMod64(uint64_t a, uint64_t b, uint32_t c) {
	uint64_t ab;
	if (!__builtin_mul_overflow(a, b, &ab)) {
		return ab%c;
	}
	else {
		#if defined(__SIZEOF_INT128__) && defined(__GNUC__) && ! defined(__clang__)
		using int128 = __int128_t;
		return ( int128(a)*int128(b) ) % c;
		#else 
		uint64_t r_a = a%c;
		uint64_t r_b = b%c;
		return (r_a*r_b)%c;
		#endif
	}
}

/** @return [-1, 0, 1]
   * @src http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
   */
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

template<typename T>
inline uint32_t popCount(T v);

template<>
inline uint32_t popCount<int>(int v) {
	return (uint32_t) __builtin_popcount((unsigned int)v);
}

template<>
inline uint32_t popCount<uint8_t>(uint8_t v) {
	return (uint32_t) __builtin_popcount(v);
}

template<>
inline uint32_t popCount<uint16_t>(uint16_t v) {
	return (uint32_t) __builtin_popcount(v);
}

template<>
inline uint32_t popCount<uint32_t>(uint32_t v) {
	return (uint32_t) __builtin_popcount(v);
}

template<>
inline uint32_t popCount<uint64_t>(uint64_t v) {
	return (uint32_t) __builtin_popcountll(v);
}

double inline logTo2(double num) {
#ifdef __ANDROID__
	return std::log(num)/0.6931471805599453;;
#else
	return std::log2(num);
#endif
}

inline constexpr uint32_t createMask(uint32_t bpn) {
	return ((bpn == 32) ? std::numeric_limits<uint32_t>::max() : ((static_cast<uint32_t>(1) << bpn) - 1));
}

inline constexpr uint64_t createMask64(uint64_t bpn) {
	return ((bpn == 64) ? std::numeric_limits<uint64_t>::max() : ((static_cast<uint64_t>(1) << bpn) - 1));
}

inline uint32_t saturatedAdd32(const uint32_t a, const uint32_t b) {
	return (a > std::numeric_limits<uint32_t>::max() - b) ? std::numeric_limits<uint32_t>::max() : a + b;
}

///@return position of the most significant bit starting with 0 or 0 if non is present
template<typename TValue>
inline
typename std::enable_if<std::is_integral<TValue>::value && std::is_unsigned<TValue>::value, uint32_t>::type 
msb(TValue num) {
	uint32_t r = 0;
	while (num >>= 1) {
		++r;
	}
	return r;
}

uint32_t msb(uint32_t v);
uint32_t msb(uint64_t v);

//from http://stereopsis.com/log2.html
inline int32_t fastLog2(uint32_t x)  {
#if defined(__amd64__)
	int32_t retval;
	asm("bsr %%eax, %1;"
		"mov %0, %%eax"
		: "=r"(retval)
		: "r"(x)
		: "%eax"
	);
	return retval;
#else
	return msb(x);
#endif
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

template<typename T>
class MinMax final {
public:
	typedef T value_type;
private:
	value_type m_max;
	value_type m_min;
public:
	MinMax(const value_type & minInitial, const value_type & maxInitial) : m_max(maxInitial), m_min(minInitial) {}
	MinMax() : m_max(std::numeric_limits<value_type>::min()), m_min(std::numeric_limits<value_type>::max()) {}
	~MinMax() {}
	inline const value_type & min() const { return m_min; }
	inline const value_type & max() const { return m_max; }
	void update(const value_type & v) {
		m_max = std::max<value_type>(m_max, v);
		m_min = std::min<value_type>(m_min, v);
	}
	void update(const MinMax & o) {
		m_max = std::max<value_type>(m_max, o.m_max);
		m_min = std::min<value_type>(m_min, o.m_min);
	}
	void reset() {
		m_max = std::numeric_limits<value_type>::min();
		m_min = std::numeric_limits<value_type>::max();
	}
};

template<typename T>
class AtomicMinMax final {
public:
	typedef T value_type;
private:
	AtomicMax<value_type> m_max;
	AtomicMin<value_type> m_min;
public:
	AtomicMinMax(const value_type & minInitial, const value_type & maxInitial) : m_max(maxInitial), m_min(minInitial) {}
	AtomicMinMax() {}
	~AtomicMinMax() {}
	inline value_type min() const { return m_min.load(); }
	inline value_type max() const { return m_max.load(); }
	void update(const value_type & v) {
		m_max.update(v);
		m_min.update(v);
	}
	void update(const AtomicMinMax<value_type> & o) {
		m_max.update(o.max());
		m_min.update(o.min());
	}
	void update(const MinMax<value_type> & o) {
		m_max.update(o.max());
		m_min.update(o.min());
	}
};

}//end namespace

#endif
