#include <sserialize/mt/ThreadPool.h>
#include <sserialize/mt/GuardedVariable.h>
#include "TestBase.h"

template<uint32_t T_NUM_THREADS, uint32_t T_NUM_TASKS>
class TestThreadPool: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( TestThreadPool );
CPPUNIT_TEST( test );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void test() {
		sserialize::ThreadPool tp(T_NUM_THREADS);
		sserialize::GuardedVariable<uint32_t> gv(0);
		for(uint32_t i = 0; i < T_NUM_TASKS; ++i) {
			tp.sheduleTask([&gv](){
				gv.syncedWithoutNotify([](uint32_t & v) { v += 1; });
			});
		}
		tp.flushQueue();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("number of tasks completed", T_NUM_TASKS, gv.unsyncedValue());
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::argc = argc;
	sserialize::tests::TestBase::argv = argv;
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestThreadPool<1, 10>::suite() );
	runner.addTest(  TestThreadPool<2, 2>::suite() );
	runner.addTest(  TestThreadPool<2, 20>::suite() );
	runner.addTest(  TestThreadPool<4, 100>::suite() );
	runner.addTest(  TestThreadPool<8, 8>::suite() );
	runner.addTest(  TestThreadPool<8, 1>::suite() );
	runner.addTest(  TestThreadPool<100, 1000>::suite() );
	runner.addTest(  TestThreadPool<100, 1>::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}