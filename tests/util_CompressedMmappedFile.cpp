#include <sserialize/utility/CompressedMmappedFile.h>
#include <sserialize/utility/mmappedfile.h>
#include <sserialize/utility/filewriter.h>
#include <cmath>
#include <limits>
#include <stdlib.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>


using namespace sserialize;

template<uint32_t FileSize, uint8_t chunkExponent, uint32_t TestNumber>
class CompressedMmappedFileTest: public CppUnit::TestFixture {
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
	std::vector<uint8_t> m_compressedValues;
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
		

		
		UByteArrayAdapter tmpCmpData(&m_compressedValues, false);
		CompressedMmappedFile::create(UByteArrayAdapter(&m_realValues, false), tmpCmpData, chunkExponent, 1.0);
		writeBytesToFile(m_fileName, m_compressedValues.begin(), m_compressedValues.end());
		
		m_file = CompressedMmappedFile(m_fileName);
		m_file.setCacheCount(4);
		
		CPPUNIT_ASSERT_MESSAGE("opening", m_file.open());
	}

	virtual void tearDown() {
		CPPUNIT_ASSERT_MESSAGE("closing", m_file.close());
		MmappedFile::unlinkFile(m_fileName);
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
		
		uint32_t runs = std::min<uint32_t>(m_realValues.size(), 10000);
		for(uint32_t i = 0; i < runs; ++i) {
			uint32_t pos = std::min<uint32_t>( (double) std::rand()/RAND_MAX * m_realValues.size(), m_realValues.size()-1);
			std::stringstream ss;
			ss << "value at " << pos << " in run " << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), static_cast<uint32_t>(m_realValues[pos]), static_cast<uint32_t>(m_file[pos]));
		}
	}
	
	void testReadFunction() {
		for(size_t offset = 0; offset < m_realValues.size();) {
			uint32_t len = (double) std::rand()/RAND_MAX * (1 << 20);
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
	runner.addTest( CompressedMmappedFileTest<182137657, 20, 0>::suite() ); //about 173 MebiBytes, 1 megbyte chunk size
	runner.addTest( CompressedMmappedFileTest<18213765, 22, 0>::suite() ); //about 17.3 MebiBytes, 4 megbyte chunk size
	runner.addTest( CompressedMmappedFileTest<1048576, 24, 2>::suite() ); //1 MebiByte, 16 mb chunk size
	runner.addTest( CompressedMmappedFileTest<16777216, 21, 1>::suite() ); //16 MebiBytes, 2 mb chunk sizte
	runner.addTest( CompressedMmappedFileTest<2048576, 20, 3>::suite() ); //~2 MebiByte, 1 mb chunk size
	runner.addTest( CompressedMmappedFileTest<548576, 16, 4>::suite() ); //~0.5 MebiByte, 64 kb chunk size

	runner.run();
	return 0;
	
}