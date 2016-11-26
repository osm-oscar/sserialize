#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/debuggerfunctions.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/containers/HuffmanTree.h>
#include <sserialize/iterator/MultiBitBackInserter.h>
#include <vendor/libs/minilzo/minilzo.h>
#include <unordered_map>
#include <iostream>
#include <sstream>

namespace sserialize {

ItemIndexFactory::ItemIndexFactory(bool memoryBased) :
m_dataOffsets(sserialize::MM_SHARED_MEMORY),
m_idToOffsets(sserialize::MM_SLOW_FILEBASED),
m_idxSizes(sserialize::MM_SLOW_FILEBASED),
m_hitCount(0),
m_checkIndex(true),
m_useDeduplication(true),
m_type(ItemIndex::T_RLE_DE)
{
	if (memoryBased)
		 setIndexFile( UByteArrayAdapter(new std::vector<uint8_t>(), true) );
	else
		setIndexFile( UByteArrayAdapter::createCache(8*1024*1024, sserialize::MM_FILEBASED) );
}

ItemIndexFactory::ItemIndexFactory(ItemIndexFactory && other) :
m_dataOffsets(sserialize::MM_SHARED_MEMORY),
m_idToOffsets(sserialize::MM_SLOW_FILEBASED),
m_idxSizes(sserialize::MM_SLOW_FILEBASED),
m_hitCount(other.m_hitCount.load()),
m_checkIndex(other.m_checkIndex),
m_useDeduplication(other.m_useDeduplication),
m_type(other.m_type),
m_compressionType(other.m_compressionType)
{
	m_header = std::move(other.m_header);
	m_indexStore = std::move(other.m_indexStore);
	m_hash = std::move(other.m_hash);
	m_dataOffsets = std::move(other.m_dataOffsets);
	m_idToOffsets = std::move(other.m_idToOffsets);
	m_idxSizes = std::move(other.m_idxSizes);
	//default init read-write-lock
}

ItemIndexFactory::~ItemIndexFactory() {}

ItemIndexFactory & ItemIndexFactory::operator=(ItemIndexFactory && other) {
	m_hitCount.store(other.m_hitCount.load());
	m_checkIndex = other.m_checkIndex;
	m_useDeduplication = other.m_useDeduplication;
	m_type = other.m_type;
	m_compressionType = other.m_compressionType;
	m_header = std::move(other.m_header);
	m_indexStore = std::move(other.m_indexStore);
	m_hash = std::move(other.m_hash);
	m_dataOffsets = std::move(other.m_dataOffsets);
	m_idToOffsets = std::move(other.m_idToOffsets);
	m_idxSizes = std::move(other.m_idxSizes);
	//default init read-write-lock
	return *this;
}

UByteArrayAdapter ItemIndexFactory::at(sserialize::OffsetType offset) const {
	return m_indexStore+offset;
}

void ItemIndexFactory::setIndexFile(sserialize::UByteArrayAdapter data) {
	if (size()) { //clear eversthing
		m_hitCount = 0;
		m_hash.clear();
		m_dataOffsets.clear();
		m_idToOffsets.clear();
		m_idxSizes.clear();
	}

	m_header = data;
	m_header.putUint8(0); // dummy version
	m_header.putUint8(0); //dumy index type
	m_header.putUint8(0); //dummy compression type
	m_header.putOffset(0); //dummy offset
	m_indexStore = m_header;
	m_indexStore.shrinkToPutPtr();
	m_header.resetPtrs();
	addIndex(std::set<uint32_t>());
}

std::vector<uint32_t> ItemIndexFactory::insert(const sserialize::Static::ItemIndexStore& store) {
	m_idToOffsets.reserve(store.size()+size());
	m_idxSizes.reserve(store.size()+size());
	if (m_useDeduplication) {
		m_dataOffsets.reserve(store.size()+m_dataOffsets.size());
		m_hash.reserve(store.size()+size());
	}
	std::vector<uint32_t> res;
	res.reserve(store.size());
	res.push_back(0);
	std::vector<uint32_t> tmp; 
	ItemIndex tmpIdx;
	sserialize::ProgressInfo info;
	info.begin(store.size(), "Adding indices from store");
	for(uint32_t i = 1, s = store.size(); i < s; ++i) {//skip the first index as that one is by definition 0
		tmp.clear();
		tmpIdx = store.at(i);
		tmpIdx.putInto(tmp);
		res.push_back(addIndex(tmp));
		info(i);
	}
	info.end();
	return res;
}

uint64_t ItemIndexFactory::hashFunc(const UByteArrayAdapter & v) {
	uint64_t h = 0;
	UByteArrayAdapter::MemoryView mv(v.asMemView());
	for(UByteArrayAdapter::MemoryView::const_iterator it(mv.cbegin()), end(mv.cend()); it != end; ++it) {
		hash_combine(h, (const char)*it);
	}
	return h;
}

uint64_t ItemIndexFactory::hashFunc(const std::vector<uint8_t> & v) {
	uint64_t h = 0;
	for(std::vector<uint8_t>::const_iterator it(v.begin()), end(v.end()); it != end; ++it) {
		hash_combine(h, (const char)*it);
	}
	return h;
}

int64_t ItemIndexFactory::getIndex(const std::vector<uint8_t> & v, uint64_t & hv) {
	if (v.size() == 0)
		return -1;
	hv = hashFunc(v);
	m_mapLock.acquireReadLock();
	if (m_hash.count(hv) == 0) {
		m_mapLock.releaseReadLock();
		return -1;
	}
	else {
		DataOffsetEntry doe = m_dataOffsets.at(m_hash[hv]);
		m_mapLock.releaseReadLock();
		while (true) {
			if (indexInStore(v, doe.id)) {
				return doe.id;
			}
			if (doe.prev) {
				m_mapLock.acquireReadLock();
				doe =  m_dataOffsets.at(doe.prev);
				m_mapLock.releaseReadLock();
			}
			else {
				break;
			}
		}
	}
	return -1;
}

bool ItemIndexFactory::indexInStore(const std::vector< uint8_t >& v, uint32_t id) {
	m_dataLock.acquireReadLock();
	sserialize::UByteArrayAdapter::OffsetType offset = m_idToOffsets.at(id);
	if (v.size() > (m_indexStore.tellPutPtr()-offset)) {
		m_dataLock.releaseReadLock();
		return false;
	}
	UByteArrayAdapter::MemoryView mv( m_indexStore.getMemView(offset, v.size()) );
	bool eq = memcmp(mv.get(), v.data(), v.size()) == 0;
	m_dataLock.releaseReadLock();
	return eq;
}

uint32_t ItemIndexFactory::addIndex(const ItemIndex & idx) {
	std::vector<uint32_t> tmp;
	idx.putInto(tmp);
	return addIndex(tmp);
}

Static::ItemIndexStore ItemIndexFactory::asItemIndexStore() {
	return sserialize::Static::ItemIndexStore( new sserialize::detail::ItemIndexStoreFromFactory(this) );
}

uint32_t ItemIndexFactory::addIndex(const std::vector<uint8_t> & idx, uint32_t idxSize) {
	uint64_t hv;
	int64_t id = -1;
	if (m_useDeduplication) {
		id = getIndex(idx, hv);
	}
	if (id < 0) {
		m_dataLock.acquireWriteLock();
		sserialize::UByteArrayAdapter::OffsetType dataOffset = m_indexStore.tellPutPtr();
		m_indexStore.putData(idx);
		id = m_idToOffsets.size();
		SSERIALIZE_CHEAP_ASSERT_EQUAL((std::size_t)id, m_idToOffsets.size()); //check for wrap around of ids
		m_idToOffsets.push_back(dataOffset);
		m_idxSizes.push_back(idxSize);
		m_dataLock.releaseWriteLock();
		
		if (m_useDeduplication) {
			m_mapLock.acquireWriteLock();
			uint64_t prevElement = 0;
			if (m_hash.count(hv)) {
				prevElement = m_hash[hv];
			}
			m_dataOffsets.push_back(DataOffsetEntry({ .prev = prevElement, .id = (uint32_t)id}));
			m_hash[hv] = m_dataOffsets.size()-1;
			m_mapLock.releaseWriteLock();
		}
	}
	else {//index is in store, so we have deduplication enabled
		++m_hitCount;
	}
	return narrow_check<uint32_t>(id);
}

UByteArrayAdapter ItemIndexFactory::getFlushedData() {
	UByteArrayAdapter fd = m_header;
	fd.growStorage(m_indexStore.tellPutPtr());
	return fd;
}

OffsetType ItemIndexFactory::flush() {
	SSERIALIZE_CHEAP_ASSERT_EQUAL(m_idxSizes.size(), m_idToOffsets.size());
	std::cout << "Serializing index with type=" << m_type << std::endl;
	std::cout << "Hit count was " << m_hitCount.load() << std::endl;
	std::cout << "Size=" << m_idToOffsets.size() << std::endl;
	m_header.resetPtrs();
	m_header << static_cast<uint8_t>(4); //Version
	m_header << static_cast<uint8_t>(m_type);//type
	m_header << static_cast<uint8_t>(Static::ItemIndexStore::IndexCompressionType::IC_NONE);
	m_header.putOffset(m_indexStore.tellPutPtr());
	

	uint64_t oIBegin = m_indexStore.tellPutPtr();
	std::cout << "Serializing offsets starting at " << oIBegin << "...";
	if (! Static::SortedOffsetIndexPrivate::create(m_idToOffsets, m_indexStore) ) {
		std::cout << "ItemIndexFactory::serialize: failed to create Offsetindex." << std::endl;
		return 0;
	}
	else {
		UByteArrayAdapter oIData(m_indexStore, oIBegin);
		sserialize::Static::SortedOffsetIndex oIndex(oIData);
		if (m_idToOffsets != oIndex) {
			std::cout << "OffsetIndex creation FAILED!" << std::endl;
		}
	}
	uint64_t idxSizesBegin = m_indexStore.tellPutPtr();
	std::cout << "Serializing idx sizes starting at " << idxSizesBegin << "..." << std::flush;
#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(m_idxSizes[0], (ItemIndexSizesContainer::value_type)0);
	if (m_useDeduplication) {
		for(uint32_t i(1), s(m_idxSizes.size()); i < s; ++i) {
			SSERIALIZE_EXPENSIVE_ASSERT_LARGER(m_idxSizes[i],(ItemIndexSizesContainer::value_type)0);
		}
	}
#endif
	m_indexStore << m_idxSizes;
	std::cout << std::endl;
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
	
	if (ht.depth() > 32) {
		std::cerr << "HuffmanTree depth > 32" << std::endl;
		return 0;
	}
	
	std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());
	
	
	//now recompress
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(3);
	dest.putUint8(ItemIndex::T_RLE_DE);
	dest.putUint8(Static::ItemIndexStore::IndexCompressionType::IC_HUFFMAN);
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
	
	if (ht.depth() > 32) {
		std::cerr << "HuffmanTree depth > 32" << std::endl;
		return 0;
	}
	
	std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());	
	
	//now recompress
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(4); //version
	dest.putUint8(ItemIndex::T_WAH);
	dest.putUint8(Static::ItemIndexStore::IndexCompressionType::IC_HUFFMAN);
	dest.putOffset(0);
	UByteArrayAdapter::OffsetType destDataBeginOffset = dest.tellPutPtr();
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	newOffsets.reserve(store.size());
	
	MultiBitBackInserter backInserter(dest);
	data.resetGetPtr();
	ProgressInfo pinfo;
	pinfo.begin(data.size(), "Encoding indices");
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
	if (store.compressionType() != Static::ItemIndexStore::IndexCompressionType::IC_NONE) {
		std::cerr << "Unsupported compression format detected" << std::endl;
		return 0;
	}

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
	
	if (store.compressionType() != Static::ItemIndexStore::IndexCompressionType::IC_NONE) {
		std::cerr << "Unsupported compression format detected" << std::endl;
		return 0;
	}
	
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(3);
	dest.putUint8(ItemIndex::T_WAH);
	dest.putUint8(Static::ItemIndexStore::IndexCompressionType::IC_VARUINT32);
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
	dest.putUint8(4);//version
	dest.putUint8(store.indexType());
	dest.putUint8(Static::ItemIndexStore::IndexCompressionType::IC_LZO | store.compressionType());
	dest.putOffset(0);
	UByteArrayAdapter::OffsetType destDataBeginOffset = dest.tellPutPtr();
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	std::vector<uint32_t> uncompressedSizes;
	newOffsets.reserve(store.size());
	
	HEAP_ALLOC_MINI_LZO(wrkmem, LZO1X_1_MEM_COMPRESS);

	
	uint32_t bufferSize = 10*1024*1024;
	uint8_t * outBuf = new uint8_t[2*bufferSize];

	UByteArrayAdapter::OffsetType totalOutPutBuffLen = 0;
	
	ProgressInfo pinfo;
	pinfo.begin(store.size(), "Recompressing index with lzo");
	for(uint32_t i = 0; i < store.size(); ++i ) {
		newOffsets.push_back(dest.tellPutPtr()-destDataBeginOffset);
		UByteArrayAdapter::MemoryView idxData( store.rawDataAt(i).asMemView() );
		uncompressedSizes.push_back(narrow_check<uint32_t>(idxData.size()));
		if (idxData.size() > bufferSize) {
			delete[] outBuf;
			bufferSize = (uint32_t) idxData.size();
			outBuf = new uint8_t[2*bufferSize];
			
		}
		lzo_uint outBufLen = bufferSize*2;
		int r = ::lzo1x_1_compress(idxData.get(), idxData.size(), outBuf, &outBufLen, wrkmem);
		if (r != LZO_E_OK) {
			delete[] outBuf;
			std::cerr << "Compression Error" << std::endl;
			return 0;
		}
		totalOutPutBuffLen += outBufLen;
		dest.putData(outBuf, outBufLen);
		pinfo(i);
	}
	pinfo.end();
	
	if (totalOutPutBuffLen != dest.tellPutPtr()-destDataBeginOffset) {
		std::cout << "Compression failed" << std::endl;
		return 0;
	}
	
	std::cout << "Data section has a size of " << dest.tellPutPtr()-destDataBeginOffset << std::endl;
	dest.putOffset(beginOffset+3, dest.tellPutPtr()-destDataBeginOffset);
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	std::cout << "Offset index created. Current size:" << dest.tellPutPtr()-beginOffset << std::endl;
	
	//add the index sizes table
	{
		sserialize::Static::ArrayCreator<uint32_t> ac(dest);
		ac.reserveOffsets(store.size());
		for(uint32_t i(0), s(store.size()); i < s; ++i) {
			ac.put(store.idxSize(i));
		}
		ac.flush();
	}
	
	if (store.compressionType() == Static::ItemIndexStore::IndexCompressionType::IC_HUFFMAN) {
		UByteArrayAdapter htData = store.getHuffmanTreeData();
		std::cout << "Adding huffman tree with size: " << htData.size() << std::endl;
		dest.putData(htData);
	}
	//now add the table with the uncompressedSizes
	UByteArrayAdapter bitneed = dest;
	bitneed.shrinkToPutPtr();
	dest.putUint8(0);
	uint8_t bitsForUncompressedSizes = CompactUintArray::create(uncompressedSizes, dest);
	if (!bitsForUncompressedSizes) {
		std::cout << "Failed to create the index for the uncompressed sizes" << std::endl;
		return 0;
	}
	bitneed.putUint8(bitsForUncompressedSizes);
	std::cout << "Total size: " << dest.tellPutPtr()-beginOffset << std::endl;
	return dest.tellPutPtr()-beginOffset;
}

namespace detail {

OffsetType ItemIndexStoreFromFactory::getSizeInBytes() const {
	throw sserialize::UnimplementedFunctionException("ItemIndexStoreFromFactory::getSizeInBytes");
	return 0;
}

uint32_t ItemIndexStoreFromFactory::size() const {
	return m_idxFactory->size();
}

ItemIndex::Types ItemIndexStoreFromFactory::indexType() const {
	return m_idxFactory->type();
}

uint32_t ItemIndexStoreFromFactory::compressionType() const {
	return m_idxFactory->compressionType();
}

UByteArrayAdapter::OffsetType ItemIndexStoreFromFactory::dataSize(uint32_t pos) const {
	return at(pos).getSizeInBytes();
}

UByteArrayAdapter ItemIndexStoreFromFactory::rawDataAt(uint32_t /*pos*/) const {
	throw sserialize::UnimplementedFunctionException("ItemIndexStoreFromFactory::rawDataAt");
	return UByteArrayAdapter();
}

ItemIndex ItemIndexStoreFromFactory::at(uint32_t pos) const {
	return m_idxFactory->indexById(pos);
}

uint32_t ItemIndexStoreFromFactory::idxSize(uint32_t pos) const {
	return m_idxFactory->idxSize(pos);
}

std::ostream& ItemIndexStoreFromFactory::printStats(std::ostream& out) const {
	return out;
}

std::ostream& ItemIndexStoreFromFactory::printStats(std::ostream& out, const std::unordered_set<uint32_t> & /*indexIds*/) const {
	return out;
}

sserialize::Static::SortedOffsetIndex & ItemIndexStoreFromFactory::getIndex() {
	throw sserialize::UnimplementedFunctionException("ItemIndexStoreFromFactory::getIndex");
	return m_dummyOffsets;
}

const UByteArrayAdapter & ItemIndexStoreFromFactory::getData() const {
	throw sserialize::UnimplementedFunctionException("ItemIndexStoreFromFactory::getData");
	return m_idxFactory->getIndexStore();
}

RCPtrWrapper<Static::HuffmanDecoder> ItemIndexStoreFromFactory::getHuffmanTree() const {
	throw sserialize::UnimplementedFunctionException("ItemIndexStoreFromFactory::getHuffmanTree");
	return RCPtrWrapper<Static::HuffmanDecoder>();
}

UByteArrayAdapter ItemIndexStoreFromFactory::getHuffmanTreeData() const {
	throw sserialize::UnimplementedFunctionException("ItemIndexStoreFromFactory::getHuffmanTreeData");
	return UByteArrayAdapter();
}

}//end namespace detail

}//end namespace
