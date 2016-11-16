#include "datacreationfuncs.h"
#include "TestBase.h"
#include <sserialize/utility/printers.h>
#include <sserialize/containers/MMVector.h>
#include <sserialize/algorithm/oom_algorithm.h>

class OomAlgorithm: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( OomAlgorithm );
CPPUNIT_TEST( testSort );
CPPUNIT_TEST( testLargeSort);
CPPUNIT_TEST( testUnique );
CPPUNIT_TEST( testSortOOMArray );
CPPUNIT_TEST( testUniqueOOMArray );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testSort() {
		for(uint32_t i(0); i < 4; ++i) {
			uint32_t scaleFactor = (16 << i);
			std::vector<uint32_t> data(1025*1023*519/scaleFactor);
			std::generate(data.begin(), data.end(), []() { return rand(); });
			std::vector<uint32_t> data2(data);
			
			sserialize::oom_sort(data.begin(), data.end(), [](uint32_t a, uint32_t b) { return a < b; },
									(static_cast<uint64_t>(1) << 22)/scaleFactor, sserialize::MM_PROGRAM_MEMORY);
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not sorted in run ", i), std::is_sorted(data.begin(), data.end()));
			CPPUNIT_ASSERT(data2.size() == data.size());
			
			sserialize::mt_sort(data2.begin(), data2.end(), [](uint32_t a, uint32_t b) { return a < b; });
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Data corruption in run ", i), std::equal(data2.begin(), data2.end(), data.begin()));
		}
	}
	
	void testLargeSort() {
		sserialize::MMVector<uint32_t> data(sserialize::MM_FILEBASED, 1025*1023*1024);
		std::generate(data.begin(), data.end(), []() { return rand(); });
		
		sserialize::oom_sort(data.begin(), data.end(), std::less<uint32_t>(), 1 << 30);
		CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not sorted"), std::is_sorted(data.begin(), data.end()));
		
		//data corruption is difficult to check
	}

	void testSortOOMArray() {
		for(uint32_t i(0); i < 4; ++i) {
			uint32_t scaleFactor = (16 << i);
			std::vector<uint32_t> realData(1025*1023*519/scaleFactor);
			std::generate(realData.begin(), realData.end(), []() { return rand(); });
			sserialize::OOMArray<uint32_t> data(sserialize::MM_PROGRAM_MEMORY);
			data.replace(data.end(), realData.begin(), realData.end());
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", realData.size(), data.size());
			CPPUNIT_ASSERT_MESSAGE("realData != data", sserialize::equal(realData.begin(), realData.end(), data.begin(), data.end(), [](uint32_t a, uint32_t b) {return a == b;}));
			
			sserialize::oom_sort(data.begin(), data.end(), [](uint32_t a, uint32_t b) { return a < b; },
									(static_cast<uint64_t>(1) << 22)/scaleFactor, sserialize::MM_PROGRAM_MEMORY);
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not sorted in run ", i), std::is_sorted(data.begin(), data.end()));
			CPPUNIT_ASSERT(realData.size() == data.size());
			
			sserialize::mt_sort(realData.begin(), realData.end(), [](uint32_t a, uint32_t b) { return a < b; });
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Data corruption in run ", i), std::equal(realData.begin(), realData.end(), data.begin()));
		}
	}

	void testUnique() {
		for(uint32_t i(0); i < 4; ++i) {
			uint32_t scaleFactor = (16 << i);
			std::vector<uint32_t> data(1025*1023*519/scaleFactor);
			uint32_t tmp = 0;
			std::generate(data.begin(), data.end(), [&tmp]() {
				if (rand() % 4 == 1) {
					return tmp;
				}
				return ++tmp;
			});
			std::vector<uint32_t> data2(data);
			
			sserialize::oom_sort(data.begin(), data.end(), std::less<uint32_t>(), (1 << 22)/scaleFactor);
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not sorted in run ", i), std::is_sorted(data.begin(), data.end()));
			auto end = sserialize::oom_unique(data.begin(), data.end());
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not unique in run ", i), sserialize::is_unique(data.begin(), end));
			
			sserialize::mt_sort(data2.begin(), data2.end(), [](uint32_t a, uint32_t b) { return a < b; });
			auto end2 = std::unique(data2.begin(), data2.end());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size of unique part", std::distance(data2.begin(), end2), std::distance(data.begin(), end));
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Data corrupted in run ", i), std::equal(data2.begin(), end2, data.begin()));
		}
	}


	void testUniqueOOMArray() {
		for(uint32_t i(0); i < 4; ++i) {
			uint32_t scaleFactor = (16 << i);
			std::vector<uint32_t> realData(1025*1023*519/scaleFactor);
			uint32_t tmp = 0;
			std::generate(realData.begin(), realData.end(), [&tmp]() {
				if (rand() % 4 == 1) {
					return tmp;
				}
				return ++tmp;
			});
			sserialize::OOMArray<uint32_t> data(sserialize::MM_PROGRAM_MEMORY);
			data.replace(data.end(), realData.begin(), realData.end());
			
			sserialize::oom_sort(data.begin(), data.end(), std::less<uint32_t>(), (1 << 22)/scaleFactor);
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not sorted in run ", i), std::is_sorted(data.begin(), data.end()));
			auto end = sserialize::oom_unique(data.begin(), data.end());
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Not unique in run ", i), sserialize::is_unique(data.begin(), end));
			
			sserialize::mt_sort(realData.begin(), realData.end(), [](uint32_t a, uint32_t b) { return a < b; });
			auto end2 = std::unique(realData.begin(), realData.end());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size of unique part", std::distance(realData.begin(), end2), std::distance(data.begin(), end));
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Data corrupted in run ", i), std::equal(realData.begin(), end2, data.begin()));
		}
	}
	
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::argc = argc;
	sserialize::tests::TestBase::argv = argv;
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  OomAlgorithm::suite() );
	runner.eventManager().popProtector();
	runner.run();
	return 0;
}