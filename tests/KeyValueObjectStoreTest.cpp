#include <iostream>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/containers/KeyValueObjectStore.h>
#include <sserialize/Static/KeyValueObjectStore.h>
#include <sserialize/utility/log.h>
#include <algorithm>
#define EPS 0.000025


using namespace sserialize;

std::vector<std::string> testStrings;

std::string getRandomString() {
	uint32_t pos = ((double)rand())/RAND_MAX*testStrings.size();
	return testStrings.at(pos);
}

template<int T_ITEM_COUNT, int T_ITEM_STR_COUNT>
class KeyValueObjectStoreTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( KeyValueObjectStoreTest );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST( testReorder );
CPPUNIT_TEST( testSort );
CPPUNIT_TEST( testStaticSize );
CPPUNIT_TEST( testStaticEquality );
CPPUNIT_TEST( testStaticFindKey );
CPPUNIT_TEST( testStaticFindValue );
CPPUNIT_TEST_SUITE_END();
private:
	typedef std::pair<std::string, std::string> KeyValuePair;
	typedef std::vector<KeyValuePair> SourceItem;
	std::vector<SourceItem> m_items;
	KeyValueObjectStore m_kv;
	UByteArrayAdapter m_skvData;
	Static::KeyValueObjectStore m_skv;
private:
	bool equal(const KeyValueObjectStore::Item & item, const SourceItem & srcItem) {
		if (item.size() != srcItem.size())
			return false;
		for(uint32_t i = 0, s = srcItem.size(); i < s; ++i) {
			if (item.at(i) != srcItem[i])
				return false;
		}
		return true;
	}
	
	void sortKeyValues(std::vector<SourceItem> & a) {
		for(uint32_t i = 0; i < a.size(); ++i) {
			std::sort(a[i].begin(), a[i].end());
		}
	}
	
	void serialize() {
		m_kv.sort();
		m_kv.serialize(m_skvData);
		m_skv = Static::KeyValueObjectStore(m_skvData);
	}
	
public:
	virtual void setUp() {
		for(int i = 0; i < T_ITEM_COUNT; ++i) {
			m_items.push_back(SourceItem());
			SourceItem & sitem = m_items.back();
			sitem.push_back(KeyValuePair("0id", sserialize::toString(i)));
			for(int j = 0; j < T_ITEM_STR_COUNT; ++j) {
				sitem.push_back( KeyValuePair(getRandomString(), getRandomString()) );
			}
		}
		
		for(uint32_t i = 0; i < m_items.size(); ++i) {
			m_kv.push_back(m_items[i]);
		}
		
		m_skvData = UByteArrayAdapter(new std::vector<uint8_t>(), true);
	}
	
	virtual void tearDown() {}

	void testSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size of KeyValueObjectStore does not match", static_cast<uint32_t>( m_items.size() ), m_kv.size());
	}
	
	void testEquality() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_items.size() ), m_kv.size());
		for(uint32_t i = 0; i < m_items.size(); ++i) {
			SourceItem & item = m_items[i];
			KeyValueObjectStore::Item kvitem = m_kv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("size of item", i, "does not match"), static_cast<uint32_t>( item.size() ), kvitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].key[", j, "] does not match"), item[j].first, kvitem.at(j).first);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].value[", j, "] does not match"), item[j].second, kvitem.at(j).second);
			}
		}
	}
	
	void testReorder() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_items.size() ), m_kv.size());
		
		std::vector<uint32_t> perm = range< std::vector<uint32_t>, uint32_t >(0, m_items.size(), 1);
		std::random_shuffle(perm.begin(), perm.end());
		
		m_kv.reorder(perm);
		
		for(uint32_t i = 0; i < m_items.size(); ++i) {
			SourceItem & item = m_items[perm[i]];
			KeyValueObjectStore::Item kvitem = m_kv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("size of item", i, "does not match"), static_cast<uint32_t>( item.size() ), kvitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].key[", j, "] does not match"), item[j].first, kvitem.at(j).first);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].value[", j, "] does not match"), item[j].second, kvitem.at(j).second);
			}
		}
	}
	
	void testSort() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_items.size() ), m_kv.size());
		
		m_kv.sort();
		
		sortKeyValues(m_items);
		
		for(uint32_t i = 0; i < m_items.size(); ++i) {
			SourceItem & item = m_items[i];
			KeyValueObjectStore::Item kvitem = m_kv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("size of item", i, "does not match"), static_cast<uint32_t>( item.size() ), kvitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].key[", j, "] does not match"), item[j].first, kvitem.at(j).first);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].value[", j, "] does not match"), item[j].second, kvitem.at(j).second);
			}
		}
	}
	
	void testStaticSize() {
		serialize();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size of Static::KeyValueObjectStore does not match", static_cast<uint32_t>( m_items.size() ), m_skv.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Reported sizeInBytes of Static::KeyValueObjectStore does not match", static_cast<UByteArrayAdapter::OffsetType>( m_skvData.size() ), m_skv.getSizeInBytes());
	}
	
	void testStaticEquality() {
		serialize();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_items.size() ), m_skv.size());
		for(uint32_t i = 0; i < m_items.size(); ++i) {
			KeyValueObjectStore::Item item = m_kv.at(i);
			Static::KeyValueObjectStoreItem sitem = m_skv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("item size does not match", static_cast<uint32_t>( item.size() ), sitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].key[", j, "] does not match"), item.at(j).first, sitem.key(j));
			}
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].value[", j, "] does not match"), item.at(j).second, sitem.value(j));
			}
		}
	}
	
	void testStaticFindKey() {
		serialize();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_items.size() ), m_skv.size());
		for(uint32_t i = 0; i < m_items.size(); ++i) {
			KeyValueObjectStore::Item item = m_kv.at(i);
			Static::KeyValueObjectStoreItem sitem = m_skv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("item size does not match", static_cast<uint32_t>( item.size() ), sitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].value[", j, "] does not match"), j, sitem.findKey(item.at(j).first, j));
			}
		}
	}
	
	void testStaticFindValue() {
		serialize();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_items.size() ), m_skv.size());
		for(uint32_t i = 0; i < m_items.size(); ++i) {
			KeyValueObjectStore::Item item = m_kv.at(i);
			Static::KeyValueObjectStoreItem sitem = m_skv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("item size does not match", static_cast<uint32_t>( item.size() ), sitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item ", i, " pos=", j, "does not match"), j, sitem.findValue(item.at(j).second, j));
			}
		}
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