#include <algorithm>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/containers/ItemIndexIterator.h>
#include <sserialize/containers/ItemIndexIteratorSetOp.h>
#include <sserialize/utility/utilfuncs.h>
#include "datacreationfuncs.h"
#include "utilalgos.h"

using namespace sserialize;

class ItemIndexIteratorTest: public CppUnit::TestFixture {
private:
	std::set<uint32_t> m_srcRes;
	ItemIndexIterator m_indexRes;
protected:
	std::set<uint32_t> & srcRes() { return m_srcRes; }
	ItemIndexIterator & indexRes() { return m_indexRes; }
public:
	virtual void setUp() {}
	virtual void tearDown() {}

	void testEquality() {
		CPPUNIT_ASSERT( m_indexRes.toItemIndex() == m_srcRes );
	}
};


template<uint32_t T_ITEM_COUNT>
class ItemIndexIteratorItemIndexTest: public ItemIndexIteratorTest {
CPPUNIT_TEST_SUITE( ItemIndexIteratorItemIndexTest );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {
		srcRes() = createNumbersSet(T_ITEM_COUNT);
		std::vector<uint32_t> d(srcRes().begin(), srcRes().end());
		indexRes() = ItemIndexIterator(ItemIndex::absorb(d));
	}
	virtual void tearDown() {}
};

template<uint32_t T_ITEM_COUNT, ItemIndexIteratorSetOp::OpTypes T_OP_TYPE>
class ItemIndexIteratorSetOpTest: public ItemIndexIteratorTest { 
CPPUNIT_TEST_SUITE( ItemIndexIteratorSetOpTest );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {
		std::set<uint32_t> setA, setB;
		createOverLappingSets(setA, setB, T_ITEM_COUNT, T_ITEM_COUNT, T_ITEM_COUNT);
		std::vector<uint32_t> dA(setA.begin(), setA.end());
		std::vector<uint32_t> dB(setB.begin(), setB.end());
		
		
		ItemIndexIterator idxItA = ItemIndexIterator(ItemIndex::absorb(dA));
		ItemIndexIterator idxItB = ItemIndexIterator(ItemIndex::absorb(dB));
		
		
		srcRes().clear();
		std::insert_iterator<std::set<uint32_t>> srcResBegin( srcRes(), srcRes().end() );
		if (T_OP_TYPE == ItemIndexIteratorSetOp::OPT_UNITE) {
			std::set_union(setA.begin(), setA.end(), setB.begin(), setB.end(), srcResBegin);
		}
		else if (T_OP_TYPE == ItemIndexIteratorSetOp::OPT_INTERSECT){
			std::set_intersection(setA.begin(), setA.end(), setB.begin(), setB.end(), srcResBegin);
		}
		else if (T_OP_TYPE == ItemIndexIteratorSetOp::OPT_DIFF) {
			std::set_difference(setA.begin(), setA.end(), setB.begin(), setB.end(), srcResBegin);
		}
		else if (T_OP_TYPE == ItemIndexIteratorSetOp::OPT_XOR) {
			std::set_symmetric_difference(setA.begin(), setA.end(), setB.begin(), setB.end(), srcResBegin);
		}
		ItemIndexIteratorSetOp * idxIt = new ItemIndexIteratorSetOp(idxItA, idxItB, T_OP_TYPE);
		indexRes() = ItemIndexIterator( idxIt );
	}
	virtual void tearDown() {}
};

template<uint32_t T_SET_COUNT, uint32_t T_ITEM_COUNT>
class ItemIndexIteratorInterSectingTest: public ItemIndexIteratorTest { 
CPPUNIT_TEST_SUITE( ItemIndexIteratorInterSectingTest );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {
		std::deque< std::set<uint32_t> > sets;
		createOverLappingSets(T_SET_COUNT, T_ITEM_COUNT, T_ITEM_COUNT/2, sets);
		srcRes() = intersectSets(sets);
		
		std::vector<ItemIndexIterator> its;
		for(size_t i = 0; i < sets.size(); i++) {
			std::vector<uint32_t> d(sets[i].begin(), sets[i].end());
			ItemIndexIterator idxIt = ItemIndexIterator( ItemIndex::absorb(d) );
			its.push_back(idxIt);
		}
		indexRes() = ItemIndexIterator(its);
	}
	virtual void tearDown() {}
};


int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( ItemIndexIteratorItemIndexTest<0>::suite() );
	runner.addTest( ItemIndexIteratorItemIndexTest<1>::suite() );
	runner.addTest( ItemIndexIteratorItemIndexTest<23>::suite() );
	runner.addTest( ItemIndexIteratorItemIndexTest<42>::suite() );
	runner.addTest( ItemIndexIteratorItemIndexTest<1024>::suite() );
	
	runner.addTest( ItemIndexIteratorSetOpTest<337, ItemIndexIteratorSetOp::OPT_UNITE>::suite() );
	runner.addTest( ItemIndexIteratorSetOpTest<337, ItemIndexIteratorSetOp::OPT_INTERSECT>::suite() );
	runner.addTest( ItemIndexIteratorSetOpTest<337, ItemIndexIteratorSetOp::OPT_DIFF>::suite() );
	runner.addTest( ItemIndexIteratorSetOpTest<337, ItemIndexIteratorSetOp::OPT_XOR>::suite() );

	runner.addTest( ItemIndexIteratorSetOpTest<42, ItemIndexIteratorSetOp::OPT_UNITE>::suite() );
	runner.addTest( ItemIndexIteratorSetOpTest<42, ItemIndexIteratorSetOp::OPT_INTERSECT>::suite() );
	runner.addTest( ItemIndexIteratorSetOpTest<42, ItemIndexIteratorSetOp::OPT_DIFF>::suite() );
	runner.addTest( ItemIndexIteratorSetOpTest<42, ItemIndexIteratorSetOp::OPT_XOR>::suite() );

	runner.addTest( ItemIndexIteratorInterSectingTest<7, 23>::suite() );
	runner.addTest( ItemIndexIteratorInterSectingTest<32, 23>::suite() );
	runner.addTest( ItemIndexIteratorInterSectingTest<9, 1034>::suite() );
	runner.addTest( ItemIndexIteratorInterSectingTest<7, 57>::suite() );

	
	runner.run();
	return 0;
}
