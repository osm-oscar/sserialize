#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <staging/containers/FlatGST.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"

using namespace sserialize;

template<bool T_SUFFIX_TRIE, bool TCASE_SENSITIVE>
class FlatGSTCompleterTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( FlatGSTCompleterTest );
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
	StringCompleter m_strCompleter;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }
protected:
	bool setUpCompleter() {
		FlatGST * p = new FlatGST();
		p->setCaseSensitivity(TCASE_SENSITIVE);
		p->setSuffixTrie(T_SUFFIX_TRIE);
// 		p->create();
		stringCompleter() = StringCompleter(p);
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		if (TCASE_SENSITIVE) {
			return (StringCompleter::SupportedQuerries) (StringCompleter::SQ_EPSP | StringCompleter::SQ_CASE_SENSITIVE);
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
	virtual void tearDown() {}
	
	void testStringCompleterPrivateCast() {
		FlatGST * p = dynamic_cast<FlatGST*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( p != 0 );
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( FlatGSTCompleterTest<false, false>::suite() );
	runner.addTest( FlatGSTCompleterTest<true, false>::suite() );
	runner.addTest( FlatGSTCompleterTest<false, true>::suite() );
	runner.addTest( FlatGSTCompleterTest<true, true>::suite() );
	return 0;
}
