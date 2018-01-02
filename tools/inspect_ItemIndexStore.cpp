#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/stats/statfuncs.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/containers/HuffmanTree.h>
#include <sserialize/iterator/MultiBitBackInserter.h>
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

bool doCheckIndex(const sserialize::Static::ItemIndexStore & indexStore) {
	sserialize::ProgressInfo pinfo;
	pinfo.begin(indexStore.size(), "Checking index store");
	for(uint32_t i(0), s(indexStore.size()); i < s; ++i) {
		sserialize::ItemIndex idx(indexStore.at(i));
		if (idx.size() != indexStore.idxSize(i)) {
			std::cout << "Idx size does not match real index size\n";
			return false;
		}
		pinfo(i);
	}
	pinfo.end();
	return true;
}

inline void incAlphabet(std::unordered_map<uint32_t, uint32_t> & a, uint32_t v) {
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

void dumpDataHistoFunc(sserialize::Static::ItemIndexStore & store, 	uint8_t alphabetBitLength) {
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
		for(std::unordered_map<uint32_t, uint32_t>::const_iterator it(alphabet.begin()); it != alphabet.end(); ++it) {
			tmp.push_back(std::pair<uint32_t, uint32_t>(it->second, it->first));
		}
		std::sort(tmp.begin(), tmp.end());
		for(int64_t i = (int64_t) (tmp.size()-1); i >= 0; --i) {
			alphabet[tmp[i].second] = (uint32_t) i;
		}
	}
	
	std::cout << "Alphabet size: " << alphabet.size() << std::endl;

	UByteArrayAdapter data = store.getData();
	
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(2);
	dest.putUint8(ItemIndex::T_WAH);
	dest.putUint8(0xFF);
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

UByteArrayAdapter::OffsetType recompressLZO(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	return sserialize::ItemIndexFactory::compressWithLZO(store, dest);
}

UByteArrayAdapter::OffsetType recompressDataVarUint(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	return sserialize::ItemIndexFactory::compressWithVarUint(store, dest);
}

UByteArrayAdapter::OffsetType recompressIndexDataHuffman(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	return sserialize::ItemIndexFactory::compressWithHuffman(store, dest);
}

bool checkCompressedIndex(sserialize::Static::ItemIndexStore & real, sserialize::Static::ItemIndexStore & compressed) {
	if (real.size() != compressed.size()) {
		std::cerr << "ItemIndexStore sizes don't match: src=" << real.size() << ", tgt=" << compressed.size() << std::endl;
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

void printHelp() {
	std::cout <<
	"Program options: \n \
	-d id\t dump index \n \
	-di\tdump index of index store\n \
	-abl num\talphabet bit length for huffman encoding \n \
	-rch\trecompress with huffmann \n \
	-rcv\trecompress with varuint32 \n \
	-rclzo\trecompress with lzo \n \
	-cc\tcheck compressed/transformed \n \
	-o filename\tout file name\n \
	-eq filename\tequality test \n \
	-ds\tdump stats \n \
	-t type\ttransform to (rline|wah|de|rlede|simple|native|eliasfano|pfor) \n \
	-nd\tdisable deduplication of item index store \n \
	-c\tcheck item index store \
	" << std::endl;
}

int main(int argc, char ** argv) {
	{ //init rand
		timeval t1;
		gettimeofday(&t1, NULL);
		srand(t1.tv_usec * t1.tv_sec);
	}
	std::string inFileName;
	std::string outFileName;
	std::vector<int64_t> dumpIndexId;
	bool dumpIndexStoreIndex = false;
	bool dumpDataHisto = false;
	bool dumpStats = false;
	uint8_t alphabetBitLength = 1;
	bool recompressHuffman = false;
	bool recompressVar = false;
	bool recompressVarShannon = false;
	bool recompressWithLZO = false;
	bool checkCompressed = false;
	bool checkIndex = false;
	bool deduplication = true;
	std::string equalityTest;
	ItemIndex::Types transform = ItemIndex::T_NULL;
	
	for(int i = 1; i < argc; i++) {
		std::string curArg(argv[i]);
		if (curArg == "-d" && i+1 < argc) {
			dumpIndexId.push_back(atoi(argv[i+1]));
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
		else if (curArg == "-rch") {
			recompressHuffman = true;
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
		else if (curArg == "-c") {
			checkIndex = true;
		}
		else if (curArg == "-o" && i+1 < argc) {
			outFileName = std::string(argv[i+1]);
			i++;
		}
		else if (curArg == "-eq" && i+1 < argc) {
			equalityTest = argv[i+1];
		}
		else if (curArg == "-ds") {
			dumpStats = true;
		}
		else if (curArg == "-t" && i+1 < argc) {
			std::string t(argv[i+1]);
			if (t == "rline")
				transform = sserialize::ItemIndex::T_REGLINE;
			else if (t == "wah")
				transform = sserialize::ItemIndex::T_WAH;
			else if (t == "de")
				transform = sserialize::ItemIndex::T_DE;
			else if (t == "rlede")
				transform = sserialize::ItemIndex::T_RLE_DE;
			else if (t == "simple")
				transform = sserialize::ItemIndex::T_SIMPLE;
			else if (t == "native")
				transform = sserialize::ItemIndex::T_NATIVE;
			else if (t == "eliasfano")
				transform = sserialize::ItemIndex::T_ELIAS_FANO;
			else if (t == "pfor")
				transform = sserialize::ItemIndex::T_PFOR;
			++i;
		}
		else if (curArg == "-nd") {
			deduplication = false;
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
		printHelp();
		return 1;
	}
	
	sserialize::UByteArrayAdapter adap( UByteArrayAdapter::open(inFileName) );
	if (!adap.size()) {
		std::cout << "Failed to open " << inFileName << "; size=0" << std::endl;
		return 1;
	}
	
	#ifdef SSERIALIZE_UBA_OPTIONAL_REFCOUNTING
		adap.disableRefCounting();
	#endif
	
	sserialize::Static::ItemIndexStore store(adap);
	std::cout << "ItemIndexStore Information:" << std::endl;
	std::cout << "size=" << store.size() << std::endl;
	
	if ((dumpIndexId.size()) || dumpIndexStoreIndex) {
		std::ostream * out;
		std::ofstream fout;
		if (outFileName.empty()) {
			out = &std::cout;
		}
		else {
			fout.open(outFileName);
			if (!fout.is_open()) {
				std::cerr << "Could not open " << outFileName << std::endl;
				return 1;
			}
			out = &fout;
		}
		if (dumpIndexStoreIndex) {
			Static::SortedOffsetIndex idx = store.getIndex();
			*out << "size=" << idx.size() << ";";
			*out << "; sizeInBytes=" << idx.getSizeInBytes();
			*out << std::endl;
			dumpIndex(*out, idx);
		}
		for(uint32_t i = 0, s = (uint32_t) dumpIndexId.size(); i < s; ++i) {
			if (dumpIndexId[i] >= store.size()) {
				continue;
			}
			std::cout << "ItemIndex with id " << dumpIndexId[i] << ": " << std::endl;
			store.at((uint32_t) dumpIndexId[i]).dump(*out);
		}
		if (fout.is_open()) {
			fout.close();
		}
	}
	if (dumpDataHisto) {
		dumpDataHistoFunc(store, alphabetBitLength);
	}
	
	if (recompressHuffman) {
		std::string outFile;
		if (outFileName.empty())
			outFile = inFileName + ".htcmp";
		else
			outFile = outFileName;
		UByteArrayAdapter outData(UByteArrayAdapter::createFile(adap.size(), outFile));
		UByteArrayAdapter::OffsetType size = recompressIndexDataHuffman(store, outData);
		if (size > 0)
			outData.shrinkStorage(outData.size()-size);
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
	
	if (recompressVar) {
		std::string outFile;
		if (outFileName.empty())
			outFile = inFileName + ".vcmp";
		else
			outFile = outFileName;
		UByteArrayAdapter outData(UByteArrayAdapter::createFile(adap.size(), outFile));
		UByteArrayAdapter::OffsetType size = recompressDataVarUint(store, outData);
		if (size > 0)
			outData.shrinkStorage(outData.size()-size);
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
			outData.shrinkStorage(outData.size()-size);
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
			outData.shrinkStorage(outData.size()-size);
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
		store.printStats(std::cout);
	}
	
	if (!equalityTest.empty()) {
		UByteArrayAdapter otherAdap(UByteArrayAdapter::open(equalityTest) );
		Static::ItemIndexStore otherStore(otherAdap);
		if (checkCompressedIndex(otherStore, store)) {
			std::cout << "Stores are equal" << std::endl;
		}
	}
	
	if (checkIndex) {
		if (doCheckIndex(store)) {
			std::cout << "Detected NO errors in ItemIndexStore" << std::endl;
		}
		else {
			std::cout << "Detected ERRORS in ItemIndexStore" << std::endl;
		}
	}
	
	if (transform != ItemIndex::T_NULL) {
		if (outFileName.empty()) {
			outFileName = inFileName + sserialize::to_string(transform);
		}
		UByteArrayAdapter outData(UByteArrayAdapter::createFile(adap.size(), outFileName));
		ItemIndexFactory factory;
		factory.setType(transform);
		factory.setIndexFile(outData);
		factory.setDeduplication(deduplication);
		factory.setCheckIndex(checkCompressed);
		std::cout << "Transforming index" << std::endl;
		factory.insert(store);
		std::cout << "Serializing IndexStore" << std::endl;
		UByteArrayAdapter::OffsetType s = factory.flush();
		outData = factory.getFlushedData();
		std::cout << "Created index store with a size of " << s << std::endl;
		outData.shrinkStorage(outData.size() - s);
		
		if (checkCompressed) {
			Static::ItemIndexStore tstore(outData);
			if (!checkCompressedIndex(store, tstore)) {
				std::cerr << "Stores are not equal" << std::endl;
			}
		}
	}
	
	#ifdef SSERIALIZE_UBA_OPTIONAL_REFCOUNTING
		adap.enableRefCounting();
	#endif
}