#ifndef SSERIALIZE_UTIL_MATH_H
#define SSERIALIZE_UTIL_MATH_H
#include <cmath>
#include <limits>
#include <complex>
#include <stdint.h>
#include <sserialize/utility/types.h>

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

}//end namespace

#endif