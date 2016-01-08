#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestResult.h>
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
CPPUNIT_TEST( testReplace );
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
	void testReplace() {
		for(uint32_t i(0); i < TTestCount; ++i) {
			m_rd.clear();
			m_d.clear();
			for(uint32_t j(0); j < TEntrySize; ++j) {
				m_rd.push_back(TestData());
			}
			auto dit = m_d.replace(m_d.begin(), m_rd.begin(), m_rd.end());
			CPPUNIT_ASSERT_MESSAGE("returned iterator is wrong after random long replace", dit == m_d.end());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", m_rd.size(), m_d.size());

			auto rdit = m_rd.begin();
			for(OOMCI it(m_d.begin()), end(m_d.end()); it != end; ++it, ++rdit) {
				CPPUNIT_ASSERT_MESSAGE("entries are unequal after initial insert", *rdit == *it);
			}
			
			for(uint32_t j(0); j < TTestCount; ++j) {
				uint32_t replacePos = rand() % m_d.size();
				std::vector<TestData> tmp;
				for(uint32_t k(0), s(rand() % 1000); k < s; ++k) {
					tmp.push_back(TestData());
				}
				if (m_rd.size() < replacePos + tmp.size()) {
					m_rd.resize(replacePos+tmp.size());
				}
				for(uint32_t k(0); k < tmp.size(); ++k) {
					m_rd[k+replacePos] = tmp[k];
				}
				m_d.replace(m_d.begin()+replacePos, tmp.begin(), tmp.end());
			}
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size after random replace", m_rd.size(), m_d.size());
			rdit = m_rd.begin();
			for(OOMCI it(m_d.begin()), end(m_d.end()); it != end; ++it, ++rdit) {
				CPPUNIT_ASSERT_MESSAGE("entry[" +  std::to_string(rdit-m_rd.begin()) + "] are unequal after random replace", *rdit == *it);
			}
			
			m_d.flush();
			uint32_t replacePos = rand() % m_rd.size();
			uint32_t replaceCount = m_d.size() + m_d.backBufferSize()/2;
			if (replacePos + replaceCount > m_rd.size()) {
				m_rd.resize(replacePos+replaceCount);
			}
			for(uint32_t j(0); j < replaceCount; ++j) {
				m_rd[replacePos+j] = TestData();
			}

			dit = m_d.replace(m_d.begin()+replacePos, m_rd.begin()+replacePos, m_rd.end());
			CPPUNIT_ASSERT_MESSAGE("returned iterator is wrong after random long replace", dit == m_d.end());
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size after random long replace", m_rd.size(), m_d.size());
			rdit = m_rd.begin();
			for(OOMCI it(m_d.begin()), end(m_d.end()); it != end; ++it, ++rdit) {
				CPPUNIT_ASSERT_MESSAGE("entries[" +  std::to_string(rdit-m_rd.begin()) + "] are unequal after long replace", *rdit == *it);
			}
			
			uint32_t backInsertPos = m_rd.size();
			for(uint32_t j(0), s(2*m_d.backBufferSize()); j < s; ++j) {
				m_rd.push_back(TestData());
			}
			
			m_d.replace(m_d.end(), m_rd.begin()+backInsertPos, m_rd.end());
			
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size after replace at end", m_rd.size(), m_d.size());
			rdit = m_rd.begin();
			for(OOMCI it(m_d.begin()), end(m_d.end()); it != end; ++it, ++rdit) {
				CPPUNIT_ASSERT_MESSAGE("entries[" +  std::to_string(rdit-m_rd.begin()) + "] are unequal after replace at the end", *rdit == *it);
			}
		}
	}
};

int main(int argc, char ** argv) {

	bool testLarge = false;
	if (argc > 1 && std::string(argv[1]) == "-l") {
		testLarge = true;
	}

	srand( 1 );
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

	srand( 2 );
	runner.addTest(  TestOOMArray<32, 1000, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 1000, sserialize::MM_SHARED_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<32, 1000, sserialize::MM_FAST_FILEBASED>::suite() );
	runner.addTest(  TestOOMArray<32, 1000, sserialize::MM_SLOW_FILEBASED>::suite() );
	
	srand( 3 );
	runner.addTest(  TestOOMArray<16, 2000, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<16, 2000, sserialize::MM_SHARED_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<16, 2000, sserialize::MM_FAST_FILEBASED>::suite() );
	runner.addTest(  TestOOMArray<16, 2000, sserialize::MM_SLOW_FILEBASED>::suite() );

	srand( 4 );
	runner.addTest(  TestOOMArray<8, 100000, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<8, 100000, sserialize::MM_SHARED_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<8, 100000, sserialize::MM_FAST_FILEBASED>::suite() );
	runner.addTest(  TestOOMArray<8, 100000, sserialize::MM_SLOW_FILEBASED>::suite() );

	srand( 5 );
	runner.addTest(  TestOOMArray<8, 1000000, sserialize::MM_PROGRAM_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<8, 1000000, sserialize::MM_SHARED_MEMORY>::suite() );
	runner.addTest(  TestOOMArray<8, 1000000, sserialize::MM_FAST_FILEBASED>::suite() );
	runner.addTest(  TestOOMArray<8, 1000000, sserialize::MM_SLOW_FILEBASED>::suite() );
	
	if (testLarge) {
		srand( 6 );
		runner.addTest(  TestOOMArray<1, 100000000, sserialize::MM_PROGRAM_MEMORY>::suite() );
		runner.addTest(  TestOOMArray<1, 100000000, sserialize::MM_SHARED_MEMORY>::suite() );
		runner.addTest(  TestOOMArray<1, 100000000, sserialize::MM_FAST_FILEBASED>::suite() );
		runner.addTest(  TestOOMArray<1, 100000000, sserialize::MM_SLOW_FILEBASED>::suite() );
	}
// 	runner.eventManager().popProtector();
	runner.run();
	return 0;
}