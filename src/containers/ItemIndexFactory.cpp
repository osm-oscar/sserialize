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
#include <sserialize/iterator/RangeGenerator.h>
#include <sserialize/iterator/TransformIterator.h>
#include <sserialize/mt/ThreadPool.h>
#include <minilzo/minilzo.h>
#include <unordered_map>
#include <iostream>
#include <sstream>

namespace sserialize {

ItemIndexFactory::ItemIndexFactory(bool memoryBased) :
m_dataOffset(0),
m_idCounter(0),
m_idToOffsets(sserialize::MM_SLOW_FILEBASED),
m_idxSizes(sserialize::MM_SLOW_FILEBASED),
m_idxTypes(sserialize::MM_SLOW_FILEBASED),
m_hitCount(0),
m_checkIndex(true),
m_useDeduplication(true),
m_type(ItemIndex::T_RLE_DE),
m_compressionType(Static::ItemIndexStore::IC_NONE),
m_growSize(16*1024*1024),
m_auxDataGrow(4096)
{
	if (memoryBased)
		setIndexFile( UByteArrayAdapter(new std::vector<uint8_t>(), true) );
	else
		setIndexFile( UByteArrayAdapter::createCache(8*1024*1024, sserialize::MM_FILEBASED) );
}

ItemIndexFactory::ItemIndexFactory(ItemIndexFactory && other) :
m_dataOffset(other.m_dataOffset),
m_idCounter(other.m_idCounter),
m_idToOffsets(sserialize::MM_SLOW_FILEBASED),
m_idxSizes(sserialize::MM_SLOW_FILEBASED),
m_hitCount(other.m_hitCount.load()),
m_checkIndex(other.m_checkIndex),
m_useDeduplication(other.m_useDeduplication),
m_type(other.m_type),
m_compressionType(other.m_compressionType),
m_growSize(other.m_growSize),
m_auxDataGrow(other.m_auxDataGrow)
{
	m_header = std::move(other.m_header);
	m_indexStore = std::move(other.m_indexStore);
	m_hash = std::move(other.m_hash);
	m_idToOffsets = std::move(other.m_idToOffsets);
	m_idxSizes = std::move(other.m_idxSizes);
	m_idxTypes = std::move(other.m_idxTypes);
	//default init read-write-lock
}

ItemIndexFactory::~ItemIndexFactory() {}

ItemIndexFactory & ItemIndexFactory::operator=(ItemIndexFactory && other) {
	m_dataOffset = other.m_dataOffset;
	m_idCounter = other.m_idCounter;
	m_hitCount.store(other.m_hitCount.load());
	m_checkIndex = other.m_checkIndex;
	m_useDeduplication = other.m_useDeduplication;
	m_type = other.m_type;
	m_compressionType = other.m_compressionType;
	m_header = std::move(other.m_header);
	m_indexStore = std::move(other.m_indexStore);
	m_hash = std::move(other.m_hash);
	m_idToOffsets = std::move(other.m_idToOffsets);
	m_idxSizes = std::move(other.m_idxSizes);
	m_idxTypes = std::move(other.m_idxTypes);
	m_growSize = other.m_growSize;
	m_auxDataGrow = other.m_auxDataGrow;
	//default init read-write-lock
	return *this;
}

int ItemIndexFactory::types() const {
	return m_type;
}

ItemIndex::Types ItemIndexFactory::type(uint32_t pos) const {
	if (m_type & sserialize::ItemIndex::T_MULTIPLE) {
		return ItemIndex::Types(m_idxTypes.at(pos));
	}
	return ItemIndex::Types(m_type);
}

UByteArrayAdapter ItemIndexFactory::at(sserialize::OffsetType offset) const {
	return m_indexStore+offset;
}

void ItemIndexFactory::setType(int type) {
	m_type = type;
}

void ItemIndexFactory::setIndexFile(sserialize::UByteArrayAdapter data) {
	if (size()) { //clear everything
		m_dataOffset = 0;
		m_hitCount = 0;
		m_hash.clear();
		m_idCounter = 0;
		m_idToOffsets.clear();
		m_idxSizes.clear();
		m_idxTypes.clear();
	}

	m_header = data;
	m_header.putUint8(0); // dummy version
	m_header.putUint16(0); //dumy index type
	m_header.putUint8(0); //dummy compression type
	m_header.putOffset(0); //dummy offset
	m_indexStore = m_header;
	m_indexStore.shrinkToPutPtr();
	m_header.resize(m_header.tellPutPtr());
	m_header.resetPtrs();
	addIndex(std::set<uint32_t>());
}

std::vector<uint32_t> ItemIndexFactory::insert(const sserialize::Static::ItemIndexStore& store, uint32_t threadCount) {
	m_idToOffsets.reserve(store.size()+size());
	m_idxSizes.reserve(store.size()+size());
	if (m_useDeduplication) {
		m_hash.reserve(store.size()+size());
	}
	struct State {
		sserialize::Static::ItemIndexStore const & src;
		sserialize::ItemIndexFactory & dest;
		sserialize::ProgressInfo pinfo;
		std::atomic<uint64_t> i{1};
		std::vector<uint32_t> oldToNewId;
		State(sserialize::Static::ItemIndexStore const & src, sserialize::ItemIndexFactory & dest) :
		src(src),
		dest(dest),
		oldToNewId(src.size(), 0)
		{}
	};
	struct Worker {
		State * state;
		std::vector<uint32_t> tmp;
		Worker(State * state) : state(state) {}
		Worker(Worker const & other) : state(other.state) {}
		void operator()() {
			while(true) {
				auto i = state->i.fetch_add(1, std::memory_order_relaxed);
				if (i >= state->src.size()) {
					return;
				}
				state->src.at(i).putInto(tmp);
				state->oldToNewId[i] = state->dest.addIndex(tmp);
				tmp.clear();
				state->pinfo(state->i.load(std::memory_order_relaxed));
			}
		}
	};
	State state(store, *this);
	state.pinfo.begin(store.size());
	sserialize::ThreadPool::execute(Worker(&state), threadCount, sserialize::ThreadPool::CopyTaskTag());
	state.pinfo.end();
	return state.oldToNewId;
}

ItemIndexFactory::DataHashKey ItemIndexFactory::hashFunc(const UByteArrayAdapter & v) {
	UByteArrayAdapter::MemoryView mv(v.asMemView());
	sserialize::ShaHasher<UByteArrayAdapter::MemoryView> hasher;
	return hasher(mv);
}

ItemIndexFactory::DataHashKey ItemIndexFactory::hashFunc(const std::vector<uint8_t> & v) {
	sserialize::ShaHasher< std::vector<uint8_t> > hasher;
	return hasher(v);
}

uint32_t ItemIndexFactory::addIndex(const ItemIndex & idx) {
	std::vector<uint32_t> tmp;
	idx.putInto(tmp);
	return addIndex(tmp);
}

Static::ItemIndexStore ItemIndexFactory::asItemIndexStore() {
	return sserialize::Static::ItemIndexStore( new sserialize::detail::ItemIndexStoreFromFactory(this) );
}

uint32_t ItemIndexFactory::addIndex(const std::vector<uint8_t> & idx, uint32_t idxSize, ItemIndex::Types type) {
	SSERIALIZE_CHEAP_ASSERT(type & m_type);
	sserialize::UByteArrayAdapter::OffsetType dataOffset;
	IndexId indexId;
	if (m_useDeduplication) {
		ItemIndexFactory::DataHashKey hv = hashFunc(idx);
		std::lock_guard<std::mutex> metaDataLock(m_metaDataLock);
		IndexId & tmpId = m_hash[hv];
		if (tmpId.valid()) {
			m_hitCount.fetch_add(1, std::memory_order_relaxed);
			return tmpId;
		}
		else {
			tmpId = m_idCounter; //sets value in m_hash
			m_idCounter += 1;
			indexId = tmpId;
			
			dataOffset = m_dataOffset;
			m_dataOffset += idx.size();
		}
	}
	else {
		std::lock_guard<std::mutex> metaDataLock(m_metaDataLock);
		indexId = m_idCounter;
		m_idCounter += 1;
		dataOffset = m_dataOffset;
		m_dataOffset += idx.size();
	}
	//check if we need to grow the data
	if (dataOffset+idx.size() >= m_indexStore.size()) {
		std::lock_guard<std::shared_mutex> dataGrowLock(m_dataGrowLock);
		//check again since the size may have changed already
		if (dataOffset+idx.size() >= m_indexStore.size()) {
			UByteArrayAdapter::SizeType needSize = dataOffset+idx.size() - m_indexStore.size();
			m_indexStore.growStorage(std::max(needSize, m_growSize));
		}
	}
	//push the data
	{
		std::shared_lock<std::shared_mutex> dataGrowLock(m_dataGrowLock);
		m_indexStore.putData(dataOffset, idx);
	}
	//adjust aux data
	{
		std::lock_guard<std::mutex> auxDataLock(m_auxDataLock);
		if (m_idToOffsets.size() <= indexId) {
			m_idToOffsets.resize(indexId+m_auxDataGrow);
			m_idxSizes.resize(indexId+m_auxDataGrow);
			if (m_type & ItemIndex::T_MULTIPLE) {
				m_idxTypes.resize(indexId+m_auxDataGrow);
			}
		}
		m_idToOffsets.at(indexId) = dataOffset;
		m_idxSizes.at(indexId) = idxSize;
		if (m_type & ItemIndex::T_MULTIPLE) {
			m_idxTypes.at(indexId) = type;
		}
	}
	return indexId;
}

void ItemIndexFactory::recalculateDeduplicationData() {
	std::lock_guard<std::mutex> metaDataLock(m_metaDataLock);
	std::shared_lock<std::shared_mutex> dataGrowLock(m_dataGrowLock);
	m_hash.clear();
	for(uint32_t id(0), s(size()); id < s; ++id) {
		auto hv = hashFunc( this->indexDataById(id) );
		SSERIALIZE_NORMAL_ASSERT(m_hash.count(hv) == 0);
		m_hash[hv] = id;
	}
}

UByteArrayAdapter ItemIndexFactory::getFlushedData() {
	UByteArrayAdapter fd = m_header;
	fd.growStorage(m_indexStore.tellPutPtr());
	return fd;
}

OffsetType ItemIndexFactory::flush() {
	SSERIALIZE_CHEAP_ASSERT_EQUAL(m_idxSizes.size(), m_idToOffsets.size());
	
	//adjust aux data and index storage to correct values
	m_indexStore.setPutPtr(m_dataOffset);
	m_idxSizes.resize(m_idCounter);
	m_idToOffsets.resize(m_idCounter);
	if (m_type & ItemIndex::T_MULTIPLE) {
		m_idxTypes.resize(m_idCounter);
	}
	std::cout << "Serializing index with type=" << m_type << std::endl;
	std::cout << "Hit count was " << m_hitCount.load() << std::endl;
	std::cout << "Size=" << m_idToOffsets.size() << std::endl;
	m_header.resetPtrs();
	m_header.putUint8(5); //Version
	m_header.putUint16(m_type);//type
	m_header.putUint8(Static::ItemIndexStore::IndexCompressionType::IC_NONE);
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
	if (m_type & ItemIndex::T_MULTIPLE) {
		uint32_t bits = sserialize::msb( sserialize::msb( uint32_t(m_type - ItemIndex::T_MULTIPLE) ) ) + 1;
		auto tf = [](uint8_t v) {return sserialize::msb(v); };
		using MyIterator = sserialize::TransformIterator<decltype(tf), uint32_t, ItemIndexTypesContainer::const_iterator>;
		CompactUintArray::create(MyIterator(tf, m_idxTypes.begin()), MyIterator(tf, m_idxTypes.end()), m_indexStore, bits);
	}
	std::cout << std::endl;
	std::cout << "done." << std::endl;

	return 4+UByteArrayAdapter::OffsetTypeSerializedLength()+m_indexStore.tellPutPtr();
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
	if (store.indexTypes() == ItemIndex::T_WAH) {
		charSize = 4;
		for(UByteArrayAdapter::OffsetType i = 0; i < size; i += charSize) {
			uint32_t character = data.getUint32(i);
			incAlphabet(alphabet, character);
		}
	}
	else if (store.indexTypes() == ItemIndex::T_RLE_DE) {
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
		throw sserialize::CreationException("HuffmanTree depth > 32");
	}
	
	std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());
	
	
	//now recompress
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(6);
	dest.putUint16(ItemIndex::T_RLE_DE);
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
	
	dest.putOffset(beginOffset+4, dest.tellPutPtr()-destDataBeginOffset);
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	//add the index sizes table
	{
		sserialize::Static::ArrayCreator<uint32_t> ac(dest);
		ac.reserveOffsets(store.size());
		for(uint32_t i(0), s(store.size()); i < s; ++i) {
			ac.put(store.idxSize(i));
		}
		ac.flush();
	}
	//now comes the decoding table
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
		throw sserialize::CreationException("HuffmanTree depth > 32");
	}
	
	std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());	
	
	//now recompress
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(6); //version
	dest.putUint16(ItemIndex::T_WAH);
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
	
	dest.putOffset(beginOffset+4, dest.tellPutPtr()-destDataBeginOffset);
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	//add the index sizes table
	{
		sserialize::Static::ArrayCreator<uint32_t> ac(dest);
		ac.reserveOffsets(store.size());
		for(uint32_t i(0), s(store.size()); i < s; ++i) {
			ac.put(store.idxSize(i));
		}
		ac.flush();
	}
	//now comes the decoding table
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

	if (store.indexTypes() == ItemIndex::T_WAH) {
		return compressWithHuffmanWAH(store, dest);
	}
	else if (store.indexTypes() == ItemIndex::T_RLE_DE) {
		return compressWithHuffmanRLEDE(store, dest);
	}
	else {
		throw sserialize::UnsupportedFeatureException("Unsupported index type: " + to_string(sserialize::ItemIndex::Types(store.indexTypes())));
		return 0;
	}
}

UByteArrayAdapter::OffsetType ItemIndexFactory::compressWithVarUint(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	if (store.indexTypes() != ItemIndex::T_WAH) {
		std::cerr << "Unsupported index format" << std::endl;
		return 0;
	}
	
	if (store.compressionType() != Static::ItemIndexStore::IndexCompressionType::IC_NONE) {
		std::cerr << "Unsupported compression format detected" << std::endl;
		return 0;
	}
	
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(6);
	dest.putUint16(ItemIndex::T_WAH);
	dest.putUint8(Static::ItemIndexStore::IndexCompressionType::IC_VARUINT32);
	dest.putOffset(0);
	UByteArrayAdapter::OffsetType destDataBeginOffset = dest.tellPutPtr();
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	std::vector<uint32_t> idxSizes;
	newOffsets.reserve(store.size());
	
	ProgressInfo pinfo;
	pinfo.begin(store.size(), "Encoding indices");
	for(std::size_t i(0), s(store.size()); i < s; ++i) {
		sserialize::breakHereIf(i == 1418);
		newOffsets.push_back(dest.tellPutPtr()-destDataBeginOffset);
		UByteArrayAdapter idxData = store.rawDataAt(i);
		uint32_t indexSize = idxData.getUint32();
		uint32_t indexCount = idxData.getUint32();
		
		SSERIALIZE_NORMAL_ASSERT_EQUAL(indexSize, store.rawDataAt(i).getUint32(0));
		SSERIALIZE_NORMAL_ASSERT_EQUAL(indexCount, store.rawDataAt(i).getUint32(4));
		SSERIALIZE_NORMAL_ASSERT_EQUAL(indexSize+8, store.rawDataAt(i).size());
		
		dest.putVlPackedUint32(indexSize);
		dest.putVlPackedUint32(indexCount);
		indexSize = indexSize / 4;
		for(uint32_t i = 0; i < indexSize; ++i) {
			uint32_t src = idxData.getUint32();
			dest.putVlPackedUint32(src);
		}
		pinfo(i);
	}
	pinfo.end("Encoded indices");
	SSERIALIZE_CHEAP_ASSERT_EQUAL(store.size(), newOffsets.size());
	dest.putOffset(beginOffset+4, dest.tellPutPtr()-destDataBeginOffset);
	std::cout << "Creating offset index" << std::endl;
	sserialize::Static::SortedOffsetIndexPrivate::create(newOffsets, dest);
	std::cout << "Offset index created. Current size: " << dest.tellPutPtr()-beginOffset;
	//add the index sizes table
	{
		sserialize::Static::ArrayCreator<uint32_t> ac(dest);
		ac.reserveOffsets(store.size());
		for(uint32_t i(0), s(store.size()); i < s; ++i) {
			ac.put(store.idxSize(i));
		}
		ac.flush();
	}
	std::cout << "Total size: " << dest.tellPutPtr()-beginOffset;
	return dest.tellPutPtr()-beginOffset;
}

#define HEAP_ALLOC_MINI_LZO(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

UByteArrayAdapter::OffsetType ItemIndexFactory::compressWithLZO(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest) {
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(6);//version
	dest.putUint16(store.indexTypes());
	dest.putUint8(Static::ItemIndexStore::IndexCompressionType::IC_LZO | store.compressionType());
	dest.putOffset(0);
	UByteArrayAdapter::OffsetType destDataBeginOffset = dest.tellPutPtr();
	std::vector<UByteArrayAdapter::OffsetType> newOffsets;
	std::vector<uint32_t> uncompressedSizes;
	newOffsets.reserve(store.size());
	
	HEAP_ALLOC_MINI_LZO(wrkmem, LZO1X_1_MEM_COMPRESS);

	
	std::vector<uint8_t> outBuf;

	UByteArrayAdapter::OffsetType totalOutPutBuffLen = 0;
	
	ProgressInfo pinfo;
	pinfo.begin(store.size(), "Recompressing index with lzo");
	for(uint32_t i = 0; i < store.size(); ++i ) {
		newOffsets.push_back(dest.tellPutPtr()-destDataBeginOffset);
		UByteArrayAdapter::MemoryView idxData( store.rawDataAt(i).asMemView() );
		uncompressedSizes.push_back(narrow_check<uint32_t>(idxData.size()));
		outBuf.resize(2*idxData.size()+512); //lzo doesn't do any kind of bounds checking on the buffer size?
		lzo_uint outBufLen = outBuf.size();
		int r = ::lzo1x_1_compress(idxData.get(), idxData.size(), outBuf.data(), &outBufLen, wrkmem);
		if (r != LZO_E_OK) {
			std::stringstream ss;
			ss << "lzo1x_1_compress returned error " << r << " for index " << i;
			throw sserialize::CreationException(ss.str());
			return 0;
		}
		totalOutPutBuffLen += outBufLen;
		dest.putData(outBuf.data(), outBufLen);
		pinfo(i);
	}
	pinfo.end();
	
	if (totalOutPutBuffLen != dest.tellPutPtr()-destDataBeginOffset) {
		std::stringstream ss;
		ss << "totalOutPutBuffLen != dest.tellPutPtr()-destDataBeginOffset with ";
		ss << "totalOutPutBuffLen=" << totalOutPutBuffLen;
		ss << ", dest.tellPutPtr()=" << dest.tellPutPtr();
		ss << ", destDataBeginOffset=" << destDataBeginOffset;
		throw sserialize::CreationException(ss.str());
		return 0;
	}
	
	std::cout << "Data section has a size of " << dest.tellPutPtr()-destDataBeginOffset << std::endl;
	dest.putOffset(beginOffset+4, dest.tellPutPtr()-destDataBeginOffset);
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
	BoundedCompactUintArray::create(uncompressedSizes, dest);
	//finally the index type info
	if (store.indexTypes() & ItemIndex::T_MULTIPLE) {
		auto deref = [&store](uint32_t pos) -> uint32_t {
			return sserialize::msb( uint32_t(store.indexType(pos)) );
		};
		using MyIterator = sserialize::TransformIterator<decltype(deref), uint32_t, sserialize::RangeGenerator<uint32_t>::const_iterator>;
		uint32_t bits = sserialize::msb(sserialize::msb(uint32_t(store.indexTypes() - ItemIndex::T_MULTIPLE))) + 1;
		CompactUintArray::create(MyIterator(deref, 0), MyIterator(deref, store.size()), dest, bits);
	}
	std::cout << "Total size: " << dest.tellPutPtr()-beginOffset << std::endl;
	return dest.tellPutPtr()-beginOffset;
}

ItemIndex ItemIndexFactory::range(uint32_t begin, uint32_t end, uint32_t step, int type) {
	sserialize::RangeGenerator<uint32_t> rg(begin, end, step);
	return create(rg, type);
}

namespace detail {

OffsetType ItemIndexStoreFromFactory::getSizeInBytes() const {
	throw sserialize::UnimplementedFunctionException("ItemIndexStoreFromFactory::getSizeInBytes");
	return 0;
}

uint32_t ItemIndexStoreFromFactory::size() const {
	return m_idxFactory->size();
}

int ItemIndexStoreFromFactory::indexTypes() const {
	return m_idxFactory->types();
}

ItemIndex::Types ItemIndexStoreFromFactory::indexType(uint32_t pos) const {
	return m_idxFactory->type(pos);
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

std::ostream& ItemIndexStoreFromFactory::printStats(std::ostream& out, std::function<bool(uint32_t)>) const {
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
