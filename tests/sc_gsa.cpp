#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/templated/StringsItemDB.h>
#include <sserialize/containers/FlatGST.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"

using namespace sserialize;

template<bool T_SUFFIX_TRIE, bool TCASE_SENSITIVE, bool T_WITH_STRING_IDS>
class StringsGSACompleterTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( StringsGSACompleterTest );
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
	StringsItemDBWrapper<TestItemData> m_db;
	std::map<unsigned int, unsigned int> m_dbIdRealIdMap;
	StringCompleter m_strCompleter;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }
protected:
	bool setUpCompleter() {
		for(size_t i = 0; i < items().size(); i++) {
			m_dbIdRealIdMap[ m_db.insert(items()[i].strs, items()[i]) ] = items()[i].id;
		}
		
		dynamic::FlatGST<TestItemData> * p = new dynamic::FlatGST<TestItemData>(m_db, T_SUFFIX_TRIE, TCASE_SENSITIVE, T_WITH_STRING_IDS);
		p->create();
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
		dynamic::FlatGST<TestItemData> * p = dynamic_cast<dynamic::FlatGST<TestItemData>*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( p != 0 );
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StringsGSACompleterTest<false, false, false>::suite() );
	runner.addTest( StringsGSACompleterTest<true, false, false>::suite() );
	runner.addTest( StringsGSACompleterTest<false, true, false>::suite() );
	runner.addTest( StringsGSACompleterTest<true, true, false>::suite() );
	runner.addTest( StringsGSACompleterTest<false, false, true>::suite() );
	runner.addTest( StringsGSACompleterTest<true, false, true>::suite() );
	runner.addTest( StringsGSACompleterTest<false, true, true>::suite() );
	runner.addTest( StringsGSACompleterTest<true, true, true>::suite() );
	runner.run();
	return 0;
}
