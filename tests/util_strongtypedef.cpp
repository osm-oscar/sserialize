#include "TestBase.h"
#include <sserialize/utility/strongtypedefs.h>


//This is mostly a compile-time test

namespace ns1 {
	class WrappedArithmeticWithCast;
	class WrappedArithmeticAttorney;
}

template<>
struct sserialize::st::underlying_type<ns1::WrappedArithmeticWithCast> {
	using type = uint64_t;
};

template<>
struct sserialize::st::underlying_type<ns1::WrappedArithmeticAttorney> {
	using type = uint64_t;
};

template<>
struct sserialize::st::underlying_type_accessor<ns1::WrappedArithmeticAttorney>;

namespace ns1 {

class WrappedArithmeticWithCast:
	public sserialize::st::Arithmetic<WrappedArithmeticWithCast>,
	public sserialize::st::CompareEqual<WrappedArithmeticWithCast>
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

class WrappedArithmeticAttorney:
	public sserialize::st::Arithmetic<WrappedArithmeticAttorney>,
	public sserialize::st::CompareEqual<WrappedArithmeticAttorney>
{
public:
	using underlying_type = uint64_t;
public:
	WrappedArithmeticAttorney(underlying_type v) : m_v(v) {}
private:
	friend class sserialize::st::underlying_type_accessor<ns1::WrappedArithmeticAttorney>;
private:
	underlying_type m_v;
};

struct Outside {
	class WrappedArithmeticInnerAccessor;
	class WrappedArithmeticInnerAttorney:
		public sserialize::st::Arithmetic<WrappedArithmeticInnerAttorney, uint64_t, WrappedArithmeticInnerAccessor>,
		public sserialize::st::CompareEqual<WrappedArithmeticInnerAttorney, uint64_t, WrappedArithmeticInnerAccessor>
	{
	public:
		using underlying_type = uint64_t;
	public:
		WrappedArithmeticInnerAttorney(underlying_type v) : m_v(v) {}
	private:
		friend class WrappedArithmeticInnerAccessor;
	private:
		underlying_type m_v;
	};
	struct WrappedArithmeticInnerAccessor {
		using type = uint64_t;
		inline static type & get(WrappedArithmeticInnerAttorney & v) { return v.m_v; }
		inline static type const & get(WrappedArithmeticInnerAttorney const & v) { return v.m_v; }
	};
	class WrappedArithmeticInnerWithCast:
		public sserialize::st::Arithmetic<WrappedArithmeticInnerWithCast, uint64_t>,
		public sserialize::st::CompareEqual<WrappedArithmeticInnerWithCast, uint64_t>
	{
	public:
		using underlying_type = uint64_t;
	public:
		WrappedArithmeticInnerWithCast(underlying_type v) : m_v(v) {}
	public:
		operator underlying_type&() { return m_v; }
		operator underlying_type const &() const { return m_v; }
	private:
		friend class WrappedArithmeticInnerAccessor;
	private:
		underlying_type m_v;
	};
};

}//end namespace ns1

namespace sserialize::st {

	
template<>
struct underlying_type_accessor<ns1::WrappedArithmeticAttorney> {
	using type = typename sserialize::st::underlying_type<ns1::WrappedArithmeticAttorney>::type;
	inline static type const & get(ns1::WrappedArithmeticAttorney const & v) {
		return v.m_v;
	}
	static type & get(ns1::WrappedArithmeticAttorney & v) {
		return v.m_v;
	}
};

}//end namespace sserialize::st

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
	runner.addTest(  StrongTypeDefTest<ns1::WrappedArithmeticWithCast>::suite() );
	runner.addTest(  StrongTypeDefTest<ns1::WrappedArithmeticAttorney>::suite() );
	runner.addTest(  StrongTypeDefTest<ns1::Outside::WrappedArithmeticInnerAttorney>::suite() );
	runner.addTest(  StrongTypeDefTest<ns1::Outside::WrappedArithmeticInnerWithCast>::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
