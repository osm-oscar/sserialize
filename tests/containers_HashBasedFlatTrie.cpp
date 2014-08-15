#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestResult.h>
#include <sserialize/containers/HashBasedFlatTrie.h>

const char * inFileName = 0;

inline std::ostream & operator<<(std::ostream & out, const sserialize::detail::HashBasedFlatTrie::StaticString & str) {
	out.write(str.begin(), str.size());
	return out;
}

class TestHashBasedFlatTrie: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( TestHashBasedFlatTrie );
CPPUNIT_TEST( test );
CPPUNIT_TEST_SUITE_END();
private:
	typedef sserialize::HashBasedFlatTrie<uint32_t> MyT;
	MyT m_ht;
	std::vector<std::string> m_testStrings;
	std::vector<std::string> m_checkStrings;
public:
	TestHashBasedFlatTrie() :
	m_testStrings({//all: missing empty parent
		"A", //single
		"BB", "BC", "BD", //missing parent
		"C", "CD", "CE", //parent available with children 
		"DAAAA", "DAAAB", "DAAABE", "DAAAC", //longer node string, but missing parent
		"EAAAA", "EAAAAA", "EAAAAB", "EAAAAC", //longer node string, no missing parent
		"FF", "FEG", "FEHI", "FEHJ" //multiple missing parents
	}),
	m_checkStrings({
		"",
		"A", //single
		"B",
		"BB", "BC", "BD", //missing parent
		"C", "CD", "CE", //parent available with children
		"DAAA",
		"DAAAA", "DAAAB", "DAAABE", "DAAAC", //longer node string, but missing parent
		"EAAAA", "EAAAAA", "EAAAAB", "EAAAAC", //longer node string, no missing parent
		"F", "FE",
		"FEG",
		"FEH", "FEHI", "FEHJ",
		"FF" //multiple missing parents
	})
	{
		std::random_shuffle(m_testStrings.begin(), m_testStrings.end());
	}
	virtual void setUp() {}
	virtual void tearDown() {}
	void test() {
		for(const std::string & str : m_testStrings) {
			m_ht.insert(str);
		}
		m_ht.finalize();
		for(auto x : m_ht) {
			std::cout << x.first << std::endl;
		}
		{
			CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t) m_checkStrings.size(), m_ht.size());
			MyT::const_iterator tIt(m_ht.cbegin()), tEnd(m_ht.cend());
			std::vector<std::string>::const_iterator cIt(m_checkStrings.cbegin()), cEnd(m_checkStrings.cend());
			for(; tIt != tEnd; ++tIt, ++cIt) {
				sserialize::detail::HashBasedFlatTrie::StaticString tStr(tIt->first);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("node string", *cIt, std::string(tStr.begin(), tStr.end()));
			}
		}
		sserialize::UByteArrayAdapter tmp(sserialize::UByteArrayAdapter::createCache(1, false));
		m_ht.append(tmp);
	}
};

int main(int /*argc*/, const char ** argv) {
	srand( 0 );
	inFileName = argv[1];
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestHashBasedFlatTrie::suite() );
	runner.eventManager().popProtector();
	runner.run();
	return 0;
}