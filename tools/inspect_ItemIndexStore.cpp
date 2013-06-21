#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/statfuncs.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/templated/HuffmanTree.h>
#include <sserialize/containers/MultiBitBackInserter.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <vendor/libs/minilzo/minilzo.h>

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

#define HEAP_ALLOC_MINI_LZO(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]


UByteArrayAdapter::OffsetType recompressLZO(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest, double compressionRatio) {
	UByteArrayAdapter data = store.getData();
	
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(2);
	dest.putUint8(store.indexType());
	dest.putUint8(Static::ItemIndexStore::IC_LZO);
	dest.putOffset(0);
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	newOffsets.reserve(store.size());
	
	HEAP_ALLOC_MINI_LZO(wrkmem, LZO1X_1_MEM_COMPRESS);

	
	uint32_t bufferSize = 10*1024*1024;
	uint8_t * inBuf = new uint8_t[bufferSize];
	uint8_t * outBuf = new uint8_t[2*bufferSize];
	
	data.resetGetPtr();
	ProgressInfo pinfo;
	pinfo.begin(data.size(), "Encoding words");
	for(uint32_t i = 0; i < store.size(); ++i ) {
		UByteArrayAdapter idxData( store.dataAt(i) );
		if (idxData.size() > bufferSize) {
			delete inBuf;
			delete outBuf;
			bufferSize = idxData.size();
			inBuf = new uint8_t[bufferSize];
			outBuf = new uint8_t[2*bufferSize];
			
		}
		lzo_uint inBufLen = idxData.size();
		lzo_uint outBufLen = bufferSize*2;
		idxData.get(inBuf, inBufLen);
		int r = ::lzo1x_1_compress(inBuf,inBufLen,outBuf,&outBufLen,wrkmem);
		if (r != LZO_E_OK) {
			delete[] inBuf;
			delete[] outBuf;
			std::cerr << "Compression Error" << std::endl;
			return 0;
		}
		double cmpRatio = (double)inBufLen / outBufLen;
		if (cmpRatio >= compressionRatio) {
			dest.put(outBuf, outBufLen);
		}
		else {
			dest.put(inBuf, inBufLen);
		}
		pinfo(data.tellGetPtr());
	}
	pinfo.end("Encoded words");
	dest.putOffset(beginOffset+3, dest.tellPutPtr()-beginOffset);
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	std::cout << "Offset index created. Total size: " << dest.tellPutPtr()-beginOffset;
	return dest.tellPutPtr()+8-beginOffset;
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
	if (store.indexType() != ItemIndex::T_WAH) {
		std::cerr << "Unsupported index format" << std::endl;
		return 0;
	}
	
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(2);
	dest.putUint8(ItemIndex::T_WAH);
	dest.putUint8(Static::ItemIndexStore::IC_VARUINT32);
	dest.putOffset(0);
	UByteArrayAdapter::OffsetType destDataBeginOffset = dest.tellPutPtr();
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	newOffsets.reserve(store.size());
	
	
	UByteArrayAdapter data = store.getData();
	data.resetGetPtr();
	ProgressInfo pinfo;
	pinfo.begin(data.size(), "Encoding words");
	while(data.getPtrHasNext()) {
		newOffsets.push_back(dest.tellPutPtr()-destDataBeginOffset);
		uint32_t indexSize = data.getUint32();
		uint32_t indexCount = data.getUint32();
		dest.putVlPackedUint32(indexSize);
		dest.putVlPackedUint32(indexCount);
		indexSize = indexSize / 4;
		for(uint32_t i = 0; i < indexSize; ++i) {
			uint32_t src = data.getUint32();
			dest.putVlPackedUint32(src);
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
	dest.putUint8(2);
	dest.putUint8(ItemIndex::T_WAH);
	dest.putUint8(Static::ItemIndexStore::IC_HUFFMAN);
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
	dest.putOffset(beginOffset+3, dest.tellPutPtr()-beginOffset);
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	//now comes the deocding table
	HuffmanTree<uint32_t>::ValueSerializer sfn = &putWrapper;
	std::vector<uint8_t> bitsPerLevel;
	bitsPerLevel.push_back(16);
	int htDepth = ht.depth()-16;
	while (htDepth > 0) {
		if (htDepth > 4)
			bitsPerLevel.push_back(4);
		else
			bitsPerLevel.push_back(htDepth);
		htDepth -= 4;
	}
	ht.serialize(dest, sfn, bitsPerLevel);
	std::cout << "Offset index created. Total size: " << dest.tellPutPtr()-beginOffset;
	return dest.tellPutPtr()+8-beginOffset;
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
	double recompressWithLZO = -1.0;
	
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
		else if (curArg == "-rclzo" && i+1 < argc) {
			recompressWithLZO = atof(argv[i+1]);
			++i;
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
		UByteArrayAdapter::OffsetType size = recompressIndexData(1, store, outData);
		if (size > 0)
			outData.shrinkStorage(adap.size()-size);
		else
			outData.setDeleteOnClose(true);
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
	}
	

	if (recompressWithLZO > 0.0) {
		std::string outFile;
		if (outFileName.empty())
			outFile = inFileName + ".lzocmp";
		else
			outFile = outFileName;
		UByteArrayAdapter outData(UByteArrayAdapter::createFile(adap.size(), outFile));
		UByteArrayAdapter::OffsetType size =  recompressLZO(store, outData, recompressWithLZO);
		if (size > 0)
			outData.shrinkStorage(adap.size()-size);
		else
			outData.setDeleteOnClose(true);
	}
	
	if (dumpStats) {
		std::cout <<  "#element count\tindex size" << std::endl;
		for(uint32_t i = 0; i < store.size(); ++i) {
			std::cout << store.at(i).size() << "\t" << store.dataAt(i).size() << std::endl;
		}
	}
}