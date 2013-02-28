#include <iostream>
#include <fstream>
#include <sserialize/Static/ItemIndexStore.h>

using namespace std;
using namespace sserialize;

void dumpIndex(std::ostream & out, const ItemIndex & idx) {
	for(uint32_t i = 0; i < idx.size(); i++) {
		out << idx.at(i) << std::endl;
	}
}

void dumpIndex(std::ostream & out, const Static::SortedOffsetIndex & idx) {
	for(uint32_t i = 0; i < idx.size(); i++) {
		out << idx.at(i) << std::endl;
	}
}

int main(int argc, char ** argv) {
	std::string inFileName;
	std::string outFileName;
	int64_t dumpIndexId = -1;
	bool dumpIndexStoreIndex = false;
	
	for(int i = 1; i < argc; i++) {
		std::string curArg(argv[i]);
		if (curArg == "-d" && i+1 < argc) {
			dumpIndexId = atoi(argv[i+1]);
			i++;
		}
		if (curArg == "-di")
			dumpIndexStoreIndex = true;
		if (curArg == "-o" && i+1 < argc) {
			outFileName = std::string(argv[i+1]);
			i++;
		}
		else {
			inFileName = curArg;
		}
	}
	
	if (argc < 2 || inFileName.empty()) {
		cout << "Arguments given: " << endl;
		for (int i=0; i < argc; i++) {
			cout << argv[i];
		}
		cout << endl << "Need in filename" << endl;
		return 1;
	}
	
	sserialize::UByteArrayAdapter adap( UByteArrayAdapter::open(inFileName) );
	if (!adap.size()) {
		std::cout << "Failed to open " << inFileName << "; size=0" << std::endl;
		return 1;
	}
	
	sserialize::Static::ItemIndexStore store(adap);
	std::cout << "ItemIndexStore Information:" << std::endl;
	std::cout << "size=" << store.size() << std::endl;
	
	if ((dumpIndexId >= 0 && dumpIndexId < store.size()) || dumpIndexStoreIndex) {
		std::cout << "Dumping Index with id " << dumpIndexId << std::endl;
		if (outFileName.empty())
			store.at(dumpIndexId).dump();
		else {
			std::ofstream out;
			out.open(outFileName);
			if (!out.is_open()) {
				std::cerr << "Could not open " << outFileName << std::endl;
				return 1;
			}
			if (dumpIndexStoreIndex) {
				Static::SortedOffsetIndex idx = store.getIndex();
				std::cout << "size=" << idx.size() << ";";
				std::cout << "; sizeInBytes=" << idx.getSizeInBytes();
				std::cout << std::endl;
				dumpIndex(out, idx);
			}
			else {
				ItemIndex idx = store.at(dumpIndexId);
				std::cout << "size=" << idx.size() << "; bpn=" << static_cast<uint32_t>( idx.bpn() );
				std::cout << "; sizeInBytes=" << idx.getSizeInBytes();
				std::cout << std::endl;
				dumpIndex(out, idx);
			}
			out.close();
		}
	}
}