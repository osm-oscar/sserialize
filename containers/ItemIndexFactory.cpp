#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/debuggerfunctions.h>
#include <sserialize/utility/ProgressInfo.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/templated/HuffmanTree.h>
#include <sserialize/containers/MultiBitBackInserter.h>
#include <vendor/libs/minilzo/minilzo.h>
#include <unordered_map>
#include <iostream>
#include <sstream>

namespace sserialize {

ItemIndexFactory::ItemIndexFactory(bool memoryBased) :
m_indexIdCounter(0),
m_hitCount(0),
m_checkIndex(true),
m_bitWidth(-1),
m_useRegLine(true),
m_type(ItemIndex::T_REGLINE)
{
	if (memoryBased)
		 setIndexFile( UByteArrayAdapter(new std::vector<uint8_t>(), true) );
	else
		setIndexFile( UByteArrayAdapter::createCache(8*1024*1024, true) );
}

ItemIndexFactory::~ItemIndexFactory() {}

UByteArrayAdapter ItemIndexFactory::at(sserialize::OffsetType offset) const {
	return m_indexStore+offset;
}

void ItemIndexFactory::setIndexFile(sserialize::UByteArrayAdapter data) {
	if (m_indexIdCounter > 0) { //clear eversthing
		m_indexIdCounter = 0;
		m_hitCount = 0;
		m_hash.clear();
		m_offsetsToId.clear();
	}

	m_header = data;
	m_header.putUint8(0); // dummy version
	m_header.putUint8(0); //dumy index type
	m_header.putUint8(0); //dummy compression type
	m_header.putOffset(0); //dummy offset
	m_indexStore = m_header;
	m_indexStore.shrinkToPutPtr();
	m_header.resetPtrs();
	addIndex(std::set<uint32_t>(), 0);
}

uint64_t ItemIndexFactory::hashFunc(const std::vector<uint8_t> & v) {
	uint64_t h = 0;
	uint32_t count = std::min<uint32_t>(v.size(), 1024);
#if 1
	const uint8_t * vPtr = &(v[0]);
	for(size_t i = 0; i < count; ++i, ++vPtr)
		h += *vPtr;
#else
	for(size_t i = 0, k = 0; i < count; i = ((i+1) << 1), k+=8) {
		uint64_t tmp = 0;
		for(size_t j = 0; j+i < count; ++j) {
			tmp |= (static_cast<uint64_t>(v[i+j]) << j);
		}
		h += (tmp << k);
	}
#endif
	count = std::min<uint32_t>(count, 16*4);
	for(size_t i = 3;  i < count; i+=4) {
		h += (static_cast<uint64_t>(v[i]) << 48);
	}
	return h;
}

int64_t ItemIndexFactory::getIndex(const std::vector<uint8_t> & v, uint64_t & hv) {
	if (v.size() == 0)
		return -1;
	hv = hashFunc(v);
	if (m_hash.count(hv) == 0) {
		return -1;
	}
	else {
		std::list<OffsetType>::const_iterator end = m_hash.at(hv).end();
		for(std::list<OffsetType>::const_iterator it = m_hash.at(hv).begin(); it != end; ++it) {
			if (indexInStore(v, *it))
				return *it;
		}
	}
	return -1;
}

bool ItemIndexFactory::indexInStore(const std::vector< uint8_t >& v, uint64_t offset) {
	if (v.size() > (m_indexStore.tellPutPtr()-offset))
		return false;
	for(size_t i = 0; i < v.size(); i++) {
		if (v.at(i) != m_indexStore.at(offset+i))
			return false;
	}
	return true;
}

uint32_t ItemIndexFactory::addIndex(const std::vector< uint8_t >& idx, sserialize::OffsetType * indexOffset) {
	uint64_t hv;
	int64_t indexPos = getIndex(idx, hv);
	if (indexPos < 0) {
		indexPos = m_indexStore.tellPutPtr();
		m_offsetsToId[indexPos] = m_indexIdCounter;
		m_indexIdCounter++;
		m_indexStore.put(idx);
		m_hash[hv].push_back(indexPos);//deque<> takes of creating an empty list if none exists
	}
	else {
		m_hitCount++;
	}
	if (indexOffset)
		*indexOffset = indexPos;
	return m_offsetsToId[indexPos];
}

uint32_t ItemIndexFactory::addIndex(const std::unordered_set<uint32_t> & idx, bool * ok, OffsetType * indexOffset) {
	return addIndex< std::set<uint32_t> >(std::set<uint32_t>(idx.begin(), idx.end()), ok, indexOffset);
}

UByteArrayAdapter ItemIndexFactory::getFlushedData() {
	UByteArrayAdapter fd = m_header;
	fd.growStorage(m_indexStore.tellPutPtr());
	return fd;
}


OffsetType ItemIndexFactory::flush() {
	std::cout << "Serializing index with type=" << m_type << std::endl;
	m_header.resetPtrs();
	m_header << static_cast<uint8_t>(2); //Version
	m_header << static_cast<uint8_t>(m_type);//type
	m_header << static_cast<uint8_t>(Static::ItemIndexStore::IC_NONE);
	m_header.putOffset(m_indexStore.tellPutPtr());
	
	std::cout << "Gathering offsets...";
	//Create the offsets
	std::vector<uint64_t> os(m_offsetsToId.size(), 0);
	for(std::unordered_map<uint64_t, uint32_t>::const_iterator it = m_offsetsToId.begin(); it != m_offsetsToId.end(); ++it) {
			os[it->second] = it->first;
	}
	std::cout << os.size() << " gathered" << std::endl;
	std::cout << "Serializing offsets...";
	uint64_t oIBegin = m_indexStore.tellPutPtr();
	if (! Static::SortedOffsetIndexPrivate::create(os, m_indexStore) ) {
		std::cout << "ItemIndexFactory::serialize: failed to create Offsetindex." << std::endl;
		return 0;
	}
	else {
		UByteArrayAdapter oIData(m_indexStore, oIBegin);
		sserialize::Static::SortedOffsetIndex oIndex(oIData);
		if (os != oIndex) {
			std::cout << "OffsetIndex creation FAILED!" << std::endl;
		}
	}
	std::cout << "done." << std::endl;

	return 3+UByteArrayAdapter::OffsetTypeSerializedLength()+m_indexStore.tellPutPtr();
}

void putWrapper(UByteArrayAdapter & dest, const uint32_t & src) {
	dest.putUint32(src);
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

void createHuffmanTree(sserialize::Static::ItemIndexStore & store, HuffmanTree<uint32_t> & ht) {
	UByteArrayAdapter::OffsetType size = store.getData().size();
	std::unordered_map<uint32_t, uint32_t> alphabet;
	uint32_t charSize = 4;
	createAlphabet(store, alphabet, charSize);
	ht = HuffmanTree<uint32_t>(1);
	ht.create(alphabet.begin(), alphabet.end(), size/charSize);
	std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());
}

UByteArrayAdapter::OffsetType compressWithHuffmanRLEDE(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	UByteArrayAdapter data = store.getData();
	HuffmanTree<uint32_t> ht;
	createHuffmanTree(store, ht);
	std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());
	
	
	//now recompress
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(2);
	dest.putUint8(ItemIndex::T_RLE_DE);
	dest.putUint8(Static::ItemIndexStore::IC_HUFFMAN);
	dest.putOffset(0);
	UByteArrayAdapter::OffsetType destDataBeginOffset = dest.tellPutPtr();
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	newOffsets.reserve(store.size());
	
	MultiBitBackInserter backInserter(dest);
	data.resetGetPtr();
	ProgressInfo pinfo;
	pinfo.begin(data.size(), "Encoding words");
	while(data.getPtrHasNext()) {
		newOffsets.push_back(backInserter.data().tellPutPtr()-destDataBeginOffset);

		uint32_t indexSize = data.getUint32();
		uint32_t indexCount = data.getUint32();
		const HuffmanCodePoint & sizeCp = htMap.at(indexSize);
		const HuffmanCodePoint & countCp = htMap.at(indexCount);
		backInserter.push_back(sizeCp.code(), sizeCp.codeLength());
		backInserter.push_back(countCp.code(), countCp.codeLength());

		UByteArrayAdapter indexData = data;
		indexData.shrinkToGetPtr();
		while(indexData.tellGetPtr() < indexSize) {
			uint32_t src = indexData.getVlPackedUint32();
			const HuffmanCodePoint & srcCp =  htMap.at(src);
			backInserter.push_back(srcCp.code(), srcCp.codeLength());
		}
		data.incGetPtr(indexSize);
		backInserter.flush();
		pinfo(data.tellGetPtr());
	}
	backInserter.flush();
	pinfo.end("Encoded words");
	dest = backInserter.data();
	
	dest.putOffset(beginOffset+3, dest.tellPutPtr()-destDataBeginOffset);
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
	return dest.tellPutPtr()-beginOffset;
}

UByteArrayAdapter::OffsetType compressWithHuffmanWAH(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	UByteArrayAdapter data = store.getData();
	HuffmanTree<uint32_t> ht;
	createHuffmanTree(store, ht);
	std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());	
	
	//now recompress
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(2);
	dest.putUint8(ItemIndex::T_WAH);
	dest.putUint8(Static::ItemIndexStore::IC_HUFFMAN);
	dest.putOffset(0);
	UByteArrayAdapter::OffsetType destDataBeginOffset = dest.tellPutPtr();
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	newOffsets.reserve(store.size());
	
	MultiBitBackInserter backInserter(dest);
	data.resetGetPtr();
	ProgressInfo pinfo;
	pinfo.begin(data.size(), "Encoding words");
	while(data.getPtrHasNext()) {
		newOffsets.push_back(backInserter.data().tellPutPtr()-destDataBeginOffset);
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
	pinfo.end("Encoded words");
	dest = backInserter.data();
	
	dest.putOffset(beginOffset+3, dest.tellPutPtr()-destDataBeginOffset);
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
	return dest.tellPutPtr()-beginOffset;
}

UByteArrayAdapter::OffsetType ItemIndexFactory::compressWithHuffman(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	if (store.indexType() == ItemIndex::T_WAH) {
		return compressWithHuffmanWAH(store, dest);
	}
	else if (store.indexType() == ItemIndex::T_RLE_DE) {
		return compressWithHuffmanRLEDE(store, dest);
	}
	else {
		std::cerr << "Unsupported index format" << std::endl;
		return 0;
	}
}

UByteArrayAdapter::OffsetType ItemIndexFactory::compressWithVarUint(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
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
	dest.putOffset(beginOffset+3, dest.tellPutPtr()-destDataBeginOffset);
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	std::cout << "Offset index created. Total size: " << dest.tellPutPtr()-beginOffset;
	return dest.tellPutPtr()-beginOffset;
}

#define HEAP_ALLOC_MINI_LZO(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

UByteArrayAdapter::OffsetType ItemIndexFactory::compressWithLZO(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(2);
	dest.putUint8(store.indexType());
	dest.putUint8(Static::ItemIndexStore::IC_LZO | store.compressionType());
	dest.putOffset(0);
	UByteArrayAdapter::OffsetType destDataBeginOffset = dest.tellPutPtr();
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	newOffsets.reserve(store.size());
	
	HEAP_ALLOC_MINI_LZO(wrkmem, LZO1X_1_MEM_COMPRESS);

	
	uint32_t bufferSize = 10*1024*1024;
	uint8_t * inBuf = new uint8_t[bufferSize];
	uint8_t * outBuf = new uint8_t[2*bufferSize];

	UByteArrayAdapter::OffsetType totalOutPutBuffLen = 0;
	
	ProgressInfo pinfo;
	pinfo.begin(store.size(), "Encoding words");
	for(uint32_t i = 0; i < store.size(); ++i ) {
		newOffsets.push_back(dest.tellPutPtr()-destDataBeginOffset);
		UByteArrayAdapter idxData( store.dataAt(i) );
		if (idxData.size() > bufferSize) {
			delete[] inBuf;
			delete[] outBuf;
			bufferSize = idxData.size();
			inBuf = new uint8_t[bufferSize];
			outBuf = new uint8_t[2*bufferSize];
			
		}
		lzo_uint inBufLen = idxData.size();
		lzo_uint outBufLen = bufferSize*2;
		idxData.get(0, inBuf, inBufLen);
		int r = ::lzo1x_1_compress(inBuf, inBufLen, outBuf, &outBufLen, wrkmem);
		if (r != LZO_E_OK) {
			delete[] inBuf;
			delete[] outBuf;
			std::cerr << "Compression Error" << std::endl;
			return 0;
		}
		totalOutPutBuffLen += outBufLen;
		dest.put(outBuf, outBufLen);
		pinfo(i);
	}
	pinfo.end("Encoded words");
	
	if (totalOutPutBuffLen != dest.tellPutPtr()-destDataBeginOffset) {
		std::cout << "Compression failed" << std::endl;
		return 0;
	}
	
	std::cout << "Data section has a size of " << dest.tellPutPtr()-destDataBeginOffset;
	dest.putOffset(beginOffset+3, dest.tellPutPtr()-destDataBeginOffset);
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	std::cout << "Offset index created. Current size:" << dest.tellPutPtr()-beginOffset << std::endl;
	if (store.compressionType() == Static::ItemIndexStore::IC_HUFFMAN) {
		UByteArrayAdapter htData = store.getHuffmanTreeData();
		std::cout << "Adding huffman tree with size: " << htData.size() << std::endl;
		dest.put(htData);
	}
	std::cout << "Total size: " << dest.tellPutPtr()-beginOffset << std::endl;
	return dest.tellPutPtr()-beginOffset;
}

}//end namespace
