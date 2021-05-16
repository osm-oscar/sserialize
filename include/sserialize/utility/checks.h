#ifndef SSERIALIZE_CHECKS_H
#define SSERIALIZE_CHECKS_H
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/constants.h>
#include <cmath>
#include <limits>
#include <type_traits>

namespace sserialize {

template<typename To, typename From, typename Enable = void>
struct NarrowCheck;

///Float specializations
template<typename To, typename From>
struct NarrowCheck<To, From, std::enable_if_t<(std::is_integral_v<To> && std::is_floating_point_v<From>)> > {
	static To cast(From value) {
		if (UNLIKELY_BRANCH(value < std::ceil(From(std::numeric_limits<To>::min())) || value > std::floor(From(std::numeric_limits<To>::max())))) {
			throw sserialize::TypeOverflowException("out of range");
		}
		return static_cast<To>(value);
	}
};

//Integer variants

template<typename To, typename From>
struct NarrowCheck<To, From, std::enable_if_t<(std::is_signed_v<To> && std::is_signed_v<From> && std::is_integral_v<From>)> > {
	static To cast(From value) {
		if (UNLIKELY_BRANCH(value < std::numeric_limits<To>::min() || value > std::numeric_limits<To>::max())) {
			throw sserialize::TypeOverflowException("out of range");
		}
		return static_cast<To>(value);
	}
};

template<typename To, typename From>
struct NarrowCheck<To, From, std::enable_if_t<(std::is_signed_v<To> && std::is_unsigned_v<From> && std::is_integral_v<From>)> > {
	static To cast(From value) {
		if (UNLIKELY_BRANCH(value > static_cast<typename std::make_unsigned<To>::type>(std::numeric_limits<To>::max()))) {
			throw sserialize::TypeOverflowException("out of range");
		}
		return static_cast<To>(value);
	}
};

template<typename To, typename From>
struct NarrowCheck<To, From, std::enable_if_t<(std::is_unsigned_v<To> && std::is_signed_v<From> && std::is_integral_v<From>)> > {
	static To cast(From value) {
		if (UNLIKELY_BRANCH(value < 0 || static_cast<typename std::make_unsigned<From>::type>(value) > std::numeric_limits<To>::max())) {
			throw sserialize::TypeOverflowException("out of range");
		}
		return static_cast<To>(value);
	}
};

template<typename To, typename From>
struct NarrowCheck<To, From, std::enable_if_t<(std::is_unsigned_v<To> && std::is_unsigned_v<From> && std::is_integral_v<From>)> > {
	static To cast(From value) {
		if (UNLIKELY_BRANCH(value > std::numeric_limits<To>::max())) {
			throw sserialize::TypeOverflowException("out of range");
		}
		return static_cast<To>(value);
	}
};

template<typename I, typename J>
auto
narrow_check(J value) {
	return NarrowCheck<I, J>::cast(value);
}

template<typename I, typename J>
auto
narrow(J value) {
	return narrow_check<I>(value);
}

///assign lhs to rhs. Use it like narrow_check_assign(lhs, rhs);
template<typename I, typename J>
void narrow_check_assign(I & lhs, const J & rhs) {
	lhs = narrow_check<I, J>(rhs);
}

namespace detail {

	template<typename I>
	class NarrowCheckAssigner {
	private:
		I & m_lhs;
	public:
		NarrowCheckAssigner(I & lhs) : m_lhs(lhs) {}
		
		template<typename J>
		inline I & operator=(const J & rhs) {
			m_lhs = sserialize::narrow_check<I>(rhs);
			return m_lhs;
		}
	};
} //end namespace detail


///Returns a simple wrapper, you can use like this: narrow_check_assign(lhs) = rhs;
template<typename I> 
detail::NarrowCheckAssigner<I> narrow_check_assign(I & lhs) {
	return detail::NarrowCheckAssigner<I>(lhs);
}

template<typename I, typename J>
inline auto checked_add(I const & first, J const & second) {
	decltype(first+second) result;
	if (__builtin_add_overflow(first, second, &result)) {
		throw sserialize::TypeOverflowException("Add overflows");
	}
	return result;
}

template<typename I, typename J>
inline auto checked_mult(I const & first, J const & second) {
	decltype(first*second) result;
	if (__builtin_mul_overflow(first, second, &result)) {
		throw sserialize::TypeOverflowException("Multi overflows");
	}
	return result;
}

template<typename I, typename J>
inline auto checked_sub(I const & first, J const & second) {
	decltype(first-second) result;
	if (__builtin_sub_overflow(first, second, &result)) {
		throw sserialize::TypeOverflowException("Sub overflows");
	}
	return result;
}

namespace checked {
	
template<typename I, typename J>
inline auto add(I const & first, J const & second) {
	return checked_add(first, second);
}

template<typename I, typename J>
inline auto mult(I const & first, J const & second) {
	return checked_mult(first, second);
}

template<typename I, typename J>
inline auto sub(I const & first, J const & second) {
	return checked_sub(first, second);
}

} //end namespace checked

namespace unchecked {

template<typename I, typename J>
inline auto add(I const & first, J const & second) {
	return first+second;
}

template<typename I, typename J>
inline auto mult(I const & first, J const & second) {
	return first*second;
}

template<typename I, typename J>
inline auto sub(I const & first, J const & second) {
	return first-second;
}

} //end namespace unchecked


}//end namespace sserialize

#endif
