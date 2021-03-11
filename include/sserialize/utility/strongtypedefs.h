#pragma once
#include <sserialize/storage/UByteArrayAdapter.h>

//TODO: Add noexcept specifier, perfect forwarding if neccessary

namespace sserialize::st {
	
template<typename T>
struct underlying_type {
	using type = typename T::underlying_type;
};

template<typename T>
struct underlying_type_accessor {
	using type = typename underlying_type<T>::type;
	static inline type const & get(T const & v) { return static_cast<type&>(v); }
	static inline type & get(T & v) { return static_cast<type&>(v); }
};

#define UNDERLYING_TYPE_HELPERS \
using underlying_type = typename sserialize::st::underlying_type<Derived>::type; \
inline underlying_type & ut() { return sserialize::st::underlying_type_accessor<Derived>::get(*this); } \
inline underlying_type const & ut() const { return sserialize::st::underlying_type_accessor<Derived>::get(*this); }


// #define UNDERLYING_TYPE_HELPERS \
// using underlying_type = typename sserialize::st::underlying_type<Derived>::type; \
// inline underlying_type & ut() { return static_cast<underlying_type&>(*this); } \
// inline underlying_type const & ut() const { return static_cast<underlying_type const&>(*this); }

template<
	typename Derived,
	typename underlying_type = typename sserialize::st::underlying_type<Derived>::type,
	typename underlying_type_accessor = sserialize::st::underlying_type_accessor<Derived>
>
class Special {
private:
	inline underlying_type & ut() { return underlying_type_accessor::get(*this); } \
	inline underlying_type const & ut() const { return underlying_type_accessor::get(*this); }
public:
	inline Derived operator+(Derived const & o) const { return Derived( ut() + o.ut() ); }
	inline Derived & operator++() { ++ut(); return static_cast<Derived>(*this); }
	inline Derived operator++(int) { return Derived(ut()++); }
	inline Derived & operator+=(Derived const & o) { ut() += o.ut(); return static_cast<Derived>(*this); }
};

template<typename Derived>
class Add {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline Derived operator+(Derived const & o) const { return Derived( ut() + o.ut() ); }
	inline Derived & operator++() { ++ut(); return static_cast<Derived>(*this); }
	inline Derived operator++(int) { return Derived(ut()++); }
	inline Derived & operator+=(Derived const & o) { ut() += o.ut(); return static_cast<Derived>(*this); }
};

template<typename Derived>
class Sub {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline Derived operator-(Derived const & o) const { return Derived( ut() - o.ut() ); }
	inline Derived & operator--() { --ut(); return static_cast<Derived>(*this); }
	inline Derived operator--(int) { return Derived(ut()--); }
	inline Derived & operator-=(Derived const & o) { ut() -= o.ut(); return static_cast<Derived>(*this); }
};

#define BIN_OPS(__NAME, __OPSYM) \
template<typename Derived> \
class __NAME { \
private: \
	UNDERLYING_TYPE_HELPERS \
public: \
	inline Derived operator __OPSYM (Derived const & o) const { return Derived( ut() __OPSYM o.ut() ); } \
	inline Derived & operator __OPSYM ## = (Derived const & o) { ut() __OPSYM ##= o.ut(); return static_cast<Derived>(*this); } \
};

BIN_OPS(Mult, *)
BIN_OPS(Div, /)
BIN_OPS(Mod, %)
BIN_OPS(LShift, <<)
BIN_OPS(RShift, >>)
BIN_OPS(BitOr, |)
BIN_OPS(BitAnd, &)
BIN_OPS(BitXor, ^)
#undef BIN_OPS

#define BIN_OPS(__NAME, __OPSYM) \
template<typename Derived> \
class __NAME { \
private: \
	UNDERLYING_TYPE_HELPERS \
public: \
	inline Derived operator __OPSYM (Derived const & o) const { return Derived( ut() __OPSYM o.ut() ); } \
};
BIN_OPS(And, &&)
BIN_OPS(Or, ||)
#undef BIN_OPS

#define BIN_OPS(__NAME, __OPSYM) \
template<typename Derived> \
class __NAME { \
private: \
	UNDERLYING_TYPE_HELPERS \
public: \
	inline bool operator __OPSYM (Derived const & o) const { return ut() __OPSYM o.ut(); } \
};
BIN_OPS(CompareEqual, ==)
BIN_OPS(CompareNotEqual, !=)
BIN_OPS(CompareLess, <)
BIN_OPS(CompareLessEqual, <=)
BIN_OPS(CompareGreater, >)
BIN_OPS(CompareGreaterEqual, >=)
#undef BIN_OPS

#define UN_OPS(__NAME, __OPSYM) \
template<typename Derived> \
class __NAME { \
private: \
	UNDERLYING_TYPE_HELPERS \
public: \
	inline Derived operator __OPSYM () const { return Derived( __OPSYM ut() ); } \
};

UN_OPS(BitInvert, ~)
UN_OPS(Negate, -)
UN_OPS(Not, !)

#undef UN_OPS

template<typename Derived>
class Arithmetic:
	public Add<Derived>,
	public Sub<Derived>,
	public Mult<Derived>,
	public Div<Derived>,
	public Mod<Derived>,
	public Negate<Derived>
{};

template<typename Derived>
class BitBased:
	public LShift<Derived>,
	public RShift<Derived>,
	public BitOr<Derived>,
	public BitAnd<Derived>,
	public BitXor<Derived>,
	public BitInvert<Derived>
{};

template<typename Derived>
class BooleanLike:
	public And<Derived>,
	public Or<Derived>,
	public Not<Derived>
{};

template<typename Derived>
class Serialize {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline friend sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, Derived const & src) {
		return dest << src.ut();
	}
};

template<typename Derived>
class Deserialize {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline friend sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & src, Derived & dest) {
		return dest >> dest.ut();
	}
};

}//end namespace sserialize::st
