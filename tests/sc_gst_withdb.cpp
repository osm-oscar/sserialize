#include <sserialize/containers//GeneralizedTrie.h>
#include <sserialize/containers/StringsItemDBWrapperPrivateSSIDB.h>
#include <sserialize/completers/StringCompleterPrivateGST.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"

using namespace sserialize;

template<bool T_TREE_CASE_SENSITIVE>
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
CPPUNIT_TEST_SUITE_END();
private:
	typedef GeneralizedTrie::MultiPassTrie TrieImp;
	typedef StringCompleterPrivateGST< TrieImp::MyBaseClass::ItemSetContainer > StringCompleterPrivateType;
	TrieImp * m_trie;
	StringCompleter m_strCompleter;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }
protected:
	bool setUpCompleter() {
		
		StringsItemDB<TestItemData> db;
		for(size_t i = 0; i < items().size(); i++) {
			db.insert(items()[i].strs, items()[i]);
		}
		m_trie = new GeneralizedTrie::MultiPassTrie();
		
		m_trie->setCaseSensitivity(T_TREE_CASE_SENSITIVE);
		m_trie->setSuffixTrie(true);
		
		m_trie->setDB( StringsItemDBWrapper<TestItemData>(new StringsItemDBWrapperPrivateSIDB<TestItemData>(db) ));
		
// 						StringsItemDBWrapper<TestItemData>(new StringsItemDBWrapperPrivateSIDB<TestItemData>(db)),
// 						T_TREE_CASE_SENSITIVE,
// 						true);
		
		StringCompleterPrivateType * p = new StringCompleterPrivateType(m_trie);
		stringCompleter() = StringCompleter( p );
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		if (T_TREE_CASE_SENSITIVE) {
			return StringCompleter::SQ_ALL; //for the settings above
		}
		else {
			return (StringCompleter::SupportedQuerries) (StringCompleter::SQ_EPSP | StringCompleter::SQ_CASE_INSENSITIVE);
		}
	}

	virtual bool createStringCompleter() {
		return true;
	}
	
public:
	virtual void setUp() {
		setUpCompleter();
	}
	virtual void tearDown() {
		delete m_trie;
	}
	
	void testStringCompleterPrivateCast() {
		StringCompleterPrivateType * p = dynamic_cast<StringCompleterPrivateType*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( p != 0 );
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( GeneralizedTrieTest<false>::suite() );
	runner.addTest( GeneralizedTrieTest<true>::suite() );
	runner.run();
	return 0;
}
