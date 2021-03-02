#include <iostream>
#include <sserialize/containers/KeyValueObjectStore.h>
#include <sserialize/Static/KeyValueObjectStore.h>
#include <sserialize/utility/log.h>
#include <sserialize/iterator/RangeGenerator.h>
#include <algorithm>
#include "TestBase.h"

#define EPS 0.000025


using namespace sserialize;

std::vector<std::string> testStrings;

std::string getRandomString() {
	uint32_t pos = ((double)rand())/RAND_MAX*testStrings.size();
	return testStrings.at(pos);
}

template<int T_ITEM_COUNT, int T_ITEM_STR_COUNT>
class KeyValueObjectStoreTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( KeyValueObjectStoreTest );
CPPUNIT_TEST( testSize );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST( testReorder );
CPPUNIT_TEST( testSort );
CPPUNIT_TEST( testStaticSize );
CPPUNIT_TEST( testStaticEquality );
CPPUNIT_TEST( testStaticFindKey );
CPPUNIT_TEST( testStaticFindValue );
CPPUNIT_TEST( testItemEquality );
CPPUNIT_TEST_SUITE_END();
private:
	typedef std::pair<std::string, std::string> KeyValuePair;
	typedef std::vector<KeyValuePair> SourceItem;
	struct Data {
		std::vector<SourceItem> items;
		KeyValueObjectStore kv;
		UByteArrayAdapter skvData;
		Static::KeyValueObjectStore skv;
	};
	std::unique_ptr<Data> m_d;
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
		m_d->kv.sort();
		m_d->kv.serialize(m_d->skvData);
		m_d->skv = Static::KeyValueObjectStore(m_d->skvData);
	}
	
public:
	virtual void setUp() {
		m_d = std::make_unique<Data>();
		for(int i = 0; i < T_ITEM_COUNT; ++i) {
			m_d->items.push_back(SourceItem());
			SourceItem & sitem = m_d->items.back();
			sitem.push_back(KeyValuePair("0id", sserialize::toString(i)));
			for(int j = 0; j < T_ITEM_STR_COUNT; ++j) {
				sitem.push_back( KeyValuePair(getRandomString(), getRandomString()) );
			}
		}
		
		for(uint32_t i = 0; i < m_d->items.size(); ++i) {
			m_d->kv.push_back(m_d->items[i]);
		}
		
		m_d->skvData = UByteArrayAdapter(new std::vector<uint8_t>(), true);
	}
	
	virtual void tearDown() {
		m_d.reset(0);
	}

	void testSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size of KeyValueObjectStore does not match", static_cast<uint32_t>( m_d->items.size() ), (uint32_t)m_d->kv.size());
	}
	
	void testEquality() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_d->items.size() ), (uint32_t)m_d->kv.size());
		for(uint32_t i = 0; i < m_d->items.size(); ++i) {
			SourceItem & item = m_d->items[i];
			KeyValueObjectStore::Item kvitem = m_d->kv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("size of item", i, "does not match"), static_cast<uint32_t>( item.size() ), (uint32_t)kvitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].key[", j, "] does not match"), item[j].first, kvitem.at(j).first);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].value[", j, "] does not match"), item[j].second, kvitem.at(j).second);
			}
		}
	}
	
	void testReorder() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_d->items.size() ), (uint32_t)m_d->kv.size());
		
		std::vector<uint32_t> perm(sserialize::RangeGenerator<uint32_t>::begin(0, (uint32_t) m_d->items.size()), sserialize::RangeGenerator<uint32_t>::end(0, (uint32_t) m_d->items.size()));
		std::random_shuffle(perm.begin(), perm.end());
		
		m_d->kv.reorder(perm);
		
		for(uint32_t i = 0; i < m_d->items.size(); ++i) {
			SourceItem & item = m_d->items[perm[i]];
			KeyValueObjectStore::Item kvitem = m_d->kv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("size of item", i, "does not match"), static_cast<uint32_t>( item.size() ), (uint32_t)kvitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].key[", j, "] does not match"), item[j].first, kvitem.at(j).first);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].value[", j, "] does not match"), item[j].second, kvitem.at(j).second);
			}
		}
	}
	
	void testSort() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_d->items.size() ), (uint32_t)m_d->kv.size());
		
		m_d->kv.sort();
		
		sortKeyValues(m_d->items);
		
		for(uint32_t i = 0; i < m_d->items.size(); ++i) {
			SourceItem & item = m_d->items[i];
			KeyValueObjectStore::Item kvitem = m_d->kv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("size of item", i, "does not match"), static_cast<uint32_t>( item.size() ), (uint32_t)kvitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].key[", j, "] does not match"), item[j].first, kvitem.at(j).first);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].value[", j, "] does not match"), item[j].second, kvitem.at(j).second);
			}
		}
	}
	
	void testStaticSize() {
		serialize();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size of Static::KeyValueObjectStore does not match", static_cast<uint32_t>( m_d->items.size() ), m_d->skv.size());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Reported sizeInBytes of Static::KeyValueObjectStore does not match", static_cast<UByteArrayAdapter::OffsetType>( m_d->skvData.size() ), m_d->skv.getSizeInBytes());
	}
	
	void testStaticEquality() {
		serialize();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_d->items.size() ), m_d->skv.size());
		for(uint32_t i = 0; i < m_d->items.size(); ++i) {
			KeyValueObjectStore::Item item = m_d->kv.at(i);
			Static::KeyValueObjectStoreItem sitem = m_d->skv.at(i);
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
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_d->items.size() ), m_d->skv.size());
		for(uint32_t i = 0; i < m_d->items.size(); ++i) {
			KeyValueObjectStore::Item item = m_d->kv.at(i);
			Static::KeyValueObjectStoreItem sitem = m_d->skv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("item size does not match", static_cast<uint32_t>( item.size() ), sitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item[", i, "].value[", j, "] does not match"), j, sitem.findKey(item.at(j).first, j));
			}
		}
	}
	
	void testStaticFindValue() {
		serialize();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", static_cast<uint32_t>( m_d->items.size() ), m_d->skv.size());
		for(uint32_t i = 0; i < m_d->items.size(); ++i) {
			KeyValueObjectStore::Item item = m_d->kv.at(i);
			Static::KeyValueObjectStoreItem sitem = m_d->skv.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("item size does not match", static_cast<uint32_t>( item.size() ), sitem.size());
			for(uint32_t j = 0; j < item.size(); ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item ", i, " pos=", j, "does not match"), j, sitem.findValue(item.at(j).second, j));
			}
		}
	}
	
	void testItemEquality() {
		sserialize::UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		for(uint32_t startValues = 0; startValues < 1024*1024; startValues = (1+startValues)*2) {
			for(uint32_t keyId = startValues; keyId < 256*1024*1024; keyId = (1+keyId)*2) {
				KeyValueObjectStore::ItemData itemData;
				for(uint32_t valueId = startValues; valueId < 256*1024*1024; valueId = (1+valueId)*2) {
					itemData.push_back( KeyValueObjectStore::KeyValue(keyId, valueId) );
				}
				d.resetPtrs();
				KeyValueObjectStore::serialize(itemData, d);
				Static::KeyValueObjectStoreItemBase sitem(d);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("item.size()",(uint32_t)itemData.size(), sitem.size());
				for(uint32_t i = 0, s = (uint32_t) itemData.size(); i < s; ++i) {
					CPPUNIT_ASSERT_EQUAL_MESSAGE("item keyid", itemData[i].key, sitem.keyId(i));
					CPPUNIT_ASSERT_EQUAL_MESSAGE("item valueid", itemData[i].value, sitem.valueId(i));
				}
			}
		}
	}
};


int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	
	if (argc < 2) {
		std::cout << "Need string source file as first parameter" << std::endl;
		return -1;
	}
	std::ifstream inFile;
	inFile.open(argv[1]);
	if (!inFile.is_open()) {
		std::cout << "Could not open source string file. Should be first parameter" << std::endl;
	}
	
	std::string line;
	while(!inFile.eof()) {
		std::getline(inFile, line);
		testStrings.push_back(line);
	}

	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( KeyValueObjectStoreTest<100, 10>::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
