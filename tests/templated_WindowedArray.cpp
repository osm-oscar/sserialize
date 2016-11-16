#include <vector>
#include <sserialize/containers/WindowedArray.h>
#include <sserialize/utility/log.h>
#include "datacreationfuncs.h"
#include "TestBase.h"

using namespace sserialize;

template<int NumberOfBuckets, int ItemsPerBucket>
class WindowedArrayTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( WindowedArrayTest );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST_SUITE_END();
private:
	typedef std::vector< std::vector< uint32_t > > DataContainer;
private:
	static const int NumberOfRuns = 10;
// 	static const int NumberOfBuckets = 50;
// 	static const int ItemsPerBucket  = 100;
private:
	DataContainer createTestData() {
		DataContainer ret;
		createOverLappingSets(NumberOfBuckets, ItemsPerBucket/10, ItemsPerBucket, ret);
		return ret;
	}
	
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	
	void testEquality() {
		for(int i = 0; i < NumberOfRuns; ++i) {
			std::vector< std::vector< uint32_t > > testData = createTestData();
			auto mergeFunc = [](const std::vector<uint32_t> & a, const std::vector<uint32_t> & b) {
				std::vector<uint32_t> ret;
				std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(ret));
				return ret;
			};
			std::vector<uint32_t> mergedTestData = sserialize::treeReduce<std::vector< std::vector<uint32_t> >::const_iterator, std::vector<uint32_t> >(testData.begin(), testData.end(), mergeFunc);
			
			
			uint32_t storageNeed = 0;
			for(uint32_t i = 0; i < testData.size(); ++i)
				storageNeed += testData[i].size();
			
			uint32_t * waData = new uint32_t[storageNeed];
			
			std::vector< WindowedArray<uint32_t> > was;
			uint32_t * ptr = waData;
			for(uint32_t i = 0; i < testData.size(); ++i) {
				was.push_back( sserialize::WindowedArray<uint32_t>(ptr, ptr+testData[i].size()) );
				WindowedArray<uint32_t> & wa = was.back();
				for(std::size_t j = 0, js = testData[i].size(); j < js; ++j) {
					wa.push_back(testData[i][j]);
				}
				ptr = ptr+testData[i].size();
			}
			
			WindowedArray<uint32_t> mergedWa = WindowedArray<uint32_t>::uniteSortedInPlace(was.begin(), was.end());
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("Merged data don't have the same size", mergedTestData.size(), mergedWa.size());
			
			for(std::size_t i = 0, s = mergedWa.size(); i < s; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("Merged data at ", i), mergedTestData[i], mergedWa[i]);
			}
			
			delete[] waData;
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::argc = argc;
	sserialize::tests::TestBase::argv = argv;
	srand(0);
	srandom( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.eventManager().popProtector();
	runner.addTest( WindowedArrayTest<2, 100>::suite() );
	runner.addTest( WindowedArrayTest<5, 100>::suite() );
	runner.addTest( WindowedArrayTest<7, 100>::suite() );
	runner.addTest( WindowedArrayTest<83, 100>::suite() );
	runner.addTest( WindowedArrayTest<128, 100>::suite() );
	runner.addTest( WindowedArrayTest<1024, 1024>::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
