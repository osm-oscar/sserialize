#ifndef SSERIALIZE_TESTS_STRING_COMPLETER_TEST_H
#define SSERIALIZE_TESTS_STRING_COMPLETER_TEST_H
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/containers/StringsItemDBWrapper.h>
#include <sserialize/templated/StringsItemDBWrapperPrivateSIDB.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"



namespace sserialize {


class StringCompleterTest: public CppUnit::TestFixture {
private:
	std::deque<std::string> m_sampleCmpStrs;
	std::deque<TestItemData> m_items;
	StringsItemDBWrapper<TestItemData> m_db;

protected:
	
	virtual StringCompleter & stringCompleter() = 0;
	const std::deque<TestItemData> & items() { return m_items;}
	StringsItemDBWrapper<TestItemData> & db() { return m_db; }
	
	StringsItemDBWrapper<TestItemData> createDB() const {
		StringsItemDBWrapper<TestItemData> db( new StringsItemDBWrapperPrivateSIDB<TestItemData>() );
		for(size_t i = 0; i < m_items.size(); i++) {
			db.insert(m_items[i].strs, m_items[i]);
		}
		return db;
	}
	
	/** this will be run by this class. Override if necessary */
	virtual bool completionTest(unsigned int qt) {
		StringCompleter::QuerryType mqt = (StringCompleter::QuerryType) qt;
		if (stringCompleter().supportsQuerry(mqt)) {
			return testStringCompleterQuerry(m_sampleCmpStrs, stringCompleter(), m_items, mqt);
		}
		return true;
	}
	
	/** this will be run by this class. Override if necessary */
	virtual bool partialCompletionTest(unsigned int qt) {
		StringCompleter::QuerryType mqt = (StringCompleter::QuerryType) qt;
		if (stringCompleter().supportsQuerry(mqt)) {
			return testPartialStringCompleterQuerry(m_sampleCmpStrs, stringCompleter(), m_items, mqt);
		}
		return true;
	}
	
	/** This function should return the supported querries by the testet completer */
	virtual StringCompleter::SupportedQuerries supportedQuerries() = 0;
	
	/** this function is called by the testCreateStringCompleter test.
	  * derived classes should create the StringCompleter structures here */
	virtual bool createStringCompleter()  = 0;
	
public:
	StringCompleterTest() : CppUnit::TestFixture() {
		m_sampleCmpStrs = createSampleSingleCompletionStrings();
		m_items = createSampleData();
		m_db = createDB();
	}
    virtual ~StringCompleterTest() {}
	
	/** Derived classes need to set the stringCompleter via stringCompleter() */
	virtual void setUp() {}
	
	/** derived classes need to do clean-up of data-structures associated with StringCompleter */
	virtual void tearDown() {}
	
	void testCreateStringCompleter() {
		CPPUNIT_ASSERT( this->createStringCompleter() );
	}
	
	void testSupportedQuerries() {
		CPPUNIT_ASSERT_EQUAL( stringCompleter().getSupportedQuerries(), this->supportedQuerries() );
	}
	
	void testCompletionECS() {
		CPPUNIT_ASSERT( completionTest(StringCompleter::QT_EXACT) );
		CPPUNIT_ASSERT( partialCompletionTest(StringCompleter::QT_EXACT) );
	}

	void testCompletionECI() {
		CPPUNIT_ASSERT( completionTest(StringCompleter::QT_EXACT | StringCompleter::QT_CASE_INSENSITIVE) );
		CPPUNIT_ASSERT( partialCompletionTest(StringCompleter::QT_EXACT | StringCompleter::QT_CASE_INSENSITIVE) );
	}
	
	void testCompletionPCS() {
		CPPUNIT_ASSERT( completionTest(StringCompleter::QT_PREFIX) );
		CPPUNIT_ASSERT( partialCompletionTest(StringCompleter::QT_PREFIX) );
	}

	void testCompletionPCI() {
		CPPUNIT_ASSERT( completionTest(StringCompleter::QT_PREFIX | StringCompleter::QT_CASE_INSENSITIVE) );
		CPPUNIT_ASSERT( partialCompletionTest(StringCompleter::QT_PREFIX | StringCompleter::QT_CASE_INSENSITIVE) );
	}

	void testCompletionSCS() {
		CPPUNIT_ASSERT( completionTest(StringCompleter::QT_SUFFIX) );
		CPPUNIT_ASSERT( partialCompletionTest(StringCompleter::QT_SUFFIX) );
	}

	void testCompletionSCI() {
		CPPUNIT_ASSERT( completionTest(StringCompleter::QT_SUFFIX | StringCompleter::QT_CASE_INSENSITIVE) );
		CPPUNIT_ASSERT( partialCompletionTest(StringCompleter::QT_SUFFIX | StringCompleter::QT_CASE_INSENSITIVE) );
	}
	
	void testCompletionSPCS() {
		CPPUNIT_ASSERT( completionTest(StringCompleter::QT_SUFFIX_PREFIX) );
		CPPUNIT_ASSERT( partialCompletionTest(StringCompleter::QT_SUFFIX_PREFIX) );
	}

	void testCompletionSPCI() {
		CPPUNIT_ASSERT( completionTest(StringCompleter::QT_SUFFIX_PREFIX | StringCompleter::QT_CASE_INSENSITIVE) );
		CPPUNIT_ASSERT( partialCompletionTest(StringCompleter::QT_SUFFIX_PREFIX | StringCompleter::QT_CASE_INSENSITIVE) );
	}
};



}//end namespace

#endif