#include <sserialize/storage/ChunkedMmappedFile.h>
#include <sserialize/storage/MmappedFile.h>
#include <cmath>
#include <limits>
#include <stdlib.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>


using namespace sserialize;

template<uint32_t FileSize, uint8_t chunkExponent, uint32_t TestNumber>
class ChunkedMmappedFileTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( ChunkedMmappedFileTest );
CPPUNIT_TEST( testStats );
CPPUNIT_TEST( testSequentialRead );
CPPUNIT_TEST( testRandomRead );
CPPUNIT_TEST( testReadFunction );
CPPUNIT_TEST_SUITE_END();
private:
	bool m_deleteOnClose;
	std::string m_fileName;
	sserialize::ChunkedMmappedFile m_file;
	std::vector<uint8_t> m_realValues;
public:
	ChunkedMmappedFileTest() {
		std::stringstream ss;
		ss << "chunkedmmappedfiletest" << TestNumber << ".bin";
		m_fileName = ss.str();
		m_deleteOnClose = true;
	}
	
	virtual void setUp() {
		m_realValues.reserve(FileSize);
		for(size_t i = 0; i < FileSize; ++i) {
			m_realValues.push_back( rand() );
		}
		m_file = ChunkedMmappedFile(m_fileName, chunkExponent, true);
		m_file.setDeleteOnClose(m_deleteOnClose);
		m_file.setSyncOnClose(true);
		m_file.setCacheCount(4);
		SizeType len = FileSize;
		m_file.write(&(m_realValues[0]), 0, len);
		
		CPPUNIT_ASSERT_MESSAGE("opening", m_file.open());
	}

	virtual void tearDown() {
		CPPUNIT_ASSERT_MESSAGE("closing", m_file.close());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("file exists", !m_deleteOnClose, MmappedFile::fileExists(m_fileName));
	}

	void  testStats() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Size", m_realValues.size(), m_file.size());
	}
	
	void testSequentialRead() {
		CPPUNIT_ASSERT_EQUAL(m_realValues.size(), m_file.size());
		
		for(std::size_t i = 0; i < m_realValues.size(); ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("value", m_realValues[i], m_file[i]);
		}
	}
	
	void testRandomRead() {
		CPPUNIT_ASSERT_EQUAL(m_realValues.size(), m_file.size());
		
		for(std::size_t i = 0; i < m_realValues.size(); ++i) {
			std::size_t pos = std::min<std::size_t>( (double) std::rand()/RAND_MAX * m_realValues.size(), m_realValues.size()-1);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("value", m_realValues[pos], m_file[pos]);
		}
	}
	
	void testReadFunction() {
		for(size_t offset = 0; offset < m_realValues.size();) {
			SizeType len = (double) std::rand()/RAND_MAX * (1 << 20);
			uint8_t buf[len];
			m_file.read(offset, buf, len);
			for(size_t i = 0; i< len; ++i) {
				CPPUNIT_ASSERT_EQUAL(static_cast<uint32_t>( m_realValues[offset+i] ), static_cast<uint32_t>(buf[i]) );
			}
			offset += len;
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( ChunkedMmappedFileTest<18213765, 22, 0>::suite() ); //aboutt 17.3 MebiBytes, 4 megbyte chunk size
	runner.addTest( ChunkedMmappedFileTest<1048576, 24, 2>::suite() ); //1 MebiByte, 16 mb chunk size
	runner.addTest( ChunkedMmappedFileTest<16777216, 21, 1>::suite() ); //16 MebiBytes, 2 mb chunk sizte
	runner.addTest( ChunkedMmappedFileTest<2048576, 20, 3>::suite() ); //~2 MebiByte, 1 mb chunk size
	runner.addTest( ChunkedMmappedFileTest<548576, 14, 3>::suite() ); //~0.5 MebiByte, 16 kb chunk size

	runner.run();
	return 0;
	
}