#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <stdlib.h>
#include <vector>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.h>
#include <sserialize/containers/ItemIndex.h>
#include <staging/containers/DataSetFactory.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/Static/Array.h>
#include <sserialize/utility/printers.h>
#include "datacreationfuncs.h"

using namespace sserialize;

template<uint32_t T_SET_COUNT, uint32_t T_MAX_SET_FILL, ItemIndex::Types T_IDX_TYPE>
class DataSetFactoryTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( DataSetFactoryTest );
CPPUNIT_TEST( testSerializedEquality );
CPPUNIT_TEST( testSameId );
CPPUNIT_TEST_SUITE_END();
private:
	DataSetFactory m_dsFactory;
	std::vector< std::set<uint32_t> > m_sets;
	std::vector<uint32_t> m_setIds;
public:
	DataSetFactoryTest() : m_dsFactory(true) {}

	virtual void setUp() {
		m_dsFactory.setDataStoreFile( UByteArrayAdapter::createCache(T_SET_COUNT*T_MAX_SET_FILL, sserialize::MM_PROGRAM_MEMORY) );
		m_sets.reserve(T_SET_COUNT);
		m_setIds.reserve(T_SET_COUNT);
		
		m_sets.push_back(std::set<uint32_t>() ); //push the empty set as the index store always has it
		
		for(uint32_t i = 0; i < T_SET_COUNT; ++i) {
			m_sets.push_back( createNumbersSet( ((double)rand()/RAND_MAX) * T_MAX_SET_FILL ) );
		}
		
		for(uint32_t i = 0; i< m_sets.size(); ++i) {
			sserialize::UByteArrayAdapter tmp(new std::vector<uint8_t>(), true);
			sserialize::ItemIndexFactory::create(m_sets[i], tmp, T_IDX_TYPE);
			uint32_t id = m_dsFactory.insert(tmp);
			m_setIds.push_back(id);
		}
	}
	virtual void tearDown() {}
	
	void testSameId() {
		for(uint32_t i = 0; i < m_sets.size(); ++i) {
			sserialize::UByteArrayAdapter tmp(new std::vector<uint8_t>(), true);
			sserialize::ItemIndexFactory::create(m_sets[i], tmp, T_IDX_TYPE);
			uint32_t id = m_dsFactory.insert(tmp);
			CPPUNIT_ASSERT_EQUAL(m_setIds[i], id);
		}
	}
	
	void testSerializedEquality() {
		
		CPPUNIT_ASSERT_MESSAGE("Serialization failed", m_dsFactory.flush());

		UByteArrayAdapter dataAdap( m_dsFactory.getFlushedData());


		Static::Array<sserialize::UByteArrayAdapter> sdb(dataAdap);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("DataSetFactory.size() != DataSetFactory.size()", m_dsFactory.size(), sdb.size());

		for(size_t i = 0; i < m_sets.size(); ++i) {
			ItemIndex idx(sdb.at( m_setIds[i] ), T_IDX_TYPE);
			std::stringstream ss;
			ss << "Index at " << i;
			CPPUNIT_ASSERT_MESSAGE(ss.str(), m_sets[i] == idx);
		}
	}

};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  DataSetFactoryTest<64, 512, ItemIndex::T_SIMPLE>::suite() );
	runner.addTest(  DataSetFactoryTest<2048, 512, ItemIndex::T_REGLINE>::suite() );
	runner.addTest(  DataSetFactoryTest<2048, 512, ItemIndex::T_DE>::suite() );
	runner.addTest(  DataSetFactoryTest<2048, 512, ItemIndex::T_RLE_DE>::suite() );
	runner.addTest(  DataSetFactoryTest<2048, 512, ItemIndex::T_WAH>::suite() );
	runner.addTest(  DataSetFactoryTest<4047, 1001, ItemIndex::T_WAH>::suite() );
	runner.addTest(  DataSetFactoryTest<10537, 2040, ItemIndex::T_WAH>::suite() );
	runner.addTest(  DataSetFactoryTest<10537, 2040, ItemIndex::T_NATIVE>::suite() );
	runner.run();
	return 0;
}