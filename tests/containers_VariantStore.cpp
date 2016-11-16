#include <stdlib.h>
#include <vector>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/VariantStore.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/Static/Array.h>
#include <sserialize/utility/printers.h>
#include "datacreationfuncs.h"
#include "TestBase.h"

using namespace sserialize;

template<uint32_t T_SET_COUNT, uint32_t T_MAX_SET_FILL, ItemIndex::Types T_IDX_TYPE>
class VariantStoreTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( VariantStoreTest );
CPPUNIT_TEST( testSerializedEquality );
CPPUNIT_TEST( testSameIdDedup );
CPPUNIT_TEST( testSameIdNoDedup );
CPPUNIT_TEST_SUITE_END();
private:
	VariantStore m_dsFactory;
	std::vector< std::set<uint32_t> > m_sets;
	std::vector<uint32_t> m_setIds;
public:
	VariantStoreTest() : m_dsFactory(sserialize::MM_PROGRAM_MEMORY) {}

	virtual void setUp() {
		m_sets.reserve(T_SET_COUNT);
		m_setIds.reserve(T_SET_COUNT);
		
		m_sets.push_back(std::set<uint32_t>() ); //push the empty set as the index store always has it
		
		for(uint32_t i = 0; i < T_SET_COUNT; ++i) {
			m_sets.push_back( createNumbersSet( ((double)rand()/RAND_MAX) * T_MAX_SET_FILL ) );
		}
		
		for(uint32_t i = 0; i< m_sets.size(); ++i) {
			sserialize::UByteArrayAdapter tmp(new std::vector<uint8_t>(), true);
			sserialize::ItemIndexFactory::create(m_sets[i], tmp, T_IDX_TYPE);
			VariantStore::SizeType id = m_dsFactory.insert(tmp, VariantStore::DDM_FORCE_ON);
			m_setIds.push_back((uint32_t) id);
		}
	}
	virtual void tearDown() {}
	
	void testSameIdDedup() {
		for(uint32_t i = 0; i < m_sets.size(); ++i) {
			sserialize::UByteArrayAdapter tmp(new std::vector<uint8_t>(), true);
			sserialize::ItemIndexFactory::create(m_sets[i], tmp, T_IDX_TYPE);
			VariantStore::SizeType id = m_dsFactory.insert(tmp, VariantStore::DDM_FORCE_ON);
			if (id != m_setIds[i]) {
				m_dsFactory.insert(tmp, VariantStore::DDM_FORCE_ON);
			}
			CPPUNIT_ASSERT_EQUAL_MESSAGE(std::string("at ") + std::to_string(i), m_setIds[i], (uint32_t) id);
		}
	}
	
	void testSameIdNoDedup() {
		for(uint32_t i = 0; i < m_sets.size(); ++i) {
			sserialize::UByteArrayAdapter tmp(new std::vector<uint8_t>(), true);
			sserialize::ItemIndexFactory::create(m_sets[i], tmp, T_IDX_TYPE);
			VariantStore::SizeType expectId = m_dsFactory.size();
			VariantStore::SizeType id = m_dsFactory.insert(tmp, VariantStore::DDM_FORCE_OFF);
			CPPUNIT_ASSERT_EQUAL(expectId, id);
		}
	}
	
	void testSerializedEquality() {
		m_dsFactory.flush();

		UByteArrayAdapter dataAdap( m_dsFactory.getFlushedData());
		Static::Array<sserialize::UByteArrayAdapter> sdb(dataAdap);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("VariantStore.size() != VariantStore.size()", m_dsFactory.size(), (VariantStore::SizeType) sdb.size());

		for(size_t i = 0; i < m_sets.size(); ++i) {
			ItemIndex idx(sdb.at( m_setIds[i] ), T_IDX_TYPE);
			std::stringstream ss;
			ss << "Index at " << i;
			CPPUNIT_ASSERT_MESSAGE(ss.str(), m_sets[i] == idx);
		}
	}

};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::argc = argc;
	sserialize::tests::TestBase::argv = argv;
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  VariantStoreTest<64, 512, ItemIndex::T_SIMPLE>::suite() );
	runner.addTest(  VariantStoreTest<2048, 512, ItemIndex::T_REGLINE>::suite() );
	runner.addTest(  VariantStoreTest<2048, 512, ItemIndex::T_DE>::suite() );
	runner.addTest(  VariantStoreTest<2048, 512, ItemIndex::T_RLE_DE>::suite() );
	runner.addTest(  VariantStoreTest<2048, 512, ItemIndex::T_WAH>::suite() );
	runner.addTest(  VariantStoreTest<4047, 1001, ItemIndex::T_WAH>::suite() );
	runner.addTest(  VariantStoreTest<10537, 2040, ItemIndex::T_WAH>::suite() );
	runner.addTest(  VariantStoreTest<10537, 2040, ItemIndex::T_NATIVE>::suite() );
	runner.run();
	return 0;
}