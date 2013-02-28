#include <sserialize/containers//GeneralizedTrie.h>
#include <sserialize/completers/StringCompleterPrivateGST.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"

using namespace sserialize;

template<bool T_TREE_CASE_SENSITIVE, bool T_SUFFIX_TREE>
class GeneralizedTrieTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( GeneralizedTrieTest );
CPPUNIT_TEST( testCreateStringCompleter );
CPPUNIT_TEST( testSupportedQuerries );
CPPUNIT_TEST( testCompletionECS );
CPPUNIT_TEST( testCompletionECI );
CPPUNIT_TEST( testCompletionPCS );
CPPUNIT_TEST( testCompletionPCI );
CPPUNIT_TEST( testCompletionSCS );
CPPUNIT_TEST( testCompletionSCI );
CPPUNIT_TEST( testCompletionSPCS );
CPPUNIT_TEST( testCompletionSPCI );
CPPUNIT_TEST( testStringCompleterPrivateCast );
CPPUNIT_TEST( testConsistency );
CPPUNIT_TEST_SUITE_END();
private:
	typedef StringCompleterPrivateGST<TestItemData> StringCompleterPrivateType;
	GeneralizedTrie<TestItemData> m_trie;
	StringCompleter m_strCompleter;
	virtual StringCompleter & stringCompleter() { return m_strCompleter; }
protected:
	bool setUpCompleter() {
		m_trie.setCaseSensitivity(T_TREE_CASE_SENSITIVE);
		m_trie.setSuffixTrie(T_SUFFIX_TREE);
	
		m_trie.setDB(createDB());
		
		StringCompleterPrivateType * p = new StringCompleterPrivateType(&m_trie);
		stringCompleter() = StringCompleter( p );
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		uint8_t sq = StringCompleter::SQ_EP | StringCompleter::SQ_CASE_INSENSITIVE;
		if (T_TREE_CASE_SENSITIVE)
			sq |= StringCompleter::SQ_CASE_SENSITIVE;
			
		if (T_SUFFIX_TREE)
			sq |= StringCompleter::SQ_SSP;
			
		return (StringCompleter::SupportedQuerries) sq;
	}

	virtual bool createStringCompleter() {
		return true;
	}
	
public:
	virtual void setUp() {
		setUpCompleter();
	}
	virtual void tearDown() {}
	
	void testStringCompleterPrivateCast() {
		StringCompleterPrivateType * p = dynamic_cast<StringCompleterPrivateType*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( p != 0 );
	}
	
	void testConsistency() {
		CPPUNIT_ASSERT( m_trie.consistencyCheck() );
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( GeneralizedTrieTest<false, false>::suite() );
	runner.addTest( GeneralizedTrieTest<true, true>::suite() );
	runner.addTest( GeneralizedTrieTest<false, true>::suite() );
	runner.addTest( GeneralizedTrieTest<true, false>::suite() );
	runner.run();
	return 0;
}
