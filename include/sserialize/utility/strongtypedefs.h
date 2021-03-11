#pragma once
#include <sserialize/storage/UByteArrayAdapter.h>

//TODO: Add noexcept specifier, perfect forwarding if neccessary
//TODO: Take a look at https://en.cppreference.com/w/cpp/language/operators to further improve operators
//AND to make unary/binary minus work: hint: mark operators as friend

namespace sserialize::st {
	
template<typename T>
struct underlying_type {
	using type = typename T::underlying_type;
};

template<typename T>
struct underlying_type_accessor {
	using type = typename underlying_type<T>::type;
	static inline type const & get(T const & v) { return static_cast<type const &>(v); }
	static inline type & get(T & v) { return static_cast<type &>(v); }
};

//We have to cast this to Derived first, since these functions are called within another context
#define UNDERLYING_TYPE_HELPERS \
inline underlying_type & ut() { return underlying_type_accessor::get(static_cast<Derived &>(*this)); } \
inline underlying_type const & cut() const { return underlying_type_accessor::get(static_cast<Derived const &>(*this)); }

#define TMPL_HEADER \
template< \
	typename Derived, \
	typename underlying_type = typename sserialize::st::underlying_type<Derived>::type, \
	typename underlying_type_accessor = sserialize::st::underlying_type_accessor<Derived> \
>

#define TMPL_VARS Derived, underlying_type, underlying_type_accessor

TMPL_HEADER
class Add {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline Derived operator+(Derived const & o) const {
		//We need to explicitly select the function in case there are functions with the same name
		//Note that o is a Derived class and not our own
		//We therefore should be able to use cut() on this, however they way it is now is more consistent and it looks the same everywhere
		return Derived( Add::cut() + o. Add::cut() );
	}
	inline Derived & operator++() { ++Add::ut(); return static_cast<Derived>(*this); }
	inline Derived operator++(int) { return Derived(Add::ut()++); }
	inline Derived & operator+=(Derived const & o) { Add::ut() += o. Add::cut(); return static_cast<Derived>(*this); }
};

TMPL_HEADER
class Sub {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline Derived operator-(Derived const & o) const { return Derived( Sub::cut() - o. Sub::cut() ); }
	inline Derived & operator--() { --Sub::ut(); return static_cast<Derived>(*this); }
	inline Derived operator--(int) { return Derived(Sub::ut()--); }
	inline Derived & operator-=(Derived const & o) { Sub::ut() -= o. Sub::cut(); return static_cast<Derived>(*this); }
};

#define BIN_OPS(__NAME, __OPSYM) \
TMPL_HEADER \
class __NAME { \
private: \
	UNDERLYING_TYPE_HELPERS \
public: \
	inline Derived operator __OPSYM (Derived const & o) const { return Derived( __NAME::cut() __OPSYM o. __NAME::cut() ); } \
	inline Derived & operator __OPSYM ## = (Derived const & o) { __NAME::ut() __OPSYM ##= o. __NAME::ut(); return static_cast<Derived>(*this); } \
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
TMPL_HEADER \
class __NAME { \
private: \
	UNDERLYING_TYPE_HELPERS \
public: \
	inline Derived operator __OPSYM (Derived const & o) const { return Derived( __NAME::cut() __OPSYM o. __NAME::cut() ); } \
};
BIN_OPS(And, &&)
BIN_OPS(Or, ||)
#undef BIN_OPS

#define BIN_OPS(__NAME, __OPSYM) \
TMPL_HEADER \
class __NAME { \
private: \
	UNDERLYING_TYPE_HELPERS \
public: \
	inline bool operator __OPSYM (Derived const & o) const { return __NAME::cut() __OPSYM o. __NAME::cut(); } \
};
BIN_OPS(CompareEqual, ==)
BIN_OPS(CompareNotEqual, !=)
BIN_OPS(CompareLess, <)
BIN_OPS(CompareLessEqual, <=)
BIN_OPS(CompareGreater, >)
BIN_OPS(CompareGreaterEqual, >=)
#undef BIN_OPS

#define UN_OPS(__NAME, __OPSYM) \
TMPL_HEADER \
class __NAME { \
private: \
	UNDERLYING_TYPE_HELPERS \
public: \
	inline Derived operator __OPSYM () const { return Derived( __OPSYM __NAME::cut() ); } \
};

UN_OPS(BitInvert, ~)
UN_OPS(Negate, -)
UN_OPS(Not, !)

#undef UN_OPS

TMPL_HEADER
class Arithmetic:
	public Add<TMPL_VARS>,
	public Sub<TMPL_VARS>,
	public Mult<TMPL_VARS>,
	public Div<TMPL_VARS>,
	public Mod<TMPL_VARS>
// 	,
// 	public Negate<TMPL_VARS>
{};

TMPL_HEADER
class BitBased:
	public LShift<TMPL_VARS>,
	public RShift<TMPL_VARS>,
	public BitOr<TMPL_VARS>,
	public BitAnd<TMPL_VARS>,
	public BitXor<TMPL_VARS>,
	public BitInvert<TMPL_VARS>
{};

TMPL_HEADER
class BooleanLike:
	public And<TMPL_VARS>,
	public Or<TMPL_VARS>,
	public Not<TMPL_VARS>
{};

TMPL_HEADER
class Serialize {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline friend sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, Derived const & src) {
		return dest << src.cut();
	}
};

TMPL_HEADER
class Deserialize {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline friend sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & src, Derived & dest) {
		return dest >> dest.ut();
	}
};

#undef UNDERLYING_TYPE_HELPERS
#undef TMPL_HEADER

}//end namespace sserialize::st
