#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <staging/templated/GenericTrie.h>
#include "datacreationfuncs.h"

template<int NumberOfItems, int MaxNumberOfStringsPerItem>
class GenericTrieTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( GenericTrieTest );
CPPUNIT_TEST( testFind );
CPPUNIT_TEST_SUITE_END();
private:
	std::vector< std::deque<std::string> > m_rawItems;
	sserialize::GenericTrie<std::string, std::set<uint32_t> > m_trie;
public:
	virtual void setUp() {
		for(int i = 0; i < NumberOfItems; ++i) {
			m_rawItems.push_back( sserialize::createStrings(32, rand() % MaxNumberOfStringsPerItem) );
		}
		for(int i = 0; i < NumberOfItems; ++i) {
			m_trie.at(m_rawItems[i].begin(), m_rawItems[i].end());
		}
	}
	virtual void tearDown() {}
	
	void testFind() {
		for(int i = 0; i < NumberOfItems; ++i) {
			CPPUNIT_ASSERT_MESSAGE("Find path to item", (m_trie.count(m_rawItems[i].begin(), m_rawItems[i].end(), false) > 0));
			CPPUNIT_ASSERT_MESSAGE("Find correct end node of item", (m_trie.at(m_rawItems[i].begin(), m_rawItems[i].end()).count(i) > 0));
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  GenericTrieTest<100, 5>::suite() );
	runner.run();
	return 0;
}