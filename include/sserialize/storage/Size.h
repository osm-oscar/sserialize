#pragma once

#include <limits>

#include <sserialize/utility/constants.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/checks.h>
#include <sserialize/algorithm/utilmath.h>

namespace sserialize {
	
class UByteArrayAdapter;

class Size final {
public:
	using underlying_type = uint64_t;
public:
	constexpr Size() {}
	constexpr Size(Size const &) = default;
	constexpr Size(Size &&) = default;
	template<typename I, std::enable_if_t< std::is_integral_v<I>, bool> = true>
	constexpr Size(I v) :
	m_v(v)
	{}
	Size(UByteArrayAdapter const & src) : m_v(src.getOffset(0)) {}
	Size(UByteArrayAdapter & src, UByteArrayAdapter::ConsumeTag) : m_v(src.getOffset()) {}
	Size(UByteArrayAdapter const & src, UByteArrayAdapter::NoConsumeTag) : Size(src) {}
	constexpr ~Size() {}
	template<typename I, std::enable_if_t< std::is_integral_v<I>, bool> = true>
	constexpr inline Size & operator=(I v) {
		m_v = v;
		return *this;
	}
	constexpr Size & operator=(Size const&) = default;
	constexpr Size & operator=(Size &&) = default;
public:
	#define OP(__W) \
		friend inline bool operator __W(Size const & a, Size const & b) { return a.m_v __W b.m_v; } \
		template<typename I, std::enable_if_t< std::is_integral_v<I>, bool> = true> \
		friend inline bool operator __W(Size const & a, I b) { return a.m_v __W b; } \
		template<typename I, std::enable_if_t< std::is_integral_v<I>, bool> = true> \
		friend inline bool operator __W(I a, Size const & b) { return a __W b.m_v; }
		
	OP(!=)
	OP(==)
	OP(<)
	OP(<=)
	OP(>)
	OP(>=)
	#undef OP
public:
#define OP(__W) \
	inline Size & operator __W ## = (Size const & o) { \
		m_v __W ## = o.m_v; \
		return *this; \
	} \
	inline Size operator __W (Size const & o) const { return Size(m_v __W o.m_v); } \
	template<typename I, std::enable_if_t< std::is_integral_v<I>, bool> = true> \
	inline Size & operator __W ## = (I v) { \
		m_v __W ## = v; \
		return *this; \
	} \
	template<typename I, std::enable_if_t< std::is_integral_v<I>, bool> = true> \
	inline Size operator __W (I v) { return Size(m_v __W v); }
	
	OP(*)
	OP(+)
	OP(/)
	OP(-)
#undef OP

#define INTEGRAL_OPS(__TYPE, __OP) \
	inline Size & operator __OP ## = (__TYPE v) { \
		m_v __OP ## = v; \
		return *this; \
	} \
	inline Size operator __OP (__TYPE v) { return Size(m_v __OP v); }
	
#define INTEGRAL_CAST_OP(__TYPE) \
	inline operator __TYPE() const { return narrow_check<__TYPE>(m_v); }
	
#define FOR_EACH_OP(__TYPE) \
	INTEGRAL_CAST_OP(__TYPE)
/**
// 	INTEGRAL_OPS(__TYPE, *) \
// 	INTEGRAL_OPS(__TYPE, /) \
// 	INTEGRAL_OPS(__TYPE, +) \
// 	INTEGRAL_OPS(__TYPE, -) \
**/
	
// 	FOR_EACH_OP(uint8_t)
// 	FOR_EACH_OP(uint16_t)
// 	FOR_EACH_OP(uint32_t)
	FOR_EACH_OP(uint64_t)
// 	FOR_EACH_OP(int8_t)
// 	FOR_EACH_OP(int16_t)
// 	FOR_EACH_OP(int32_t)
// 	FOR_EACH_OP(int64_t)
#undef INTEGRAL_OPS
#undef FOR_EACH_OP
	
	Size & operator++() {
		++m_v;
		return *this;
	}
	Size operator++(int) {
		return Size(m_v++);
	}
	Size & operator--() {
		--m_v;
		return *this;
	}
	Size operator--(int) {
		return Size(m_v++);
	}
public:
	friend inline UByteArrayAdapter & operator<<(UByteArrayAdapter & dest, Size const & v) {
		dest.putOffset(v.m_v);
		return dest;
	}
	friend inline UByteArrayAdapter & operator>>(UByteArrayAdapter & src, Size & v) {
		v.m_v = src.getOffset();
		return src;
	}
private:
	underlying_type m_v{0};
};

template<>
struct SerializationInfo<Size> {
	using type = Size;
	static const bool is_fixed_length = true;
	static const OffsetType length = SSERIALIZE_OFFSET_BYTE_COUNT;
	static const OffsetType max_length = length;
	static const OffsetType min_length = length;
	static OffsetType sizeInBytes(const type &) {
		return length;
	}
};

SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(sserialize::Size, getOffset, putOffset)
}

namespace std {

template<>
struct numeric_limits<sserialize::Size>  {
	using value_type = sserialize::Size;
	using underlying_type = value_type::underlying_type;
	static constexpr bool is_specialized = true;
	static constexpr value_type min() { return value_type(underlying_type(0)); }
	static constexpr value_type max() { return value_type(underlying_type(0xFFFFFFFFFF)); }
};

} //end namespace std

namespace sserialize {
	

template<typename To>
struct NarrowCheck<To, sserialize::Size, void> {
	static To cast(sserialize::Size value) {
		return sserialize::narrow_check<To, sserialize::Size::underlying_type>(static_cast<sserialize::Size::underlying_type>(value));
	}
};

template<typename From>
struct NarrowCheck<sserialize::Size, From, void> {
	static sserialize::Size cast(From value) {
		return sserialize::Size( sserialize::narrow_check<sserialize::Size::underlying_type, From>(value) );
	}
};
	
}
