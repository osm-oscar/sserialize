#include "TestBase.h"
#include <sserialize/utility/strongtypedefs.h>


//This is mostly a compile-time test

namespace ns1 {
	class WrappedArithmeticWithCast;
	class WrappedArithmeticWithAttorney;
}

namespace ns1 {
	
class WrappedArithmeticWithStore:
#define MY_BASE \
	sserialize::st::strong_type_store< \
		WrappedArithmeticWithStore, \
		uint64_t, \
		sserialize::st::Arithmetic, \
		sserialize::st::CompareAll \
		>
public MY_BASE
{
	//have to import the ctors
	using MyBase = MY_BASE;
public:
	using MyBase::MyBase;
};

class WrappedArithmeticWithCast:
	public sserialize::st::strong_type_ca<
		WrappedArithmeticWithCast,
		uint64_t,
		sserialize::st::Arithmetic,
		sserialize::st::CompareAll
		>
{
public:
	using underlying_type = uint64_t;
	public:
		WrappedArithmeticWithCast(underlying_type v) : m_v(v) {}
public:
	operator underlying_type&() { return m_v; }
	operator underlying_type const &() const { return m_v; }
private:
	underlying_type m_v;
};

class WrappedArithmeticAttorney {
public:
	using type = uint64_t;
	static type const & get(WrappedArithmeticWithAttorney const & v) noexcept(true);
	static type & get(WrappedArithmeticWithAttorney & v) noexcept(true);
};

class WrappedArithmeticWithAttorney:
	public sserialize::st::strong_type<
		WrappedArithmeticWithAttorney,
		uint64_t,
		WrappedArithmeticAttorney,
		sserialize::st::Arithmetic,
		sserialize::st::CompareAll
		>
{
public:
	using underlying_type = uint64_t;
public:
	WrappedArithmeticWithAttorney(underlying_type v) : m_v(v) {}
private:
	friend class WrappedArithmeticAttorney;
private:
	underlying_type m_v;
};

uint64_t const &
WrappedArithmeticAttorney::get(WrappedArithmeticWithAttorney const & v) noexcept(true) {
	return v.m_v;
}

uint64_t &
WrappedArithmeticAttorney::get(WrappedArithmeticWithAttorney & v) noexcept(true) {
	return v.m_v;
}


struct Outside {
	class WrappedArithmeticInnerAttorney;
	class WrappedArithmeticInnerWithAttorney:
		public sserialize::st::strong_type<
			WrappedArithmeticInnerWithAttorney,
			uint64_t,
			WrappedArithmeticInnerAttorney,
			sserialize::st::Arithmetic,
			sserialize::st::CompareAll
			>
	{
	public:
		using underlying_type = uint64_t;
	public:
		WrappedArithmeticInnerWithAttorney(underlying_type v) : m_v(v) {}
	private:
		friend class WrappedArithmeticInnerAttorney;
	private:
		underlying_type m_v;
	};
	class WrappedArithmeticInnerAttorney {
	public:
		using type = uint64_t;
		inline static type & get(WrappedArithmeticInnerWithAttorney & v) { return v.m_v; }
		inline static type const & get(WrappedArithmeticInnerWithAttorney const & v) { return v.m_v; }
	};
	class WrappedArithmeticInnerWithCast:
		public sserialize::st::strong_type_ca<
			WrappedArithmeticInnerWithCast,
			uint64_t,
			sserialize::st::Arithmetic,
			sserialize::st::CompareAll
			>
	{
	public:
		using underlying_type = uint64_t;
	public:
		WrappedArithmeticInnerWithCast(underlying_type v) : m_v(v) {}
	public:
		operator underlying_type&() { return m_v; }
		operator underlying_type const &() const { return m_v; }
	private:
		underlying_type m_v;
	};
};

}//end namespace ns1


template<typename T>
class StrongTypeDefTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( StrongTypeDefTest );
CPPUNIT_TEST( testAdd );
CPPUNIT_TEST( testSub );
CPPUNIT_TEST( testMult );
CPPUNIT_TEST( testDiv );
CPPUNIT_TEST_SUITE_END();
public:
	using ut = typename sserialize::st::underlying_type<T>::type;
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	#define E(__NAME, __OP) \
		void __NAME() { \
			ut a(35), b(5); \
			ut c = a __OP b; \
			CPPUNIT_ASSERT(T(c) == T(a) __OP T(b)); \
		}
		
	E(testAdd, +)
	E(testSub, -)
	E(testMult, *)
	E(testDiv, /)
	
	#undef E
};

int main(int argc, char ** argv) {
	srand( 0 );
	
	sserialize::tests::TestBase::init(argc, argv);
	
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  StrongTypeDefTest<ns1::WrappedArithmeticWithStore>::suite() );
	runner.addTest(  StrongTypeDefTest<ns1::WrappedArithmeticWithCast>::suite() );
	runner.addTest(  StrongTypeDefTest<ns1::WrappedArithmeticWithAttorney>::suite() );
	runner.addTest(  StrongTypeDefTest<ns1::Outside::WrappedArithmeticInnerWithAttorney>::suite() );
	runner.addTest(  StrongTypeDefTest<ns1::Outside::WrappedArithmeticInnerWithCast>::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
