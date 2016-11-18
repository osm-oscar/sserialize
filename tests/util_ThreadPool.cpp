#include <sserialize/mt/ThreadPool.h>
#include <sserialize/mt/GuardedVariable.h>
#include "TestBase.h"
#include <unistd.h>

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
		std::atomic<uint32_t> scheduledTasks(0);
		std::atomic<uint32_t> fcalls(0);
		std::atomic<uint32_t> gvcalls(0);
		CPPUNIT_ASSERT_EQUAL((uint32_t)0, gv.unsyncedValue());
		for(uint32_t i = 0; i < T_NUM_TASKS; ++i) {
			bool ok = tp.sheduleTask([&gv, &fcalls, &gvcalls](){
				fcalls += 1;
				gv.syncedWithoutNotify([&gvcalls](uint32_t & v) {
					v += 1;
					gvcalls += 1;
				});
			});
			if (ok) {
				scheduledTasks += 1;
			}
		}
		tp.flushQueue();
		std::size_t qs = tp.queueSize();
		uint32_t v;
		gv.syncedWithoutNotify([&v](uint32_t iv) { v = iv; });
		CPPUNIT_ASSERT_EQUAL_MESSAGE("scheduled tasks", T_NUM_TASKS, scheduledTasks.load());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("fcalls", T_NUM_TASKS, fcalls.load());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("gvcalls", T_NUM_TASKS, gvcalls.load());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("number of tasks in queue", (std::size_t) 0, qs);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("number of tasks completed", (uint32_t) T_NUM_TASKS, v);
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
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