#include "containers_ItemIndexBaseTest.h"
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <cppunit/TestResult.h>

#define DEFAULT_ENABLED_TESTS 


template<typename T_STL_CONTAINER>
class ItemIndexPrivateStlContainerTest: public ItemIndexPrivateBaseTest {
CPPUNIT_TEST_SUITE( ItemIndexPrivateStlContainerTest );
CPPUNIT_TEST( testRandomEquality );
CPPUNIT_TEST( testSpecialEquality );
CPPUNIT_TEST( testIntersect );
CPPUNIT_TEST( testUnite );
CPPUNIT_TEST( testDifference );
CPPUNIT_TEST( testSymmetricDifference );
CPPUNIT_TEST( testDynamicBitSet );
CPPUNIT_TEST( testPutIntoVector );
CPPUNIT_TEST( testIterator );
CPPUNIT_TEST( testRandomMaxSetEquality );
CPPUNIT_TEST_SUITE_END();
protected:
	template<typename T_SORTED_CONTAINER>
	bool createBase(const T_SORTED_CONTAINER & srcSet, sserialize::ItemIndex & idx) {
		idx = sserialize::ItemIndex( T_STL_CONTAINER(srcSet.cbegin(), srcSet.cend()) );
		return true;
	}

	virtual bool create(const std::set<uint32_t> & srcSet, sserialize::ItemIndex & idx) override {
		return createBase(srcSet, idx);
	}
	virtual bool create(const std::vector<uint32_t> & srcSet, sserialize::ItemIndex & idx) override {
		return createBase(srcSet, idx);
	}
public:
	ItemIndexPrivateStlContainerTest() : ItemIndexPrivateBaseTest(sserialize::ItemIndex::ItemIndex::T_STL_VECTOR) {}
};

template<>
ItemIndexPrivateStlContainerTest< std::vector<uint32_t> >::ItemIndexPrivateStlContainerTest() :
ItemIndexPrivateBaseTest(sserialize::ItemIndex::ItemIndex::T_STL_VECTOR) {}

template<>
ItemIndexPrivateStlContainerTest< std::deque<uint32_t> >::ItemIndexPrivateStlContainerTest() :
ItemIndexPrivateBaseTest(sserialize::ItemIndex::ItemIndex::T_STL_DEQUE) {}

template<ItemIndex::Types T_TYPE>
class ItemIndexPrivateSerializedTest: public ItemIndexPrivateBaseTest {
CPPUNIT_TEST_SUITE( ItemIndexPrivateSerializedTest );
CPPUNIT_TEST( testRandomEquality );
CPPUNIT_TEST( testSpecialEquality );
CPPUNIT_TEST( testIntersect );
CPPUNIT_TEST( testUnite );
CPPUNIT_TEST( testDifference );
CPPUNIT_TEST( testSymmetricDifference );
CPPUNIT_TEST( testDynamicBitSet );
CPPUNIT_TEST( testPutIntoVector );
CPPUNIT_TEST( testIterator );
CPPUNIT_TEST( testRandomMaxSetEquality );
CPPUNIT_TEST_SUITE_END();
protected:
	template<typename T_SORTED_CONTAINER>
	bool createBase(const T_SORTED_CONTAINER & srcSet, sserialize::ItemIndex & idx) {
		sserialize::UByteArrayAdapter dest(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
		bool ok = sserialize::ItemIndexFactory::create(srcSet, dest, T_TYPE);
		idx = sserialize::ItemIndex(dest, T_TYPE);
		return ok;
	}

	virtual bool create(const std::set<uint32_t> & srcSet, sserialize::ItemIndex & idx) override {
		return createBase(srcSet, idx);
	}
	virtual bool create(const std::vector<uint32_t> & srcSet, sserialize::ItemIndex & idx) override {
		return createBase(srcSet, idx);
	}
public:
	ItemIndexPrivateSerializedTest() : ItemIndexPrivateBaseTest(T_TYPE) {}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	
	runner.addTest(  ItemIndexPrivateStlContainerTest< std::vector<uint32_t> >::suite() );
	runner.addTest(  ItemIndexPrivateStlContainerTest< std::deque<uint32_t> >::suite() );
	
	runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_SIMPLE>::suite() );
	runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_REGLINE>::suite() );
	runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_NATIVE>::suite() );
	runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_WAH>::suite() );
	runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_DE>::suite() );
	runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_RLE_DE>::suite() );
// 	runner.eventManager().popProtector();
	bool ok = runner.run();
	return ok ? 0 : 1;
}