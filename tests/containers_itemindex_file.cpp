#include <iostream>
#include <fstream>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>

using namespace std;

int main(int argc, const char ** argv) {
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
	
	std::ifstream file;
	file.open(inFileName);
	if (!file.is_open()) {
		std::cout << "Failed to open file: " << inFileName << std::endl;
		return 1;
	}
	
	std::set<uint32_t> s;
	while (!file.eof()) {
		uint32_t num;
		file >> num;
		s.insert(num);
	}
	
	std::cout << "Test set size: " << s.size() << std::endl;
	
	{
		std::deque<uint8_t> idxData;
		sserialize::ItemIndexPrivateRegLine::create(s, idxData, -1, true);
		sserialize::UByteArrayAdapter idxAdap(&idxData);
		sserialize::ItemIndex idx(idxAdap);
		std::cout << "ItemIndexRegLine with auto bit and regline...";
		if (s != idx) {
			std::cout << "Failed!";
		}
		else {
			std::cout << "Passed! Size=" << idxData.size() << "; reported: " << idx.getSizeInBytes();
		}
		std::cout << std::endl;
	}
	
	{
		std::deque<uint8_t> idxData;
		sserialize::ItemIndexPrivateRegLine::create(s, idxData, -1, false);
		sserialize::UByteArrayAdapter idxAdap(&idxData);
		sserialize::ItemIndex idx(idxAdap);
		std::cout << "ItemIndexRegLine with auto bit without regline...";
		if (s != idx) {
			std::cout << "Failed!";
		}
		else {
			std::cout << "Passed! Size=" << idxData.size() << "; reported: " << idx.getSizeInBytes();
		}
		std::cout << std::endl;
	}
	
		{
		std::deque<uint8_t> idxData;sserialize::UByteArrayAdapter idxAdap(&idxData);
		sserialize::ItemIndexPrivateSimple::addItemIndexFromIds(s, idxAdap);
		sserialize::ItemIndex idx(sserialize::UByteArrayAdapter(&idxData), sserialize::ItemIndex::T_SIMPLE);
		std::cout << "ItemIndexSimple...";
		if (s != idx) {
			std::cout << "Failed!";
		}
		else {
			std::cout << "Passed! Size=" << idxData.size() << "; reported: " << idx.getSizeInBytes();
		}
		std::cout << std::endl;
	}
	
	return 0;
}