#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sserialize/Static/StringTable.h>
#include "Static/StringsItemDB.h"
#include "containers/StringsItemDB.h"


using namespace std;
using namespace sserialize;

template<typename A, typename B>
std::ostream&
printDebugShouldIs(std::string what, A a, B b, std::ostream & out = std::cout) {
	out << what << ", Should: " << a << "; is: " << b << std::endl;
	return out;
}

template<typename A, typename B>
bool checkAndDebugPrint(std::string what, A a, B b, std::ostream & out = std::cout) {
	if (a != b) {
		printDebugShouldIs(what, a, b, out);
		return false;
	}
	return true;
}

struct DataItem {
	DataItem() : id(0) {}
	DataItem(UByteArrayAdapter data) {
		if (data.size() > 0) {
			id = data.getUint32(0);
		}
	}
	~DataItem() {}
	std::deque<std::string> strs;
	uint32_t id;
};

UByteArrayAdapter & operator<<(UByteArrayAdapter & destination, const DataItem & item) {
	return destination << item.id;
}

DataItem createItem(const std::deque<std::string> & possibleStrings, uint8_t stringCount, uint32_t id) {
	DataItem tdata;
	tdata.id = id;
	uint32_t rndnum;
	for(uint8_t i = 0; i < stringCount; i++) {
		rndnum = (double)rand()/RAND_MAX * possibleStrings.size(); 
		tdata.strs.push_back(possibleStrings.at(rndnum));
	}
	return tdata;
}

bool createDataBase(uint32_t itemCount, uint8_t maxStrCount, const std::deque<std::string> possibleStrings,StringsItemDB<DataItem> & database) {
	uint32_t rndnum;
	for(uint32_t i = 0; i < itemCount; i++) {
		rndnum = (double)rand()/RAND_MAX * maxStrCount;
		DataItem item = createItem(possibleStrings, rndnum, i);
		database.insert(item.strs, item);
	}
	return true;
}

bool testDataBase(const StringsItemDB<DataItem> & database, UByteArrayAdapter dbAdap, UByteArrayAdapter stableAdap) {
	Static::StringTable stable(stableAdap);
	sserialize::Static::StringsItemDB<DataItem> stDB(dbAdap,  stable);
	if (!checkAndDebugPrint("ERROR: Database has wrong size", database.size(), stDB.size()) ) {
		return false;
	}
	if (!checkAndDebugPrint("ERROR: Database has wrong getSizeInBytes", dbAdap.size(), stDB.getSizeInBytes()))
		return false;
	
	for(uint32_t i = 0; i < database.size(); i++) {
		typename sserialize::Static::StringsItemDB<DataItem>::Item sitem = stDB.at(i);
		DataItem sitemPayload = sitem.data();
		if (sitemPayload.id != database.items()[i].id) {
			std::cout << "ERROR: Payload is different" << std::endl;
			return false;
		}
		if (sitem.strCount() != database.itemStrings()[i].size()) {
			std::cout << "ERROR: String count is different" << std::endl;
			return false;
		} 
		for(uint32_t j = 0; j < database.itemStrings()[i].size(); j++) {
			if (sitem.strAt(j) != database.strIdToStr().at(database.itemStrings()[i][j]) ) {
				std::cout << "ERROR: String is different" << std::endl;
				return false;
			}
		}
	}
	return true;
}

int main(int argc, char **argv) {
	srand(0);
	std::string inFileName = "";
	if (argc > 1) {
		inFileName = std::string(argv[1]);
	}
	else {
		cout << "Arguments given: " << endl;
		for (int i=0; i < argc; i++) {
			cout << argv[i];
		}
		cout << endl << "Need in filename" << endl;
		return 1;
	}

	ifstream file;
	file.open(inFileName.c_str());
	if (!file.is_open()) {
		std::cout <<  "Failed to open file: " << inFileName << std::endl;
		return 1;
	}
	
	std::set<std::string> stringSet;
	std::string tempstr;
	while(!file.eof()) {
		std::getline(file, tempstr, ' ');
		stringSet.insert(tempstr);
	}
	std::deque<std::string> strings(stringSet.begin(), stringSet.end());
	stringSet.clear();

	for(unsigned int itemCount = 100; itemCount < 10000000; itemCount*=10) {
		std::cout << "Creating database with " << itemCount << " items." << std::endl;
		StringsItemDB<DataItem> dataBase;

		if (createDataBase(itemCount, 10, strings, dataBase)) {
			std::cout << "Creation successfull" << std::endl;
		}
		else {
			std::cout << "Creation Failed!" << std::endl;
			continue;
		}

		UByteArrayAdapter dbAdap(new std::deque<uint8_t>, true);
		UByteArrayAdapter stableAdap(new std::deque<uint8_t>, true);
		dbAdap << dataBase;
		Static::StringTable::create(stableAdap, dataBase.strIdToStr());

		if (dbAdap.size() > 0) {
			std::cout << "Serialization successfull?" << std::endl;
		}
		else {
			std::cout << "Serialization failed!" << std::endl;
		}

		if (testDataBase(dataBase, dbAdap, stableAdap)) {
			std::cout << "Testing successfull" << std::endl;
		}
		else {
			std::cout << "Testing Failed!" << std::endl;
			continue;
		}
	}

	return 0;
}