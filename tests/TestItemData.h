#ifndef SSERIALIZE_TESTS_TEST_ITEM_DATA_H
#define SSERIALIZE_TESTS_TEST_ITEM_DATA_H

#include <deque>
#include <set>
#include <string>
#include <sserialize/search/StringCompleterPrivate.h>
#include "Static/StringsItemDB.h"
#include <sserialize/containers/ItemSet.h>
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/spatial/GeoWay.h>

namespace sserialize {

struct TestItemData {
	TestItemData() : id(0) {}
	TestItemData(UByteArrayAdapter data) : id(data.getUint32(0)), geoId(data.getUint32(4)) {}
	~TestItemData() {}
	std::deque<std::string> strs;
	std::vector<spatial::GeoPoint> points;
	uint32_t id;
	uint32_t geoId;
	/** returns true, if at least one item string matches str with teh given match type **/
	bool matchesOneString(std::string str, sserialize::StringCompleter::QuerryType type) const;
	std::ostream& dump(std::ostream& out);
};

typedef sserialize::Static::StringsItemDB<TestItemData> TestItemDataDB;
typedef typename TestItemDataDB::Item TestItemDataDBItem;
typedef ItemSet< TestItemDataDBItem, TestItemDataDB > TestItemDataItemSet;

std::deque<TestItemData> createSampleData();
std::deque< std::deque< std::string > > createSampleCompletionStrings();
std::deque< std::string > createSampleSingleCompletionStrings();

std::deque<TestItemData> getElementsWithString(const std::string & str, sserialize::StringCompleter::QuerryType matchType, const std::deque<TestItemData> & items);
std::set<unsigned int> getItemIdsWithString(const std::string & str, sserialize::StringCompleter::QuerryType matchType, const std::deque<TestItemData> & items);
std::set<unsigned int> getItemIdsWithString(const std::deque<std::string> & strs, sserialize::StringCompleter::QuerryType matchType, const std::deque<TestItemData> & items);
std::set<unsigned int> getItemIds(const std::deque<TestItemData> & items, ItemIndex idx);

inline sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const TestItemData & data) {
	return destination << data.id << data.geoId;
}

}//end namespace

#endif