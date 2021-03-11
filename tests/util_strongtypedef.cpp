#include "TestBase.h"
#include <sserialize/utility/strongtypedefs.h>


//This is mostly a compile-time test

namespace ns1 {
	class WrappedArithmeticWithCast;
	class WrappedArithmeticAttorney;
	
	
	class WrappedArithmeticSpecial;
}

template<>
struct sserialize::st::underlying_type<ns1::WrappedArithmeticWithCast> {
	using type = uint64_t;
};

template<>
struct sserialize::st::underlying_type<ns1::WrappedArithmeticSpecial> {
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
	public sserialize::st::Arithmetic<WrappedArithmeticWithCast>
{
public:
	using underlying_type = uint64_t;
public:
	operator underlying_type&() { return m_v; }
	operator underlying_type const &() const { return m_v; }
private:
	underlying_type m_v;
};

class WrappedArithmeticSpecial:
	public sserialize::st::Special<WrappedArithmeticSpecial>
{
public:
	using underlying_type = uint64_t;
public:
	operator underlying_type&() { return m_v; }
	operator underlying_type const &() const { return m_v; }
private:
	underlying_type m_v;
};

class WrappedArithmeticAttorney:
	public sserialize::st::Arithmetic<WrappedArithmeticAttorney>
{
public:
	using underlying_type = uint64_t;
private:
	friend class sserialize::st::underlying_type_accessor<ns1::WrappedArithmeticAttorney>;
private:
	underlying_type m_v;
};

class Outside {
	class WrappedArithmeticInnerAccessor;
	class WrappedArithmeticInnerAttorney:
		public sserialize::st::Special<WrappedArithmeticInnerAttorney, uint64_t, WrappedArithmeticInnerAccessor>
	{
	public:
		using underlying_type = uint64_t;
	private:
		friend class WrappedArithmeticInnerAccessor;
	private:
		underlying_type m_v;
	};
	class WrappedArithmeticInnerAccessor {
		using type = uint64_t;
		inline static type & get(WrappedArithmeticInnerAttorney & v) { return v.m_v; }
		inline static type const & get(WrappedArithmeticInnerAttorney const & v) { return v.m_v; }
	};
	class WrappedArithmeticInnerWithCast:
		public sserialize::st::Special<WrappedArithmeticInnerWithCast, uint64_t>
	{
	public:
		using underlying_type = uint64_t;
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

class StrongTypeDefTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( StrongTypeDefTest );
CPPUNIT_TEST( test );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void test() {}
};

int main(int argc, char ** argv) {
	srand( 0 );
	
	sserialize::tests::TestBase::init(argc, argv);
	
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  StrongTypeDefTest::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
