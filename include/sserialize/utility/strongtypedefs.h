#pragma once
#include <sserialize/storage/UByteArrayAdapter.h>
#include <utility>

//Strong typedef based on ideas seen at
//https://github.com/rollbear/strong_type
//https://github.com/foonathan/type_safe

namespace sserialize::st {

template<typename Derived, typename T>
class Tag {
	using underlying_type = T;
};

template<typename Derived, typename T>
T underlying_type_impl(Tag<Derived, T>*);

template<typename Derived>
struct underlying_type {
	using type = decltype( underlying_type_impl(static_cast<Derived*>(nullptr)) );
};

template<typename T>
struct underlying_type_accessor {
	using type = typename underlying_type<T>::type;
	static inline type const & get(T const & v) noexcept(true) { return static_cast<type const &>(v); }
	static inline type & get(T & v) noexcept(true) { return static_cast<type &>(v); }
};

template<
	typename Derived,
	typename underlying_type,
	typename underlying_type_accessor,
	class TModule
>
using Module = typename TModule::template type<Derived, underlying_type, underlying_type_accessor>;

template<
	typename Derived,
	typename underlying_type,
	typename underlying_type_accessor,
	typename... Modules
>
class strong_type:
	public Tag<Derived, underlying_type>,
	public Module<Derived, underlying_type, underlying_type_accessor, Modules>...
{};

///Strong typedef with underlying_type_accessor = sserialize::st::underlying_type_accessor<Derived>
template<
	typename Derived,
	typename underlying_type,
	typename... Modules
>
using strong_type_ca = strong_type<Derived, underlying_type, sserialize::st::underlying_type_accessor<Derived>, Modules...>;

///Strong typedef which also stores the type and allows an explicit cast to the underlying type
template<
	typename Derived,
	typename underlying_type,
	typename... Modules
>
class strong_type_store:
	public strong_type_ca<Derived, underlying_type, Modules...>
{
public:
	strong_type_store() :
	m_v()
	{}
	explicit strong_type_store(underlying_type const & v) noexcept(std::is_nothrow_move_constructible<underlying_type>::value) :
	m_v(v)
	{}
	explicit strong_type_store(underlying_type && v) :
	m_v(std::move(v))
	{}
public:
	explicit operator underlying_type&() { return m_v; }
	explicit operator underlying_type const &() const { return m_v; }
private:
	underlying_type m_v;
};

//We have to cast this to Derived first, since these functions are called within another context
#define UNDERLYING_TYPE_HELPERS \
inline underlying_type & ut() { return underlying_type_accessor::get(static_cast<Derived &>(*this)); } \
inline underlying_type const & cut() const { return underlying_type_accessor::get(static_cast<Derived const &>(*this)); }

#define TMPL_HEADER \
template< \
	typename Derived, \
	typename underlying_type, \
	typename underlying_type_accessor = sserialize::st::underlying_type_accessor<Derived> \
>

#define TMPL_VARS Derived, underlying_type, underlying_type_accessor

namespace impl {

TMPL_HEADER
class Store {
private:
	UNDERLYING_TYPE_HELPERS
public:
	Store() :
	m_v()
	{}
	explicit Store(underlying_type const & v) noexcept(std::is_nothrow_move_constructible<underlying_type>::value) :
	m_v(v)
	{}
	explicit Store(underlying_type && v) :
	m_v(std::move(v))
	{}
public:
	explicit operator underlying_type&() { return m_v; }
	explicit operator underlying_type const &() const { return m_v; }
private:
	underlying_type m_v;
};


TMPL_HEADER
class Add {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline Derived & operator++() noexcept( noexcept(++underlying_type()) ) { ++Add::ut(); return static_cast<Derived&>(*this); }
	inline Derived operator++(int) noexcept( noexcept(underlying_type()++) ) { return Derived(Add::ut()++); }
	inline Derived & operator+=(Derived const & o) noexcept( noexcept(Add::ut() += o. Add::cut()) ) {
		//We need to explicitly select the function in case there are functions with the same name
		//Note that o is a Derived class and not our own
		//We therefore should be able to use cut() on this, however the way it is now is more consistent and it looks the same everywhere
		Add::ut() += o. Add::cut();
		return static_cast<Derived&>(*this);
	}
	inline friend Derived operator+(Derived lhs, Derived const & rhs) noexcept( noexcept( lhs += rhs ) ) {
		lhs += rhs;
		return lhs;
	}
};

TMPL_HEADER
class Sub {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline Derived & operator--() noexcept( noexcept(--underlying_type()) ) { --Sub::ut(); return static_cast<Derived&>(*this); }
	inline Derived operator--(int) noexcept( noexcept(underlying_type()--) ) { return Derived(Sub::ut()--); }
	inline Derived & operator-=(Derived const & o) noexcept( noexcept(Sub::ut() -= o. Sub::cut()) ) { Sub::ut() -= o. Sub::cut(); return static_cast<Derived&>(*this); }
	inline friend Derived operator-(Derived lhs, Derived const & rhs) noexcept( noexcept( lhs -= rhs ) ) {
		lhs -= rhs;
		return lhs;
	}
};

#define BIN_OPS(__NAME, __OPSYM) \
TMPL_HEADER \
class __NAME { \
private: \
	UNDERLYING_TYPE_HELPERS \
public: \
	inline Derived & operator __OPSYM ## = (Derived const & o) noexcept( noexcept(__NAME::ut() __OPSYM ## = o. __NAME::cut()) ) { \
		__NAME::ut() __OPSYM ##= o. __NAME::cut(); \
		return static_cast<Derived&>(*this); \
	} \
	inline friend Derived operator __OPSYM (Derived lhs, Derived const & rhs) noexcept( noexcept( lhs __OPSYM ## = rhs ) ) { \
		lhs __OPSYM ## = rhs; \
		return lhs; \
	} \
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
	inline friend Derived operator __OPSYM (Derived const & lhs, Derived const & rhs) noexcept( noexcept( lhs. __NAME::cut() __OPSYM rhs. __NAME::cut() ) ) { \
		return Derived( lhs. __NAME::cut() __OPSYM rhs. __NAME::cut() ); \
	} \
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
	inline friend bool operator __OPSYM (Derived const & lhs, Derived const & rhs) noexcept( noexcept( lhs. __NAME::cut() __OPSYM rhs. __NAME::cut() ) ) { \
		return lhs. __NAME::cut() __OPSYM rhs. __NAME::cut(); \
	} \
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
	inline Derived operator __OPSYM () const noexcept( noexcept( __OPSYM __NAME::cut() ) ){ \
		return Derived( __OPSYM __NAME::cut() ); \
	} \
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
	public Mod<TMPL_VARS>,
	public Negate<TMPL_VARS>
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
class CompareAll:
	public CompareEqual<TMPL_VARS>,
	public CompareNotEqual<TMPL_VARS>,
	public CompareGreater<TMPL_VARS>,
	public CompareGreaterEqual<TMPL_VARS>,
	public CompareLess<TMPL_VARS>,
	public CompareLessEqual<TMPL_VARS>
{};

TMPL_HEADER
class Swap {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline friend void swap(Derived & lhs, Derived & rhs) noexcept {
		using std::swap;
		swap(lhs. Swap::ut(), rhs. Swap::ut() );
	}
};

TMPL_HEADER
class Serialize {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline friend sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & lhs, Derived const & rhs) noexcept( noexcept( lhs << rhs.cut() ) ) {
		return lhs << rhs.cut();
	}
};

TMPL_HEADER
class Deserialize {
private:
	UNDERLYING_TYPE_HELPERS
public:
	inline friend sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & lhs, Derived & rhs) noexcept( noexcept( lhs >> rhs.ut() ) ) {
		return lhs >> rhs.ut();
	}
};

} //end namespace impl

#define MODULE(__N) \
struct __N { \
	TMPL_HEADER \
	using type = impl:: __N <TMPL_VARS>; \
};

MODULE(Store)
MODULE(Add)
MODULE(Sub)
MODULE(Mult)
MODULE(Div)
MODULE(LShift)
MODULE(RShift)
MODULE(BitOr)
MODULE(BitAnd)
MODULE(BitXor)
MODULE(And)
MODULE(Or)
MODULE(CompareEqual)
MODULE(CompareNotEqual)
MODULE(CompareLess)
MODULE(CompareLessEqual)
MODULE(CompareGreater)
MODULE(CompareGreaterEqual)
MODULE(BitInvert)
MODULE(Negate)
MODULE(Not)
MODULE(Arithmetic)
MODULE(BitBased)
MODULE(BooleanLike)
MODULE(CompareAll)
MODULE(Swap)
MODULE(Serialize)
MODULE(Deserialize)

#undef MODULE
#undef UNDERLYING_TYPE_HELPERS
#undef TMPL_HEADER

}//end namespace sserialize::st
