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
		sserialize::UByteArrayAdapter dest(1, sserialize::MM_PROGRAM_MEMORY);
		bool ok = sserialize::ItemIndexFactory::create(srcSet, dest, T_TYPE);
		idx = sserialize::ItemIndex(dest, T_TYPE);
		sserialize::UByteArrayAdapter idxData;
		bool hasIdxData = true;
		try {
			idxData = idx.data();
		}
		catch (sserialize::UnimplementedFunctionException &) {
			hasIdxData = false;
		}
		if (hasIdxData) {
			CPPUNIT_ASSERT_MESSAGE("index data from index unequal to data", dest.equalContent(idxData));
			CPPUNIT_ASSERT_EQUAL_MESSAGE("Index data size incorrect", dest.size(), idx.getSizeInBytes());
			sserialize::ItemIndex idx2(idx.data(), T_TYPE);
			if (idx2 != srcSet) {
				sserialize::ItemIndex idx3 = sserialize::ItemIndexFactory::create(srcSet, T_TYPE);
			}
			CPPUNIT_ASSERT_MESSAGE("index from index data unequal",  idx2 == srcSet);
		}
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
	
	int selectedTests = 0;
	
	for (int i(0); i < argc; ++i) {
		std::string token(argv[i]);
		if (token == "-t" && i+1 < argc) {
			token = std::string(argv[i+1]);
			sserialize::ItemIndex::Types t;
			sserialize::from_string(token, t);
			selectedTests |= t;
			++i;
		}
	}
	
	if (selectedTests == 0) {
		selectedTests = 0xFFFFFFFF;
	}
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	
	if (selectedTests & sserialize::ItemIndex::T_STL_VECTOR) {
		runner.addTest(  ItemIndexPrivateStlContainerTest< std::vector<uint32_t> >::suite() );
	}
	if (selectedTests & sserialize::ItemIndex::T_STL_DEQUE) {
		runner.addTest(  ItemIndexPrivateStlContainerTest< std::deque<uint32_t> >::suite() );
	}
	
	if (selectedTests & sserialize::ItemIndex::T_SIMPLE) {
		runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_SIMPLE>::suite() );
	}
	if (selectedTests & sserialize::ItemIndex::T_REGLINE) {
		runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_REGLINE>::suite() );
	}
	if (selectedTests & sserialize::ItemIndex::T_NATIVE) {
		runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_NATIVE>::suite() );
	}
	if (selectedTests & sserialize::ItemIndex::T_WAH) {
		runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_WAH>::suite() );
	}
	if (selectedTests & sserialize::ItemIndex::T_DE) {
		runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_DE>::suite() );
	}
	if (selectedTests & sserialize::ItemIndex::T_RLE_DE) {
		runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_RLE_DE>::suite() );
	}
	if (selectedTests & sserialize::ItemIndex::T_ELIAS_FANO) {
		runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_ELIAS_FANO>::suite() );
	}
	if (selectedTests & sserialize::ItemIndex::T_PFOR) {
		runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_PFOR>::suite() );
	}
	if (selectedTests & sserialize::ItemIndex::T_FOR) {
		runner.addTest(  ItemIndexPrivateSerializedTest<sserialize::ItemIndex::T_FOR>::suite() );
	}
	
	if (sserialize::tests::TestBase::popProtector()) {
		runner.eventManager().popProtector();
	}
	
	bool ok = runner.run();
	return ok ? 0 : 1;
}
