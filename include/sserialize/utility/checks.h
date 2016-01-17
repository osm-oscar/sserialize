#ifndef SSERIALIZE_CHECKS_H
#define SSERIALIZE_CHECKS_H
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/constants.h>
#include <cmath>
#include <limits>
#include <type_traits>

namespace sserialize {

template<typename I>
inline
typename std::enable_if<std::is_unsigned<I>::value, I>::type
narrow_check(double value) {
	if (UNLIKELY_BRANCH(value < 0.0 || value > std::floor((double)std::numeric_limits<I>::max()))) {
		throw sserialize::TypeOverflowException("out of range");
	}
	return static_cast<I>(value);
}

template<typename I>
inline
typename std::enable_if<std::is_unsigned<I>::value, I>::type
narrow_check(float value) {
	if (UNLIKELY_BRANCH(value < 0.0 || value > std::floor((float)std::numeric_limits<I>::max()))) {
		throw sserialize::TypeOverflowException("out of range");
	}
	return static_cast<I>(value);
}

template<typename I, typename J>
inline
typename std::enable_if<std::is_signed<I>::value && std::is_signed<J>::value, I>::type
narrow_check(J value) {
	if (UNLIKELY_BRANCH(value < std::numeric_limits<I>::min() || value > std::numeric_limits<I>::max())) {
		throw sserialize::TypeOverflowException("out of range");
	}
	return static_cast<I>(value);
}

template<typename I, typename J>
inline
typename std::enable_if<std::is_signed<I>::value && std::is_unsigned<J>::value, I>::type
narrow_check(J value) {
	if (UNLIKELY_BRANCH(value > static_cast<typename std::make_unsigned<I>::type>(std::numeric_limits<I>::max()))) {
		throw sserialize::TypeOverflowException("out of range");
	}
	return static_cast<I>(value);
}

template<typename I, typename J>
inline
typename std::enable_if<std::is_unsigned<I>::value && std::is_signed<J>::value, I>::type
narrow_check(J value) {
	if (UNLIKELY_BRANCH(value < 0 || static_cast<typename std::make_unsigned<J>::type>(value) > std::numeric_limits<I>::max())) {
		throw sserialize::TypeOverflowException("out of range");
	}
	return static_cast<I>(value);
}

template<typename I, typename J>
inline
typename std::enable_if<std::is_unsigned<I>::value && std::is_unsigned<J>::value, I>::type
narrow_check(J value) {
	if (UNLIKELY_BRANCH(value > std::numeric_limits<I>::max())) {
		throw sserialize::TypeOverflowException("out of range");
	}
	return static_cast<I>(value);
}

}//end namespace sserialize

#endif