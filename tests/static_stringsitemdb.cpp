#include <iostream>
#include "containers/StringsItemDB.h"
#include "Static/StringsItemDB.h"
#include "TestItemData.h"
#include "TestBase.h"

using namespace sserialize;


class StaticStringsItemDBTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( StaticStringsItemDBTest );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST( testStrings );
CPPUNIT_TEST( testPayload );
CPPUNIT_TEST_SUITE_END();
public:
	typedef StringsItemDB<TestItemData> MyDBType;
	typedef typename StringsItemDB< TestItemData >::Item MyDBItemType;
	
	typedef Static::StringsItemDB<TestItemData> MyStaticDBType;
	typedef typename Static::StringsItemDB<TestItemData>::Item MyStaticDBItemType;
private:
	MyDBType m_db;
	std::vector<uint8_t> m_sdbData;
	MyStaticDBType m_sdb;
public:
	virtual void setUp() {
		std::deque<TestItemData> data = createSampleData();
		for(std::deque<TestItemData>::const_iterator it = data.begin(); it != data.end(); ++it) {
			m_db.insert(it->strs, *it);
		}
		UByteArrayAdapter stableAdap(new std::vector<uint8_t>(), true);
		UByteArrayAdapter sdbAdap(&m_sdbData);
		m_db.createStaticStringTable(stableAdap);
		sdbAdap << m_db;
		
		m_sdb = MyStaticDBType(sdbAdap, stableAdap);

	}
	virtual void tearDown() {}
	
	void testSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("DB sizes don't match", static_cast<uint32_t>( m_db.size() ), m_sdb.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Reported sizeInBytes don't match", static_cast<sserialize::UByteArrayAdapter::SizeType>( m_sdbData.size() ), m_sdb.getSizeInBytes());
	}
	
	void testStrings() {
		std::stringstream ss;
		CPPUNIT_ASSERT_EQUAL(static_cast<uint32_t>( m_db.size() ), m_sdb.size());
		for(uint32_t i = 0; i < m_db.size(); i++) {
			MyDBItemType item = m_db.at(i);
			MyStaticDBItemType sitem = m_sdb.at(i);
			ss << "Testing Item" << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": string-count failed", item.strCount(), sitem.strCount());
			uint32_t stringCount = item.strCount();
			for(uint32_t j = 0; j < stringCount; j++) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": string-equality failed", item.strAt(j), sitem.strAt(j));
			}
		}
	}
	
	void testPayload() {
		std::stringstream ss;
		CPPUNIT_ASSERT_EQUAL(static_cast<uint32_t>( m_db.size() ), m_sdb.size());
		for(uint32_t i = 0; i < m_db.size(); i++) {
			MyDBItemType item = m_db.at(i);
			MyStaticDBItemType sitem = m_sdb.at(i);
			ss << "Testing Item" << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str() + ": Payload missmatch", m_db.items().at(i).id , sitem.data().id);
		}
	}
};


int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticStringsItemDBTest::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}