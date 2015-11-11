#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestAssert.h>
#include <sserialize/containers/OOMArray.h>

struct TestData {
	uint8_t a;
	uint64_t b;
	uint16_t c;
	uint32_t d;
	TestData() : a(rand()), b(rand()), c(rand()), d(rand()) {}
	bool operator==(const TestData & other) const {
		return (a == other.a && b == other.b && c == other.c && d == other.d);
	}
};

template<uint32_t TTestCount, uint32_t TEntrySize, sserialize::MmappedMemoryType mmt>
class TestOOMArray: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( TestOOMArray );
CPPUNIT_TEST( testGetSet );
CPPUNIT_TEST( testPushBackGet );
CPPUNIT_TEST( testIteratorRead );
CPPUNIT_TEST( testIteratorWrite );
CPPUNIT_TEST_SUITE_END();
private:
	typedef sserialize::OOMArray<TestData> OOMA;
	typedef OOMA::const_iterator OOMCI;
	typedef OOMA::iterator OOMI;
private:
	std::vector<TestData> m_rd;
	sserialize::OOMArray<TestData> m_d;
public:
    TestOOMArray() : m_d(mmt) {}
	virtual ~TestOOMArray() {}
	virtual void setUp() {}
	virtual void tearDown() {
		m_rd.clear();
		m_rd.shrink_to_fit();
		m_d.clear();
		m_d.shrink_to_fit();
	}
	void testGetSet() {
		for(uint32_t i(0); i < TTestCount; ++i) {
			m_rd.clear();
			m_d.clear();
			for(uint32_t j(0); j < TEntrySize; ++j) {
				m_rd.push_back(TestData());
			}
			
			m_d.resize(TEntrySize);
			for(uint32_t i(0); i < TEntrySize; ++i) {
				m_d.set(i, m_rd.at(i));
			}
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", m_rd.size(), m_d.size());
			for(uint32_t i(0); i < TEntrySize; ++i) {
				CPPUNIT_ASSERT_MESSAGE("entries are unqueal", m_rd.at(i) == m_d.get(i));
			}
		}
	}
	void testPushBackGet() {
		for(uint32_t i(0); i < TTestCount; ++i) {
			m_rd.clear();
			m_d.clear();
			for(uint32_t j(0); j < TEntrySize; ++j) {
				m_rd.push_back(TestData());
			}
			for(auto x : m_rd) {
				m_d.push_back(x);
				CPPUNIT_ASSERT_MESSAGE("Back entry missmatch", m_d.back() == x);
			}
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", m_rd.size(), m_d.size());
			for(uint32_t i(0); i < TEntrySize; ++i) {
				CPPUNIT_ASSERT_MESSAGE("entries are unqueal", m_rd.at(i) == m_d.get(i));
			}
		}
	}
	void testIteratorRead() {
		for(uint32_t i(0); i < TTestCount; ++i) {
			m_rd.clear();
			m_d.clear();
			for(uint32_t j(0); j < TEntrySize; ++j) {
				m_rd.push_back(TestData());
			}
			for(auto x : m_rd) {
				m_d.push_back(x);
			}
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", m_rd.size(), m_d.size());
			
			auto rdit = m_rd.begin();
			for(OOMCI it(m_d.begin()), end(m_d.end()); it != end; ++it, ++rdit) {
				CPPUNIT_ASSERT_MESSAGE("entries are unequal", *rdit == *it);
			}
			rdit = m_rd.begin();
			for(OOMI it(m_d.begin()), end(m_d.end()); it != end; ++it, ++rdit) {
				CPPUNIT_ASSERT_MESSAGE("entries are unequal", *rdit == *it);
			}
		}
	}
	void testIteratorWrite() {
		for(uint32_t i(0); i < TTestCount; ++i) {
			m_rd.clear();
			m_d.clear();
			for(uint32_t j(0); j < TEntrySize; ++j) {
				m_rd.push_back(TestData());
			}
			m_d.resize(TEntrySize);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", m_rd.size(), m_d.size());

			auto rdit = m_rd.begin();
			for(OOMI it(m_d.begin()), end(m_d.end()); it != end; ++it, ++rdit) {
				it.set(*rdit);
			}
			
			rdit = m_rd.begin();
			for(OOMCI it(m_d.begin()), end(m_d.end()); it != end; ++it, ++rdit) {
				CPPUNIT_ASSERT_MESSAGE("entries are unequal", *rdit == *it);
			}
			rdit = m_rd.begin();
			for(OOMI it(m_d.begin()), end(m_d.end()); it != end; ++it, ++rdit) {
				CPPUNIT_ASSERT_MESSAGE("entries are unequal", *rdit == *it);
			}
		}
	}
};

int main(int argc, char ** argv) {

	bool testLarge = false;
	if (argc > 1 && std::string(argv[1]) == "-l") {
		testLarge = true;
	}

	srand( 0 );
	CppUnit::TextUi::TestRunner runner;

	runner.addTest(  TestOOMArray<32, 10, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 55, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 123, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 254, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 255, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 256, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 257, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 777, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 1023, sserialize::MM_PROGRAM_MEMORY>::suite() );


	runner.addTest(  TestOOMArray<32, 1000, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 1000, sserialize::MM_SHARED_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 1000, sserialize::MM_FAST_FILEBASED>::suite() );
	runner.addTest(  TestOOMArray<32, 1000, sserialize::MM_SLOW_FILEBASED>::suite() );
	
	runner.addTest(  TestOOMArray<16, 2000, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<16, 2000, sserialize::MM_SHARED_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<16, 2000, sserialize::MM_FAST_FILEBASED>::suite() );
	runner.addTest(  TestOOMArray<16, 2000, sserialize::MM_SLOW_FILEBASED>::suite() );

	runner.addTest(  TestOOMArray<8, 100000, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<8, 100000, sserialize::MM_SHARED_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<8, 100000, sserialize::MM_FAST_FILEBASED>::suite() );
	runner.addTest(  TestOOMArray<8, 100000, sserialize::MM_SLOW_FILEBASED>::suite() );

	
	runner.addTest(  TestOOMArray<8, 1000000, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<8, 1000000, sserialize::MM_SHARED_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<8, 1000000, sserialize::MM_FAST_FILEBASED>::suite() );
	runner.addTest(  TestOOMArray<8, 1000000, sserialize::MM_SLOW_FILEBASED>::suite() );
	
	if (testLarge) {
		runner.addTest(  TestOOMArray<1, 100000000, sserialize::MM_PROGRAM_MEMORY>::suite() );
		runner.addTest(  TestOOMArray<1, 100000000, sserialize::MM_SHARED_MEMORY>::suite() );
		runner.addTest(  TestOOMArray<1, 100000000, sserialize::MM_FAST_FILEBASED>::suite() );
		runner.addTest(  TestOOMArray<1, 100000000, sserialize::MM_SLOW_FILEBASED>::suite() );
	}
	runner.run();
	return 0;
}