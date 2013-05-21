#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/statfuncs.h>
#include <sserialize/utility/UByteArrayAdapter.h>

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

void incAlphabet(std::unordered_map<uint32_t, uint32_t> & a, uint32_t v) {
	if (a.count(v) == 0)
		a[v] = 1;
	else
		a[v] += 1;
}

int main(int argc, char ** argv) {
	std::string inFileName;
	std::string outFileName;
	int64_t dumpIndexId = -1;
	bool dumpIndexStoreIndex = false;
	bool dumpDataHisto = false;
	
	for(int i = 1; i < argc; i++) {
		std::string curArg(argv[i]);
		if (curArg == "-d" && i+1 < argc) {
			dumpIndexId = atoi(argv[i+1]);
			i++;
		}
		if (curArg == "-di")
			dumpIndexStoreIndex = true;
		if (curArg == "-dh")
			dumpDataHisto = true;
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
	if (dumpDataHisto) {
		UByteArrayAdapter data = store.getData();
		UByteArrayAdapter::OffsetType size = data.size();
		std::unordered_map<uint32_t, uint32_t> alphabet;
		uint32_t charSize = 4;
		if (store.indexType() == ItemIndex::T_WAH) {
			charSize = 4;
			for(UByteArrayAdapter::OffsetType i = 0; i < size; i += charSize) {
				uint32_t character = data.getUint32(i);
				if (alphabet.count(character) == 0) {
					alphabet[character] = 1;
				}
				else {
					alphabet[character] += 1;
				}
			}
		}
		else if (store.indexType() == ItemIndex::T_RLE_DE) {
			for(UByteArrayAdapter::OffsetType i = 0; i < size; ) {
				uint32_t curIndexSize = data.getUint32(i);
				i += 4;
				incAlphabet(alphabet, curIndexSize);
				uint32_t curIndexCount = data.getUint32(i);
				i += 4;
				incAlphabet(alphabet, curIndexCount);
				UByteArrayAdapter indexData(data, i, curIndexSize);
				while(indexData.tellGetPtr() < indexData.size()) {
					uint32_t c = indexData.getVlPackedUint32();
					incAlphabet(alphabet, c);
				}
				i += curIndexSize;
			}
		}
		std::cout << "#Data lenth: " << size << std::endl;
		std::cout << "#Alphabet size: " << alphabet.size() << std::endl;
		std::cout << "#String length: " << size/charSize << std::endl;
		std::cout << "#String length/Alphabet size: " << (size/charSize)/((double)alphabet.size()) << std::endl;
		double entropy = sserialize::statistics::entropy<std::unordered_map<uint32_t, uint32_t>::const_iterator, double>(alphabet.begin(), alphabet.end(), 0.0, size/charSize);
		uint64_t compressedSize = static_cast<uint64_t>(size)/charSize*std::ceil(entropy)/8;
		uint64_t compressedSizeWithDict = compressedSize + alphabet.size()*charSize;
		std::cout << "#Entropy: " << entropy << std::endl;
		std::cout << "Compressed length: " << compressedSize << "(" << static_cast<double>(compressedSize)/size*100 << "%)" << std::endl;
		std::cout << "Compressed length with dict: " << compressedSizeWithDict << "(" << static_cast<double>(compressedSizeWithDict)/size*100 << "%)" << std::endl;
		
	}
}