#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestAssert.h>
#include <sserialize/containers/IFLArray.h>
#include <sserialize/utility/printers.h>
#include "datacreationfuncs.h"
#include <functional>


template<typename T_TEST_DATA_TYPE, uint32_t T_TEST_COUNT>
class IFLArrayTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( IFLArrayTest );
CPPUNIT_TEST( testIterator );
CPPUNIT_TEST( testReverseIterator );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST_SUITE_END();
private:
	typedef T_TEST_DATA_TYPE value_type;
	typedef sserialize::IFLArray<value_type> MyIFLArray;
	std::vector<value_type> m_d;
public:
	virtual void setUp() {
		typedef std::back_insert_iterator< std::vector<value_type> > MyInserter;
		sserialize::TestDataGenerator<MyInserter, uint32_t>::generate(T_TEST_COUNT, MyInserter(m_d));
	}
	virtual void tearDown() {}
	void testSize() {
		MyIFLArray d(&(m_d[0]), m_d.size());
		MyIFLArray c(m_d.begin(), m_d.end());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("direct size", (uint32_t)m_d.size(), (uint32_t)d.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("copy size", (uint32_t)m_d.size(), (uint32_t)c.size());
	}
	
	void testIterator() {
		MyIFLArray d(&(m_d[0]), m_d.size());
		MyIFLArray c(m_d.begin(), m_d.end());
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("direct size", (uint32_t)m_d.size(), (uint32_t)d.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("copy size", (uint32_t)m_d.size(), (uint32_t)c.size());
		
		typename MyIFLArray::iterator dIt(d.begin()), dEnd(d.end());
		typename MyIFLArray::iterator cIt(c.begin()), cEnd(c.end());
		typename std::vector<value_type>::iterator mIt(m_d.begin());
		for(uint32_t i(0), s(m_d.size()); i < s; ++i) {
			CPPUNIT_ASSERT(dIt != dEnd);
			CPPUNIT_ASSERT(cIt != cEnd);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("direct iterator at ", i), *mIt, *dIt);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("copy iterator at ", i), *mIt, *cIt);
			++dIt;
			++cIt;
			++mIt;
		}
	}
	
	void testReverseIterator() {
		MyIFLArray d(&(m_d[0]), m_d.size());
		MyIFLArray c(m_d.begin(), m_d.end());
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("direct size", (uint32_t)m_d.size(), (uint32_t)d.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("copy size", (uint32_t)m_d.size(), (uint32_t)c.size());
		
		typename MyIFLArray::reverse_iterator dIt(d.rbegin()), dEnd(d.rend());
		typename MyIFLArray::reverse_iterator cIt(c.rbegin()), cEnd(c.rend());
		typename std::vector<value_type>::reverse_iterator mIt(m_d.rbegin());
		for(uint32_t i(0), s(m_d.size()); i < s; ++i) {
			CPPUNIT_ASSERT(dIt != dEnd);
			CPPUNIT_ASSERT(cIt != cEnd);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("direct iterator at ", i), *mIt, *dIt);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("copy iterator at ", i), *mIt, *cIt);
			++dIt;
			++cIt;
			++mIt;
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  IFLArrayTest<uint32_t, 1042>::suite() );
	runner.run();
	return 0;
}