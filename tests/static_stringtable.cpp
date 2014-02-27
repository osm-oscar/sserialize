#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/Static/StringTable.h>
#include <sserialize/utility/log.h>
#include "datacreationfuncs.h"

using namespace sserialize;

uint32_t strTableSize = 1024;

class TestStringTable: public CppUnit::TestFixture {
private:
	std::vector<std::string> m_strs;
public:
	std::vector<std::string> & strs() { return m_strs; }
	virtual const sserialize::Static::StringTable & stable() = 0;
	virtual const sserialize::UByteArrayAdapter & stableData() = 0;
public:
	virtual void setUp() {}
	virtual void tearDown() {}

	void testEq() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", stableData().size(), stable().getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)strs().size(), stable().size());
		for(uint32_t i = 0, s = strs().size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("string at", i), stable().at(i), strs().at(i));
		}
	}
};

class TestSortedStringTable: public TestStringTable {
CPPUNIT_TEST_SUITE( TestSortedStringTable );
CPPUNIT_TEST( testEq );
CPPUNIT_TEST( testFind );
CPPUNIT_TEST_SUITE_END();
private:
	sserialize::UByteArrayAdapter m_stableData;
	sserialize::Static::SortedStringTable m_stable;
public:
	virtual const sserialize::Static::StringTable & stable() { return m_stable; }
	virtual const sserialize::UByteArrayAdapter & stableData() { return m_stableData; }
public:
	virtual void setUp() {
		std::set<std::string> myStrs;
		for(uint32_t i = 0; i < strTableSize; ++i) {
			myStrs.insert(createString(100));
		}
		strs().insert(strs().end(), myStrs.cbegin(), myStrs.cend());
		m_stableData = sserialize::UByteArrayAdapter(new std::vector<uint8_t>(), true);
		sserialize::Static::SortedStringTable::create(strs().cbegin(), strs().cend(), m_stableData);
	}
	
	void testFind() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", stableData().size(), stable().getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)strs().size(), stable().size());
		for(uint32_t i = 0, s = strs().size(); i < s; ++i) {
			int32_t pos = m_stable.find(strs().at(i));
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("string at", i), i, (uint32_t) pos);
		}
	}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( TestSortedStringTable::suite() );
	bool ok = runner.run();
	return (ok ? 0 : 1);
}