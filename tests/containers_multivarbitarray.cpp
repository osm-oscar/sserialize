#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <vector>
#include <sserialize/containers/MultiVarBitArray.h>
#include "datacreationfuncs.h"

using namespace sserialize;

template<uint32_t TValueCount, uint32_t TSubValueCount>
class MultiVarBitArrayTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( MultiVarBitArrayTest );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST( testBitCount );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST_SUITE_END();
private:
	std::vector< std::vector<uint32_t> > m_values;
	std::vector<uint8_t> m_bitConfig;
	std::vector<uint8_t> m_data;
	MultiVarBitArray m_arr;
public:
	virtual void setUp() {
		for(size_t i = 0; i < TSubValueCount; i++) {
			m_bitConfig.push_back( (rand() & 0x1F)+1 );
		}
		
		for(size_t i = 0; i < TValueCount; i++) {
			std::vector<uint32_t> subValues;
			for(size_t j = 0; j < TSubValueCount; j++) {
				subValues.push_back( rand() & createMask(m_bitConfig[j]) );
			}
			m_values.push_back(subValues);
		}
		
		UByteArrayAdapter data(&m_data);
		data.putUint32(0xFEFE);
		
		MultiVarBitArrayCreator creator(m_bitConfig, data);
		creator.reserve(TValueCount);
		
		for(size_t i = 0; i < TValueCount; i++) {
			for(size_t j = 0; j < TSubValueCount; j++) {
				creator.set(i,j, m_values[i][j] );
			}
		}
		creator.flush();
		
		m_arr = MultiVarBitArray(creator.flush());
		
	}
	virtual void tearDown() {}
	
	void testEquality() {
		for(size_t i = 0; i < TValueCount; i++) {
			for(size_t j = 0; j < TSubValueCount; j++) {
				std::stringstream ss;
				ss << "pos=" << i << ", subpos=" << j;
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), m_values[i][j], m_arr.at(i,j)  );
			}
		}
	}
	
	void testSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size()", TValueCount, m_arr.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("getSizeInBytes()", (uint32_t) m_data.size()-4, m_arr.getSizeInBytes());
	}
	
	void testBitCount() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("bitconfig.size() is wrong", (uint32_t) m_bitConfig.size(), m_arr.bitConfigCount());
		for(size_t i = 0; i< m_bitConfig.size(); i++) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("bitconfig is broken", m_bitConfig.at(i), m_arr.bitCount(i));
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( MultiVarBitArrayTest<1024, 1>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 2>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 7>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 13>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 31>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 32>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024, 127>::suite() );
	runner.addTest( MultiVarBitArrayTest<128*1024, 3>::suite() );
	runner.addTest( MultiVarBitArrayTest<1024*1024, 3>::suite() );
	runner.run();
	return 0;
}

