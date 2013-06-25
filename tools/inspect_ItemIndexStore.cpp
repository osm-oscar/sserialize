#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/statfuncs.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/templated/HuffmanTree.h>
#include <sserialize/containers/MultiBitBackInserter.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/containers/ItemIndexFactory.h>

using namespace std;
using namespace sserialize;


void putWrapper(UByteArrayAdapter & dest, const uint32_t & src) {
	dest.putUint32(src);
}

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
	else {
		std::cerr << "Unsupported index format for createAlphabet" << std::endl;
	}
}

UByteArrayAdapter::OffsetType recompressLZO(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	return sserialize::ItemIndexFactory::compressWithLZO(store, dest);
}

UByteArrayAdapter::OffsetType recompressVarUintShannon(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	if (store.indexType() != ItemIndex::T_WAH) {
		std::cerr << "Unsupported index format" << std::endl;
		return 0;
	}
	
	std::unordered_map<uint32_t, uint32_t> alphabet; //maps from old to new
	{
		uint32_t charSize = 4;
		createAlphabet(store, alphabet, charSize);
		std::cout << "Created alphabetChar->freq mapping of size: " << alphabet.size() << std::endl;
		std::vector< std::pair<uint32_t, uint32_t> > tmp;
		tmp.reserve(alphabet.size());
		for(std::unordered_map<uint32_t, uint32_t>::const_iterator it(alphabet.begin()); it != alphabet.end(); ++it)
			tmp.push_back(std::pair<uint32_t, uint32_t>(it->second, it->first));
		std::sort(tmp.begin(), tmp.end());
		for(int i = tmp.size()-1; i >= 0; --i)
			alphabet[tmp[i].second] = i; 
	}
	
	std::cout << "Alphabet size: " << alphabet.size() << std::endl;

	UByteArrayAdapter data = store.getData();
	
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(2);
	dest.putUint8(ItemIndex::T_WAH);
	dest.putUint8(Static::ItemIndexStore::IC_ILLEGAL);
	dest.putOffset(0);
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	newOffsets.reserve(store.size());
	UByteArrayAdapter::OffsetType beginDataOffset = dest.tellPutPtr();
	
	data.resetGetPtr();
	ProgressInfo pinfo;
	pinfo.begin(data.size(), "Encoding words");
	while(data.getPtrHasNext()) {
		newOffsets.push_back(dest.tellPutPtr()-beginDataOffset);
		uint32_t indexSize = data.getUint32();
		uint32_t indexCount = data.getUint32();
		dest.putVlPackedUint32(alphabet.at(indexSize));
		dest.putVlPackedUint32(alphabet.at(indexCount));
		indexSize = indexSize / 4;
		for(uint32_t i = 0; i < indexSize; ++i) {
			uint32_t src = data.getUint32();
			dest.putVlPackedUint32(alphabet.at(src));
		}
		pinfo(data.tellGetPtr());
	}
	pinfo.end("Encoded words");
	dest.putOffset(beginOffset+3, dest.tellPutPtr()-beginOffset);
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	std::cout << "Offset index created. Total size: " << dest.tellPutPtr()-beginOffset;
	return dest.tellPutPtr()-beginOffset;
}

UByteArrayAdapter::OffsetType recompressDataVarUint(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	return sserialize::ItemIndexFactory::compressWithVarUint(store, dest);
}

UByteArrayAdapter::OffsetType recompressIndexData(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	return sserialize::ItemIndexFactory::compressWithHuffman(store, dest);
}

bool checkCompressedIndex(sserialize::Static::ItemIndexStore & real, sserialize::Static::ItemIndexStore & compressed) {
	if (real.size() != compressed.size()) {
		std::cerr << "ItemIndexStore sizes don't match" << std::endl;
		return false;
	}
	ProgressInfo info;
	info.begin(real.size(), "Testing compressed index");
	for(uint32_t i = 0; i < real.size(); ++i) {
		if (real.at(i) != compressed.at(i)) {
			std::cout << "ItemIndex at postion " << i << " are unequal" << std::endl;
			return false;
		}
		info(i);
	}
	info.end("Tested compressed index");
	return true;
}

int main(int argc, char ** argv) {
	std::string inFileName;
	std::string outFileName;
	int64_t dumpIndexId = -1;
	bool dumpIndexStoreIndex = false;
	bool dumpDataHisto = false;
	bool dumpStats = false;
	uint8_t alphabetBitLength = 1;
	bool recompress = false;
	bool recompressVar = false;
	bool recompressVarShannon = false;
	bool recompressWithLZO = false;
	bool checkCompressed = false;
	
	for(int i = 1; i < argc; i++) {
		std::string curArg(argv[i]);
		if (curArg == "-d" && i+1 < argc) {
			dumpIndexId = atoi(argv[i+1]);
			i++;
		}
		else if (curArg == "-di")
			dumpIndexStoreIndex = true;
		else if (curArg == "-dh")
			dumpDataHisto = true;
		else if (curArg == "-abl" && i+1 < argc) {
			alphabetBitLength = ::atoi(argv[i+1]);
			++i;
		}
		else if (curArg == "-rc") {
			recompress = true;
		}
		else if (curArg == "-rcv") {
			recompressVar = true;
		}
		else if (curArg == "-rcvs") {
			recompressVarShannon = true;
		}
		else if (curArg == "-rclzo") {
			recompressWithLZO = true;
		}
		else if (curArg == "-cc") {
			checkCompressed = true;
		}
		else if (curArg == "-o" && i+1 < argc) {
			outFileName = std::string(argv[i+1]);
			i++;
		}
		else if (curArg == "-ds") {
			dumpStats = true;
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
		UByteArrayAdapter::OffsetType size = recompressIndexData(store, outData);
		if (size > 0)
			outData.shrinkStorage(adap.size()-size);
		else
			outData.setDeleteOnClose(true);
		if (checkCompressed) {
			sserialize::Static::ItemIndexStore csdb(outData);
			
		}
	}
	
	if (recompressVar) {
		std::string outFile;
		if (outFileName.empty())
			outFile = inFileName + ".vcmp";
		else
			outFile = outFileName;
		UByteArrayAdapter outData(UByteArrayAdapter::createFile(adap.size(), outFile));
		UByteArrayAdapter::OffsetType size = recompressDataVarUint(store, outData);
		if (size > 0)
			outData.shrinkStorage(adap.size()-size);
		else
			outData.setDeleteOnClose(true);
		if (checkCompressed) {
			std::cout << "Checking compressed index for equality..." << std::endl;
			sserialize::Static::ItemIndexStore cis(outData);
			if (checkCompressedIndex(store, cis)) {
				std::cout << "Compressed index is equal to uncompressed index." << std::endl;
			}
		}
	}
	

	if (recompressVarShannon) {
		std::string outFile;
		if (outFileName.empty())
			outFile = inFileName + ".vscmp";
		else
			outFile = outFileName;
		UByteArrayAdapter outData(UByteArrayAdapter::createFile(adap.size(), outFile));
		UByteArrayAdapter::OffsetType size = recompressVarUintShannon(store, outData);
		if (size > 0)
			outData.shrinkStorage(adap.size()-size);
		else
			outData.setDeleteOnClose(true);
		if (checkCompressed) {
			std::cout << "Checking compressed index for equality..." << std::endl;
			sserialize::Static::ItemIndexStore cis(outData);
			if (checkCompressedIndex(store, cis)) {
				std::cout << "Compressed index is equal to uncompressed index." << std::endl;
			}
		}
	}
	

	if (recompressWithLZO) {
		std::string outFile;
		if (outFileName.empty())
			outFile = inFileName + ".lzocmp";
		else
			outFile = outFileName;
		UByteArrayAdapter outData(UByteArrayAdapter::createFile(adap.size(), outFile));
		UByteArrayAdapter::OffsetType size =  recompressLZO(store, outData);
		if (size > 0)
			outData.shrinkStorage(adap.size()-size);
		else
			outData.setDeleteOnClose(true);
		if (checkCompressed) {
			std::cout << "Checking compressed index for equality..." << std::endl;
			sserialize::Static::ItemIndexStore cis(outData);
			if (checkCompressedIndex(store, cis)) {
				std::cout << "Compressed index is equal to uncompressed index." << std::endl;
			}
		}
	}
	
	if (dumpStats) {
		std::cout <<  "#element count\tindex size" << std::endl;
		for(uint32_t i = 0; i < store.size(); ++i) {
			std::cout << store.at(i).size() << "\t" << store.dataAt(i).size() << std::endl;
		}
	}
}