#include <iostream>
#include <fstream>
#include <sserialize/Static/StringTable.h>

using namespace std;
using namespace sserialize;

int main(int argc, char **argv) {
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
	
	std::map<uint32_t, std::string> strings;
	std::string tempstr;
	uint32_t count = 0;
	while(!file.eof()) {
		std::getline(file, tempstr, ' ');
		strings.insert(std::pair<uint32_t, std::string>(count, tempstr));
		count++;
	}
	std::cout << "Read file. Testing table" << std::endl;

	UByteArrayAdapter stableDataAdapter(new std::deque<uint8_t>(), true);
	if (Static::StringTable::create(stableDataAdapter, strings) ) {
		std::cout << "Created string table with size:  " << stableDataAdapter.size() << std::endl;
	}
	else {
		std::cout << "Failed to create string table" <<  std::endl;
		return 1;
	}


	Static::StringTable stable(stableDataAdapter);
	if (stable.size() != strings.size()) {
		std::cout << "Static::StringTable.count() is invalid. Should: " << strings.size() << " is: " << stable.size() << std::endl;
	}
	for(size_t i = 0; i < strings.size(); i++) {
		if (strings.at(i) != stable.at(i)) {
			std::cout << "Strings at position " << i << " are unequal. Should: " << strings.at(i) << ";; is: " << stable.at(i) << std::endl;
			return 1; 
		}
	}
	std::cout << "Passed all tests." << std::endl;

	return 0;
}