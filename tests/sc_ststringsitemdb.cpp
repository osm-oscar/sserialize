#include "containers/StringsItemDB.h"
#include "containers/StringCompleterPrivateStaticDB.h"
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"

using namespace sserialize;

class StaticStringsItemDBTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( StaticStringsItemDBTest );
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
	typedef sserialize::StringCompleterPrivateStaticDB< Static::StringsItemDB<TestItemData> > StringCompleterPrivateType;
	std::map<unsigned int, unsigned int> m_dbIdRealIdMap;
	StringCompleter m_strCompleter;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }
protected:
	bool setUpCompleter() {
		StringsItemDB<TestItemData> db;
		for(size_t i = 0; i < items().size(); i++) {
			m_dbIdRealIdMap[ db.insert(items()[i].strs, items()[i]) ] = items()[i].id;
		}
		
		UByteArrayAdapter stableAdap(new std::deque<uint8_t>(), true);
		UByteArrayAdapter sdbAdap(new std::deque<uint8_t>(), true);
		db.createStaticStringTable(stableAdap);
		sdbAdap << db;
		
		Static::StringsItemDB<TestItemData> sdb(sdbAdap, stableAdap);
		
		StringCompleterPrivateType * p = new StringCompleterPrivateType(sdb);
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
		StringCompleterPrivateType * p = dynamic_cast<StringCompleterPrivateType*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( p != 0 );
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticStringsItemDBTest::suite() );
	runner.run();
	return 0;
}
