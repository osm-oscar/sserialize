#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/statfuncs.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/templated/HuffmanTree.h>
#include <sserialize/containers/MultiBitBackInserter.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>

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

void createAlphabet(sserialize::Static::ItemIndexStore & store, std::unordered_map<uint32_t, uint32_t> & alphabet, uint32_t & charSize) {
	UByteArrayAdapter data = store.getData();
	UByteArrayAdapter::OffsetType size = data.size();
	if (store.indexType() == ItemIndex::T_WAH) {
		charSize = 4;
		for(UByteArrayAdapter::OffsetType i = 0; i < size; i += charSize) {
			uint32_t character = data.getUint32(i);
			incAlphabet(alphabet, character);
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
}

UByteArrayAdapter::OffsetType recompressIndexData(uint8_t alphabetBitLength, sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	if (store.indexType() != ItemIndex::T_WAH) {
		std::cerr << "Unsupported index format" << std::endl;
		return 0;
	}
	UByteArrayAdapter data = store.getData();
	uint64_t size = data.size();
	std::unordered_map<uint32_t, uint32_t> alphabet;
	uint32_t charSize = 4;
	createAlphabet(store, alphabet, charSize);
	HuffmanTree<uint32_t> ht(alphabetBitLength);
	ht.create(alphabet.begin(), alphabet.end(), size/charSize);
	std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());
	
	
	//now recompress
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(0);
	dest.putUint8(0);
	dest.putOffset(0);
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	newOffsets.reserve(store.size());
	
	MultiBitBackInserter backInserter(dest);
	data.resetGetPtr();
	ProgressInfo pinfo;
	pinfo.begin(data.size(), "Encoding words");
	while(data.getPtrHasNext()) {
		newOffsets.push_back(backInserter.data().size()-7);
		uint32_t indexSize = data.getUint32();
		uint32_t indexCount = data.getUint32();
		const HuffmanCodePoint & sizeCp = htMap.at(indexSize);
		const HuffmanCodePoint & countCp = htMap.at(indexCount);
		backInserter.push_back(sizeCp.code(), sizeCp.codeLength());
		backInserter.push_back(countCp.code(), countCp.codeLength());
		indexSize = indexSize / 4;
		for(uint32_t i = 0; i < indexSize; ++i) {
			uint32_t src = data.getUint32();
			const HuffmanCodePoint & srcCp =  htMap.at(src);
			backInserter.push_back(srcCp.code(), srcCp.codeLength());
		}
		backInserter.flush();
		pinfo(data.tellGetPtr());
	}
	backInserter.flush();
	dest = backInserter.data();
	pinfo.end("Encoded words");
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	std::cout << "Offset index created. Total size: " << dest.tellPutPtr()-beginOffset;
	return dest.tellPutPtr()+7-beginOffset;
}

int main(int argc, char ** argv) {
	std::string inFileName;
	std::string outFileName;
	int64_t dumpIndexId = -1;
	bool dumpIndexStoreIndex = false;
	bool dumpDataHisto = false;
	uint8_t alphabetBitLength = 1;
	bool recompress = false;
	
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
		if (curArg == "-abl" && i+1 < argc) {
			alphabetBitLength = ::atoi(argv[i+1]);
			++i;
		}
		if (curArg == "-rc")
			recompress = true;
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
		uint32_t charSize = 4;
		uint64_t size = store.getData().size();
		std::unordered_map<uint32_t, uint32_t> alphabet;
		createAlphabet(store, alphabet, charSize);
	
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
		std::cout << "Creating huffman tree with alphabetBitLength=" << static_cast<uint32_t>(alphabetBitLength) << std::endl;
		HuffmanTree<uint32_t> ht(alphabetBitLength);
		ht.create(alphabet.begin(), alphabet.end(), size/charSize);
		std::cout << "Huffman tree depth: " << ht.depth() << std::endl;
		std::cout << "Huffman tree min code length: " << ht.levelOfFirstLeaf() << std::endl;
		
		std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());
		std::cout << "Calculating compressed size (without ht tables):" << std::flush;
		uint64_t bitUsage = 0;
		for(std::unordered_map<uint32_t, uint32_t>::const_iterator it(alphabet.begin()); it != alphabet.end(); ++it) {
			bitUsage += htMap.at(it->first).codeLength()*it->second;
		}
		std::cout << bitUsage/8 << "(" << (double)(bitUsage/8) / size * 100 << "%)" << std::endl;
	}
	
	if (recompress) {
		std::string outFile;
		if (outFileName.empty())
			outFile = inFileName + ".htcmp";
		else
			outFile = outFileName;
		UByteArrayAdapter outData(UByteArrayAdapter::createFile(adap.size(), outFile));
		UByteArrayAdapter::OffsetType size = recompressIndexData(1, store, outData);
		if (size > 0)
			outData.shrinkStorage(adap.size()-size);
		else
			outData.setDeleteOnClose(true);
	}
}