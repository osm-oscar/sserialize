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
	inline Size operator __W ## = (I v) { \
		return m_v __W v; \
		return *this; \
	} \
	template<typename I, std::enable_if_t< std::is_integral_v<I>, bool> = true> \
	inline Size operator __W (I v) { return Size(m_v __W v); }
	
	OP(*)
	OP(+)
	OP(/)
	OP(-)
#undef OP
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
public:
	inline operator underlying_type() const { return m_v; }
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

template<typename I, typename J>
inline
typename std::enable_if<std::is_same<Size, I>::value && std::is_unsigned<J>::value, I>::type
narrow_check(J value) {
	if (UNLIKELY_BRANCH(value > sserialize::createMask64(SerializationInfo<Size>::length*8))) {
		throw sserialize::TypeOverflowException("out of range");
	}
	return static_cast<I>(value);
}

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
