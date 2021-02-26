#include <vector>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/printers.h>
#include "datacreationfuncs.h"
#include "TestBase.h"
#include <random>

using namespace sserialize;

template<uint32_t TValueCount, uint32_t TSubValueCount>
class MultiVarBitArrayTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( MultiVarBitArrayTest );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST( testBitCount );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST_SUITE_END();
private:
	using value_type = MultiVarBitArray::value_type;
	using SizeType = MultiVarBitArray::SizeType;
private:
	std::vector< std::vector<value_type> > m_values;
	std::vector<uint8_t> m_bitConfig;
	UByteArrayAdapter m_data;
	MultiVarBitArray m_arr;
public:
	MultiVarBitArrayTest() : m_data(new std::vector<uint8_t>(), true) {}
	virtual void setUp() {
		auto bit_dist = std::uniform_int_distribution<uint8_t>(1, 64);
		auto value_dist = std::uniform_int_distribution<uint64_t>();
		auto gen = std::mt19937(0);
		for(size_t i = 0; i < TSubValueCount; i++) {
			m_bitConfig.push_back(bit_dist(gen));
		}
		
		for(size_t i = 0; i < TValueCount; i++) {
			std::vector<value_type> subValues;
			for(size_t j = 0; j < TSubValueCount; j++) {
				subValues.push_back( value_dist(gen) & createMask64(m_bitConfig[j]) );
			}
			m_values.push_back(subValues);
		}
		
		
		m_data.putUint32(0xFEFE);
		m_data.reserveFromPutPtr(40);
		
		MultiVarBitArrayCreator creator(m_bitConfig, m_data);
		creator.reserve(TValueCount);
		
		for(uint32_t i = 0; i < TValueCount; i++) {
			for(uint32_t j = 0; j < TSubValueCount; j++) {
				creator.set(i,j, m_values[i][j] );
			}
		}
		UByteArrayAdapter fd = creator.flush();
		
		m_arr = MultiVarBitArray(fd);
		
	}
	virtual void tearDown() {}
	
	void testEquality() {
		for(uint32_t i = 0; i < TValueCount; i++) {
			for(uint32_t j = 0; j < TSubValueCount; j++) {
				std::stringstream ss;
				ss << "pos=" << i << ", subpos=" << j;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), m_values[i][j], m_arr.at(i,j)  );
			}
		}
	}
	
	void testSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size()", SizeType(TValueCount), m_arr.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("getSizeInBytes()", (UByteArrayAdapter::OffsetType) m_data.tellPutPtr()-4, m_arr.getSizeInBytes());
	}
	
	void testBitCount() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("bitconfig.size() is wrong", (uint32_t) m_bitConfig.size(), m_arr.bitConfigCount());
		for(uint32_t i = 0; i < m_bitConfig.size(); i++) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("bitconfig is broken", m_bitConfig.at(i), m_arr.bitCount(i));
		}
	}
};

class MultiVarBitArraySpecialTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( MultiVarBitArraySpecialTest );
CPPUNIT_TEST( testLargeSpecialEquality );
CPPUNIT_TEST_SUITE_END();
public:
	void testLargeSpecialEquality() {
		uint32_t size = 0x63E70FF;
		uint32_t entry0 = 2011012750;
		uint32_t entry1 = 747;
		UByteArrayAdapter dest(UByteArrayAdapter::createCache(1, MM_PROGRAM_MEMORY));
		MultiVarBitArrayCreator tsCreator(
			std::vector<uint8_t>({(uint8_t)CompactUintArray::minStorageBits(entry0),
									(uint8_t)CompactUintArray::minStorageBits(entry1)}
		), dest);
		tsCreator.set(0, 0, 1891253);
		tsCreator.set(0, 1, 0);
		tsCreator.set(104755300, 0, 1242567038);
		tsCreator.set(104755300, 1, 9);
		CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("first entry missmatch at ", 104755300), (uint32_t)1891253, (uint32_t)tsCreator.at(0, 0));
		CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("first entry missmatch at ", 104755300), (uint32_t)0, (uint32_t)tsCreator.at(0, 1));
		for(uint32_t i(1); i < size; ++i) {
			tsCreator.set(i, 0, 1242567038);
			tsCreator.set(i, 1, 9);
			if ((uint32_t)1891253 != (uint32_t)tsCreator.at(0, 0)) {
			tsCreator.set(0, 0, 1891253);
			tsCreator.set(0, 1, 0);
			tsCreator.set(i, 0, 1242567038);
			tsCreator.set(i, 1, 9);
			}
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("first entry missmatch at ", i), (uint32_t)1891253, (uint32_t)tsCreator.at(0, 0));
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("first entry missmatch at ", i), (uint32_t)0, (uint32_t)tsCreator.at(0, 1));
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( MultiVarBitArrayTest<16, 1>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 1>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 2>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 7>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 13>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 31>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 32>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 127>::suite() );
	runner.addTest( MultiVarBitArrayTest<128*1024, 3>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024*1024, 3>::suite() );
	runner.addTest( MultiVarBitArraySpecialTest::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}

