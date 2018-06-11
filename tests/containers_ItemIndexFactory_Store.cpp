#include <stdlib.h>
#include <vector>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/printers.h>
#include "datacreationfuncs.h"
#include "TestBase.h"

using namespace sserialize;

template<uint32_t T_SET_COUNT, uint32_t T_MAX_SET_FILL, int T_IDX_TYPE>
class ItemIndexFactoryTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( ItemIndexFactoryTest );
CPPUNIT_TEST( testSameId );
CPPUNIT_TEST( testIdxSize );
CPPUNIT_TEST( testIdxFromId );
CPPUNIT_TEST( testInitFromStatic );
CPPUNIT_TEST( testSerializedEquality );
// CPPUNIT_TEST( testCompressionHuffman );
CPPUNIT_TEST( testCompressionLZO );
CPPUNIT_TEST( testCompressionVarUint );
CPPUNIT_TEST_SUITE_END();
private:
	ItemIndexFactory m_idxFactory;
	std::vector< std::set<uint32_t> > m_sets;
	std::vector<uint32_t> m_setIds;
public:
	ItemIndexFactoryTest() : m_idxFactory(true) {
		m_idxFactory.setType(T_IDX_TYPE);
	}

	virtual void setUp() {
		m_idxFactory.setIndexFile( UByteArrayAdapter::createCache(T_SET_COUNT*T_MAX_SET_FILL, sserialize::MM_PROGRAM_MEMORY) );
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
	
	void testIdxSize() {
		for(uint32_t i = 0; i < m_sets.size(); ++i) {
			uint32_t idxSize = m_idxFactory.idxSize(m_setIds[i]);
			CPPUNIT_ASSERT_EQUAL((uint32_t)m_sets[i].size(), idxSize);
		}
	}
	
	void testIdxFromId() {
		for(uint32_t i = 0; i < m_sets.size(); ++i) {
			ItemIndex idx = m_idxFactory.indexById(m_setIds[i]);
			CPPUNIT_ASSERT(m_sets[i] == idx);
		}
	}
	
	void testSerializedEquality() {
		auto flushedDataSize = m_idxFactory.flush();
		
		CPPUNIT_ASSERT_MESSAGE("Serialization failed", flushedDataSize);

		UByteArrayAdapter dataAdap( m_idxFactory.getFlushedData());
		Static::ItemIndexStore sdb(dataAdap);
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("ItemIndexFactory.size() != ItemIndexStore.size()", m_idxFactory.size(), sdb.size());

		for(size_t i = 0; i < m_sets.size(); ++i) {
			uint32_t idxId = m_setIds[i];
			CPPUNIT_ASSERT_EQUAL_MESSAGE("Index Type at " + std::to_string(i), m_idxFactory.type(idxId), sdb.indexType(idxId));
			ItemIndex idx = sdb.at( idxId );
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Index at ", i), m_sets[i] == idx);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("Index size at", i), m_idxFactory.idxSize(idxId), sdb.idxSize(idxId));
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
		CPPUNIT_ASSERT_MESSAGE("compression type not set", csdb.compressionType() & Static::ItemIndexStore::IndexCompressionType::IC_LZO);

		for(size_t i = 0; i < m_sets.size(); ++i) {
			uint32_t idxId = m_setIds[i];
			ItemIndex idx = csdb.at(idxId);
			uint32_t realIndexSize =  (uint32_t) m_sets[i].size();
			CPPUNIT_ASSERT_MESSAGE(sserialize::toString("Index at", i), m_sets[i] == idx);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("IndexIndexStore.idxSize at", i), realIndexSize, sdb.idxSize(idxId));
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("IndexIndexStore.idxSize at", i), realIndexSize, csdb.idxSize(idxId));
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("Static::Index.size at", i), realIndexSize, idx.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("Index.size at", i), realIndexSize, csdb.idxSize(idxId));
		}
	}
	
	void testVeryLargeItemIndexFactory() {
	
	}
	
	void testInitFromStatic() {
		CPPUNIT_ASSERT_MESSAGE("Serialization failed", m_idxFactory.flush());

		Static::ItemIndexStore sdb(m_idxFactory.getFlushedData());
		
		sserialize::ItemIndexFactory idxFactory;
		idxFactory.setType(T_IDX_TYPE);
		idxFactory.setIndexFile( UByteArrayAdapter::createCache(T_SET_COUNT*T_MAX_SET_FILL, sserialize::MM_PROGRAM_MEMORY) );
		std::vector<uint32_t> remap = idxFactory.insert(sdb);
		for(uint32_t i = 0, s = (uint32_t) remap.size(); i < s; ++i) {
			sserialize::ItemIndex real = sdb.at(i);
			sserialize::ItemIndex testIdx = idxFactory.indexById(remap.at(i));
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("remap id at ", i), i, remap.at(i));
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("size at ", i), real.size(), testIdx.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("idx at ", i), real, testIdx);
		}
		
		remap = idxFactory.insert(sdb);
		for(uint32_t i = 0, s = (uint32_t) remap.size(); i < s; ++i) {
			sserialize::ItemIndex real = sdb.at(i);
			sserialize::ItemIndex testIdx = idxFactory.indexById(remap.at(i));
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", real.size(), testIdx.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("idx second add", real, testIdx);
		}
		
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	if (sserialize::tests::TestBase::printHelp()) {
		return 0;
	}
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  ItemIndexFactoryTest<64, 512, ItemIndex::T_SIMPLE>::suite() );
	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_REGLINE>::suite() );
	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_DE>::suite() );
	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_RLE_DE>::suite() );
	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_WAH>::suite() );
	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_PFOR>::suite() );
	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_MULTIPLE|ItemIndex::T_RLE_DE|ItemIndex::T_NATIVE>::suite() );
	runner.addTest(  ItemIndexFactoryTest<2048, 512, ItemIndex::T_MULTIPLE|ItemIndex::T_PFOR|ItemIndex::T_RLE_DE|ItemIndex::T_ELIAS_FANO|ItemIndex::T_WAH>::suite() );
	runner.addTest(  ItemIndexFactoryTest<4047, 1001, ItemIndex::T_WAH>::suite() );
	runner.addTest(  ItemIndexFactoryTest<10537, 2040, ItemIndex::T_WAH>::suite() );
	runner.addTest(  ItemIndexFactoryTest<10537, 2040, ItemIndex::T_NATIVE>::suite() );
	runner.addTest(  ItemIndexFactoryTest<10537, 2040, ItemIndex::T_NATIVE>::suite() );
	if (sserialize::tests::TestBase::popProtector()) {
		runner.eventManager().popProtector();
	}
	bool ok = runner.run();
	return ok ? 0 : 1;
}
