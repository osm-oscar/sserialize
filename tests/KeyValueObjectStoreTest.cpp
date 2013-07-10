#include <iostream>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/containers/KeyValueObjectStore.h>
#include <sserialize/Static/KeyValueObjectStore.h>
#define EPS 0.000025


using namespace sserialize;

std::vector<std::string> testStrings;

std::string getRandomString() {
	uint32_t pos = rand()/RAND_MAX*testStrings.size();
	return testStrings.at(pos);
}

template<int T_ITEM_COUNT, int T_ITEM_STR_COUNT>
class KeyValueObjectStoreTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( KeyValueObjectStoreTest );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST( testKeyValues );
CPPUNIT_TEST_SUITE_END();
private:
	typedef std::pair<std::string, std::string> KeyValuePair;
	typedef std::vector<KeyValuePair> SourceItem;
	std::vector<SourceItem> m_items;
	UByteArrayAdapter m_skvData;
	Static::KeyValueObjectStore m_skv;
public:
	virtual void setUp() {
		for(int i = 0; i < T_ITEM_COUNT; ++i) {
			m_items.push_back(SourceItem());
			for(int j = 0; j < T_ITEM_STR_COUNT; ++j) {
				m_items.back().push_back( KeyValuePair(getRandomString(), getRandomString()) );
			}
		}
		
		sserialize::KeyValueObjectStore kvo;
		
		for(uint32_t i = 0; i < m_items.size(); ++i) {
			kvo.add(m_items[i]);
		}
		
		m_skvData = UByteArrayAdapter(new std::vector<uint8_t>(), true);
		kvo.serialize(m_skvData);
		m_skv = Static::KeyValueObjectStore(m_skvData);
	}
	
	virtual void tearDown() {}
	
	void testKeyValues() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_items.size() ), m_skv.size());
		for(uint32_t i = 0; i < m_items.size(); ++i) {
			SourceItem & item = m_items[i];
			Static::KeyValueObjectStoreItem sitem = m_skv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("item size does not match", static_cast<uint32_t>( item.size() ), sitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE("item key does not match", item[j].first, sitem.key(j));
				CPPUNIT_ASSERT_EQUAL_MESSAGE("item key does not match", item[j].second, sitem.value(j));
			}
		}
	}
	
	void testSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_items.size() ), m_skv.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Reported sizeInBytes don't match", static_cast<UByteArrayAdapter::OffsetType>( m_skvData.size() ), m_skv.getSizeInBytes());
	}
};


int main(int argc, char ** argv) {
	if (argc < 2) {
		std::cout << "Need string source file" << std::endl;
		return -1;
	}
	std::ifstream inFile;
	inFile.open(argv[1]);
	if (!inFile.is_open()) {
		std::cout << "Could not open source string file" << std::endl;
	}
	
	std::string line;
	while(!inFile.eof()) {
		std::getline(inFile, line);
		testStrings.push_back(line);
	}

	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( KeyValueObjectStoreTest<100, 10>::suite() );
	runner.run();
	return 0;
}