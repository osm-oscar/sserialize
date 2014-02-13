#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <stdlib.h>
#include <vector>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/printers.h>
#include "datacreationfuncs.h"

using namespace sserialize;

template<uint32_t T_SET_COUNT, uint32_t T_MAX_SET_FILL, ItemIndex::Types T_IDX_TYPE>
class ItemIndexFactoryTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( ItemIndexFactoryTest );
CPPUNIT_TEST( testSerializedEquality );
CPPUNIT_TEST( testSameId );
CPPUNIT_TEST( testCompressionHuffman );
CPPUNIT_TEST( testCompressionLZO );
CPPUNIT_TEST( testCompressionVarUint );
CPPUNIT_TEST( testInitFromStatic );
CPPUNIT_TEST_SUITE_END();
private:
	ItemIndexFactory m_idxFactory;
	std::vector< std::set<uint32_t> > m_sets;
	std::vector<uint32_t> m_setIds;
public:
	ItemIndexFactoryTest() : m_idxFactory(true) {
		m_idxFactory.setRegline(true);
		m_idxFactory.setBitWith(-1);
		m_idxFactory.setType(T_IDX_TYPE);
	}

	virtual void setUp() {
		m_idxFactory.setIndexFile( UByteArrayAdapter::createCache(T_SET_COUNT*T_MAX_SET_FILL, false) );
		m_sets.reserve(T_SET_COUNT);
		m_setIds.reserve(T_SET_COUNT);
		
		m_sets.push_back(std::set<uint32_t>() ); //push the empty set as the index store always has it
		
		for(uint32_t i = 0; i < T_SET_COUNT; ++i) {
			m_sets.push_back( createNumbersSet( ((double)rand()/RAND_MAX) * T_MAX_SET_FILL ) );
		}
		
		for(uint32_t i = 0; i< m_sets.size(); ++i) {
			uint32_t id = m_idxFactory.addIndex(m_sets[i]);
			m_setIds.push_back(id);
		}
	}
	virtual void tearDown() {}
	
	void testSameId() {
		for(uint32_t i = 0; i < m_sets.size(); ++i) {
			uint32_t id = m_idxFactory.addIndex(m_sets[i]);
			CPPUNIT_ASSERT_EQUAL(m_setIds[i], id);
		}
	}
	
	void testSerializedEquality() {
		
		CPPUNIT_ASSERT_MESSAGE("Serialization failed", m_idxFactory.flush());

		UByteArrayAdapter dataAdap( m_idxFactory.getFlushedData());


		Static::ItemIndexStore sdb(dataAdap);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("ItemIndexFactory.size() != ItemIndexStore.size()", m_idxFactory.size(), sdb.size());

		for(size_t i = 0; i < m_sets.size(); ++i) {
			ItemIndex idx = sdb.at( m_setIds[i] );
			std::stringstream ss;
			ss << "Index at " << i;
			CPPUNIT_ASSERT_MESSAGE(ss.str(), m_sets[i] == idx);
		}
	}
	
	void testCompressionVarUint() {
		if(T_IDX_TYPE == ItemIndex::T_WAH) {
		
			CPPUNIT_ASSERT_MESSAGE("Serialization failed", m_idxFactory.flush());

			UByteArrayAdapter dataAdap( m_idxFactory.getFlushedData());
			UByteArrayAdapter cmpDataAdap(new std::vector<uint8_t>(dataAdap.size(), 0), true);


			Static::ItemIndexStore sdb(dataAdap);
			sserialize::ItemIndexFactory::compressWithVarUint(sdb, cmpDataAdap);
			Static::ItemIndexStore csdb(cmpDataAdap);
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("ItemIndexFactory.size() != ItemIndexStore.size()", m_idxFactory.size(), csdb.size());

			for(size_t i = 0; i < m_sets.size(); ++i) {
				ItemIndex idx = csdb.at( m_setIds[i] );
				std::stringstream ss;
				ss << "Index at " << i;
				CPPUNIT_ASSERT_MESSAGE(ss.str(), m_sets[i] == idx);
			}
		}
	}
	
	void testCompressionHuffman() {
		if(T_IDX_TYPE == ItemIndex::T_WAH) {
		
			CPPUNIT_ASSERT_MESSAGE("Serialization failed", m_idxFactory.flush());

			UByteArrayAdapter dataAdap( m_idxFactory.getFlushedData());
			UByteArrayAdapter cmpDataAdap(new std::vector<uint8_t>(dataAdap.size(), 0), true);


			Static::ItemIndexStore sdb(dataAdap);
			UByteArrayAdapter::OffsetType s = sserialize::ItemIndexFactory::compressWithHuffman(sdb, cmpDataAdap);
			cmpDataAdap.shrinkStorage(cmpDataAdap.size()-s);
			Static::ItemIndexStore csdb(cmpDataAdap);
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("ItemIndexFactory.size() != ItemIndexStore.size()", m_idxFactory.size(), csdb.size());

			for(size_t i = 0; i < m_sets.size(); ++i) {
				ItemIndex idx = csdb.at( m_setIds[i] );
				std::stringstream ss;
				ss << "Index at " << i;
				CPPUNIT_ASSERT_MESSAGE(ss.str(), m_sets[i] == idx);
			}
		}
	}
	
	void testCompressionLZO() {
		
		CPPUNIT_ASSERT_MESSAGE("Serialization failed", m_idxFactory.flush());

		UByteArrayAdapter dataAdap( m_idxFactory.getFlushedData());
		UByteArrayAdapter cmpDataAdap(new std::vector<uint8_t>(dataAdap.size(), 0), true);

		Static::ItemIndexStore sdb(dataAdap);
		UByteArrayAdapter::OffsetType s = sserialize::ItemIndexFactory::compressWithLZO(sdb, cmpDataAdap);
		cmpDataAdap.shrinkStorage(cmpDataAdap.size()-s);
		Static::ItemIndexStore csdb(cmpDataAdap);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("ItemIndexFactory.size() != ItemIndexStore.size()", m_idxFactory.size(), csdb.size());
		CPPUNIT_ASSERT_MESSAGE("compression type not set", csdb.compressionType() & Static::ItemIndexStore::IC_LZO);

		for(size_t i = 0; i < m_sets.size(); ++i) {
			ItemIndex idx = csdb.at( m_setIds[i] );
			std::stringstream ss;
			ss << "Index at " << i;
			CPPUNIT_ASSERT_MESSAGE(ss.str(), m_sets[i] == idx);
		}
	}
	
	void testVeryLargeItemIndexFactory() {
	
	}
	
	void testInitFromStatic() {
		CPPUNIT_ASSERT_MESSAGE("Serialization failed", m_idxFactory.flush());

		Static::ItemIndexStore sdb(m_idxFactory.getFlushedData());
		
		sserialize::ItemIndexFactory idxFactory;
		idxFactory.setRegline(true);
		idxFactory.setBitWith(-1);
		idxFactory.setType(T_IDX_TYPE);
		idxFactory.setIndexFile( UByteArrayAdapter::createCache(T_SET_COUNT*T_MAX_SET_FILL, false) );
		std::vector<uint32_t> remap = idxFactory.fromIndexStore(sdb);
		for(uint32_t i = 0, s = remap.size(); i < s; ++i) {
			sserialize::ItemIndex real = sdb.at(i);
			sserialize::ItemIndex testIdx = idxFactory.getIndex(remap.at(i));
			CPPUNIT_ASSERT_EQUAL_MESSAGE("idx", real, testIdx);
		}
		
		remap = idxFactory.fromIndexStore(sdb);
		for(uint32_t i = 0, s = remap.size(); i < s; ++i) {
			sserialize::ItemIndex real = sdb.at(i);
			sserialize::ItemIndex testIdx = idxFactory.getIndex(remap.at(i));
			CPPUNIT_ASSERT_EQUAL_MESSAGE("idx second add", real, testIdx);
		}
		
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
// 	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_REGLINE>::suite() );
// 	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_DE>::suite() );
// 	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_RLE_DE>::suite() );
	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_WAH>::suite() );
	runner.addTest(  ItemIndexFactoryTest<4047, 1001, ItemIndex::T_WAH>::suite() );
	runner.addTest(  ItemIndexFactoryTest<10537, 2040, ItemIndex::T_WAH>::suite() );
	runner.run();
	return 0;
}