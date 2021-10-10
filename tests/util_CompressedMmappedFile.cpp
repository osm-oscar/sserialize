#include <sserialize/storage/CompressedMmappedFile.h>
#include <sserialize/storage/MmappedFile.h>
#include <cmath>
#include <limits>
#include <stdlib.h>
#include "TestBase.h"


using namespace sserialize;

template<std::size_t FileSize, uint8_t chunkExponent, uint32_t TestNumber>
class CompressedMmappedFileTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( CompressedMmappedFileTest );
CPPUNIT_TEST( testStats );
CPPUNIT_TEST( testSequentialRead );
CPPUNIT_TEST( testRandomRead );
CPPUNIT_TEST( testReadFunction );
CPPUNIT_TEST_SUITE_END();
private:
	std::string m_fileName;
	sserialize::CompressedMmappedFile m_file;
	std::vector<uint8_t> m_realValues;
	UByteArrayAdapter m_compressedData;
public:
	CompressedMmappedFileTest() {
		std::stringstream ss;
		ss << "compressedmmappedfiletest" << TestNumber << ".bin";
		m_fileName = ss.str();
	}
	
	virtual void setUp() {
		m_realValues.reserve(FileSize);

		//push compressible data
		for(size_t i = 0; i < FileSize/2; ++i) {
			m_realValues.push_back(i);
		}

		for(size_t i = 0; i < FileSize; ++i) {
			m_realValues.push_back( rand() );
		}
		

		
		m_compressedData = UByteArrayAdapter::createFile(m_realValues.size(), m_fileName);
		m_compressedData.setDeleteOnClose(true);
		CompressedMmappedFile::create(UByteArrayAdapter(&m_realValues, false), m_compressedData, chunkExponent, 1.0);
		m_compressedData.sync();
		
		m_file = CompressedMmappedFile(m_fileName);
		m_file.setCacheCount(4);
		
		CPPUNIT_ASSERT_MESSAGE("opening", m_file.open());
	}

	virtual void tearDown() {
		CPPUNIT_ASSERT_MESSAGE("closing", m_file.close());
	}

	void  testStats() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Size", m_realValues.size(), m_file.size());
	}
	
	void testSequentialRead() {
		CPPUNIT_ASSERT_EQUAL(m_realValues.size(), m_file.size());
		
		for(uint32_t i = 0; i < m_realValues.size(); ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("value", static_cast<uint32_t>(m_realValues[i]), static_cast<uint32_t>(m_file[i]));
		}
	}
	
	void testRandomRead() {
		CPPUNIT_ASSERT_EQUAL(m_realValues.size(), m_file.size());
		
		std::size_t runs = std::min<std::size_t>(m_realValues.size(), 10000);
		for(uint32_t i = 0; i < runs; ++i) {
			std::size_t pos = std::min<std::size_t>( (double) std::rand()/RAND_MAX * m_realValues.size(), m_realValues.size()-1);
			std::stringstream ss;
			ss << "value at " << pos << " in run " << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), static_cast<uint32_t>(m_realValues[pos]), static_cast<uint32_t>(m_file[pos]));
		}
	}
	
	void testReadFunction() {
		for(size_t offset = 0; offset < m_realValues.size();) {
			SizeType len = (double) std::rand()/RAND_MAX * (1 << 20);
			uint8_t buf[len];
			m_file.read(offset, buf, len);
			for(size_t i = 0; i< len; ++i) {
				CPPUNIT_ASSERT_EQUAL(static_cast<SizeType>( m_realValues[offset+i] ), static_cast<SizeType>(buf[i]) );
			}
			offset += len;
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( CompressedMmappedFileTest<548576, 16, 4>::suite() ); //~0.5 MebiByte, 64 kb chunk size
	runner.addTest( CompressedMmappedFileTest<182137657, 20, 0>::suite() ); //about 173 MebiBytes, 1 megbyte chunk size
	runner.addTest( CompressedMmappedFileTest<18213765, 22, 5>::suite() ); //about 17.3 MebiBytes, 4 megbyte chunk size
	runner.addTest( CompressedMmappedFileTest<1048576, 24, 2>::suite() ); //1 MebiByte, 16 mb chunk size
	runner.addTest( CompressedMmappedFileTest<16777216, 21, 1>::suite() ); //16 MebiBytes, 2 mb chunk sizte
	runner.addTest( CompressedMmappedFileTest<2048576, 20, 3>::suite() ); //~2 MebiByte, 1 mb chunk size

	if (sserialize::tests::TestBase::popProtector()) {
		runner.eventManager().popProtector();
	}
	
	bool ok = runner.run();
	return ok ? 0 : 1;
}
