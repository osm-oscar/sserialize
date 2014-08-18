#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestResult.h>
#include <sserialize/containers/HashBasedFlatTrie.h>

const char * inFileName = 0;

class TestHashBasedFlatTrie: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( TestHashBasedFlatTrie );
CPPUNIT_TEST( testFlatCorrect );
CPPUNIT_TEST( testNode );
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
	
	virtual void setUp() {
		for(const std::string & str : m_testStrings) {
			m_ht.insert(str);
		}
		m_ht.finalize();
	}
	
	virtual void tearDown() {}
	
	void testFlatCorrect() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t) m_checkStrings.size(), m_ht.size());
		MyT::const_iterator tIt(m_ht.cbegin()), tEnd(m_ht.cend());
		std::vector<std::string>::const_iterator cIt(m_checkStrings.cbegin()), cEnd(m_checkStrings.cend());
		for(; tIt != tEnd; ++tIt, ++cIt) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("node string", *cIt, m_ht.toStr(tIt->first));
		}
	}
	
	//walks the trie in-order and checks for correctness of the Node implementation
	void testNode() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t) m_checkStrings.size(), m_ht.size());
		std::vector< std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator> > nodeIts;
		uint32_t count = 0;
		MyT::Node node = m_ht.root();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("root node", m_checkStrings[count], m_ht.toStr(node.str()));
		++count;
		nodeIts.push_back( std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator>(node.begin(), node.end()) );
		while(nodeIts.size()) {
			//descend upwards if need be
			if (nodeIts.back().first == nodeIts.back().second) {
				while (nodeIts.size() && nodeIts.back().first == nodeIts.back().second) {
					nodeIts.pop_back();
				}
				continue;
			}
			MyT::Node::const_iterator & nIt = nodeIts.back().first;
			node = *nIt;
			++nIt;
			//insert children
			nodeIts.push_back( std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator>(node.begin(), node.end()) );
			//check the node
			CPPUNIT_ASSERT_EQUAL_MESSAGE("node string", m_checkStrings[count], m_ht.toStr(node.str()));
			++count;
		}
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