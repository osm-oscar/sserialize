#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include "containers/StringsItemDB.h"
#include "containers/StringCompleterPrivateItemDB.h"
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"

using namespace sserialize;

class StringsItemDBTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( StringsItemDBTest );
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
	StringsItemDB<TestItemData> m_db;
	std::map<unsigned int, unsigned int> m_dbIdRealIdMap;
	StringCompleter m_strCompleter;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }
protected:
	bool setUpCompleter() {
		for(size_t i = 0; i < items().size(); i++) {
			m_dbIdRealIdMap[ m_db.insert(items()[i].strs, items()[i]) ] = items()[i].id;
		}
		
		StringCompleterPrivateItemDB<TestItemData> * p = new StringCompleterPrivateItemDB<TestItemData>(&m_db);
		stringCompleter() = StringCompleter( p );
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		return StringCompleter::SQ_ALL;
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
		StringCompleterPrivateItemDB<TestItemData> * p = dynamic_cast<StringCompleterPrivateItemDB<TestItemData>*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( p != 0 );
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StringsItemDBTest::suite() );
// 	runner.eventManager().popProtector();
	runner.run();
	return 0;
}
