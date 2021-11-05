#include <sserialize/storage/CompressedMmappedFile.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/log.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <minilzo/minilzo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h> 
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#define COMPRESSED_MMAPPED_FILE_VERSION 2
#define COMPRESSED_MMAPPED_FILE_HEADER_SIZE 12

namespace sserialize {


CompressedMmappedFile::CompressedMmappedFile() : MyParentClass(new CompressedMmappedFilePrivate()) {}
CompressedMmappedFile::CompressedMmappedFile(const sserialize::CompressedMmappedFile& other) : MyParentClass(other) {}
CompressedMmappedFile::CompressedMmappedFile(const std::string& fileName) :
MyParentClass(new CompressedMmappedFilePrivate())
{
	priv()->setFileName(fileName);
}
CompressedMmappedFile::~CompressedMmappedFile() {}
CompressedMmappedFile& CompressedMmappedFile::operator=(const CompressedMmappedFile& other) {
	MyParentClass::operator=(other);
	return *this;
}

CompressedMmappedFile::SizeType CompressedMmappedFile::size() const {
	return priv()->size();
}

std::string CompressedMmappedFile::fileName() const {
	return priv()->fileName();
}

bool CompressedMmappedFile::valid() const {
	return priv()->valid();
}

bool CompressedMmappedFile::open() {
	return priv()->do_open();
}

bool CompressedMmappedFile::close() {
	return priv()->do_close();
}

uint8_t & CompressedMmappedFile::operator[](const SizeType offset) {
	return priv()->operator[](offset);
}

const uint8_t & CompressedMmappedFile::operator[](const SizeType offset) const {
	return priv()->operator[](offset);
}


uint8_t * CompressedMmappedFile::data(const SizeType offset) {
	return priv()->data(offset);
}

void CompressedMmappedFile::read(const SizeType offset, uint8_t* dest, SizeType& len) const {
	priv()->read(offset, dest, len);
}


#if defined(SSERIALIZE_UBA_NON_CONTIGUOUS)
UByteArrayAdapter CompressedMmappedFile::dataAdapter() {
	return UByteArrayAdapter(*this);
}
#elif defined(SSERIALIZE_UBA_ONLY_CONTIGUOUS) && defined(SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL)
UByteArrayAdapter CompressedMmappedFile::dataAdapter() {
	throw sserialize::UnsupportedFeatureException("sserialize was compiled with contiguous UByteArrayAdapter only.");
	return UByteArrayAdapter();
}
#endif

void CompressedMmappedFile::setCacheCount(uint32_t count){
	priv()->setCacheCount(count);
}


//Private implementation

#define HEAP_ALLOC_MINI_LZO(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]


UByteArrayAdapter::SizeType
CompressedMmappedFile::create(const UByteArrayAdapter & src, UByteArrayAdapter & dest, uint8_t chunkSizeExponent, double compressionRatio) {
	if (chunkSizeExponent < 16)
		throw sserialize::CreationException("Chunk size exponent is too small");
	dest.reserveFromPutPtr(COMPRESSED_MMAPPED_FILE_HEADER_SIZE);
	UByteArrayAdapter header = dest;
	dest.incPutPtr(COMPRESSED_MMAPPED_FILE_HEADER_SIZE);
	sserialize::UByteArrayAdapter::OffsetType beginning = dest.tellPutPtr();
	std::vector<SizeType> destOffsets;
	DynamicBitSet chunkTypeBitSet(UByteArrayAdapter(new std::vector<uint8_t>(), true));
	SizeType chunkSize = (static_cast<SizeType>(1) << chunkSizeExponent);

	chunkTypeBitSet.data().growStorage(src.size()/chunkSize + ((src.size() % chunkSize) ? 1 : 0)+COMPRESSED_MMAPPED_FILE_HEADER_SIZE);

	std::vector<uint8_t> inBuf(chunkSize, 0);
	std::vector<uint8_t> outBuf(chunkSize*2+4096, 0);

	HEAP_ALLOC_MINI_LZO(wrkmem, LZO1X_1_MEM_COMPRESS);
	
	
	ProgressInfo  progressInfo;
	progressInfo.begin(src.size(), "CompressedMmappedFile::create");
	for(std::size_t i = 0, chunkNum = 0; i < src.size(); i += chunkSize, ++chunkNum) {
		progressInfo(i);
		destOffsets.push_back(dest.tellPutPtr()-beginning);
		
		lzo_uint inBufLen = 0;
		lzo_uint outBufLen = outBuf.size();
		for(; inBufLen < chunkSize && inBufLen+i < src.size(); ++inBufLen) {
			inBuf[inBufLen] = src.at(inBufLen+i);
		}
		int r = ::lzo1x_1_compress(inBuf.data(), inBufLen, outBuf.data(), &outBufLen, wrkmem);
		if (r != LZO_E_OK) {;
			throw sserialize::CreationException("Failed to compress chunk " + std::to_string(i));
		}
		
		double cmpRatio = (double)inBufLen / outBufLen;
		if (cmpRatio >= compressionRatio) {
			dest.putData(outBuf.data(), outBufLen);
			chunkTypeBitSet.set(chunkNum);
		}
		else {
			dest.putData(inBuf.data(), inBufLen);
		}
	}
	
	progressInfo.end("CompressedMmappedFile::create completed");

	sserialize::UByteArrayAdapter::OffsetType dataSize = dest.tellPutPtr() - beginning;
	
	Static::SortedOffsetIndexPrivate::create(destOffsets, dest);
	dest.putData(chunkTypeBitSet.data());

	header.putUint8(COMPRESSED_MMAPPED_FILE_VERSION);
	header.putUint8(chunkSizeExponent);
	header.putOffset(src.size());
	header.putOffset(dataSize);
	
	return dest.tellPutPtr()-beginning+COMPRESSED_MMAPPED_FILE_VERSION;
}

CompressedMmappedFilePrivate::CompressedMmappedFilePrivate() :
m_size(0),
m_fd(-1),
m_pageSize(sysconf(_SC_PAGE_SIZE)),
m_compressedSize(0),
m_chunkShift(0),
m_chunkMask(0),
m_maxOccupyCount(32),
m_cache(0, 0xFFFFFFFF)
{}

CompressedMmappedFilePrivate::~CompressedMmappedFilePrivate() {
	do_close();
}

void CompressedMmappedFilePrivate::setCacheCount(sserialize::CompressedMmappedFilePrivate::ChunkIndexType count) {
	if (valid() && chunkSize()*count > m_decTileFile.size()) {
		m_decTileFile.resize(chunkSize()*count);
		//repopulate free list
		m_freeList.clear();
		m_freeList.reserve(count);
		for(std::size_t i = 0; i < count; ++i)
			m_freeList.push_back(i);
	}
	m_maxOccupyCount = count;
	uint32_t dummy1, dummy2;
	while(m_cache.occupyCount() > 0) {
		evict(dummy1, dummy2);
	}
	
	m_chunkStorage.resize(count, 0);
}


bool CompressedMmappedFilePrivate::do_open() {
	OffsetType fileSize;

	m_fd = ::open64(m_fileName.c_str(), O_RDONLY);
	if (m_fd < 0)
		return false;
	struct ::stat64 stFileInfo;
	if (::fstat64(m_fd,&stFileInfo) == 0) {
		fileSize = stFileInfo.st_size;
	}
	else {
		return false;
	}
	if (!S_ISREG (stFileInfo.st_mode))
		return false;

	if (fileSize <= COMPRESSED_MMAPPED_FILE_HEADER_SIZE) {
		return false;
	}
	UByteArrayAdapter data = UByteArrayAdapter::open(m_fileName, UByteArrayAdapter::OpenFlags::None());
	UByteArrayAdapter header{data.getMemView(0, COMPRESSED_MMAPPED_FILE_HEADER_SIZE)};
	
	uint8_t version = header.getUint8();
	if (version != COMPRESSED_MMAPPED_FILE_VERSION) {
		sserialize::err("CompressedMmappedFile::open", "Version missmatch: " + std::to_string(version) + " != " + std::to_string(COMPRESSED_MMAPPED_FILE_VERSION) );
		::close(m_fd);
		m_fd = -1;
		return false;
	}
	m_chunkShift = header.getUint8();
	m_chunkMask = createMask(m_chunkShift);
	m_size = header.getOffset();
	m_compressedSize = header.getOffset();

	if ( (std::size_t) (m_compressedSize / chunkSize()) + 1 > (std::size_t) std::numeric_limits<ChunkIndexType>::max() ) {
		throw sserialize::OutOfBoundsException("CompressedMmappedFile: too many chunks");
	}
	
	try {
		m_chunkIndex = Static::SortedOffsetIndex(data+COMPRESSED_MMAPPED_FILE_HEADER_SIZE+m_compressedSize);
	}
	catch (sserialize::Exception & e) {
		sserialize::err("CompressedMmappedFile::open", e.what());
		::close(m_fd);
		m_fd = -1;
		return false;
	}
	m_chunkTypeBitSet = DynamicBitSet(data+COMPRESSED_MMAPPED_FILE_HEADER_SIZE+m_compressedSize+m_chunkIndex.getSizeInBytes());
	
	//prepare the cache
	if (!MmappedFile::createCacheFile(chunkSize()*m_maxOccupyCount, m_decTileFile)) {
		sserialize::err("CompressedMmappedFile", "Creating the decompression storage failed");
		::close(m_fd);
		m_fd = -1;
		return false;
	}
	
	m_freeList.reserve(m_maxOccupyCount);
	for(std::size_t i = 0; i < m_maxOccupyCount; ++i)
		m_freeList.push_back(i);
	m_chunkStorage = std::vector<uint8_t*>(m_maxOccupyCount, 0);
	
	
	m_cache = MyCacheType(m_chunkIndex.size(), std::numeric_limits<uint32_t>::max());
	
	return true;
}

bool CompressedMmappedFilePrivate::do_close() {
	uint32_t dummy1, dummy2;
	while(m_cache.occupyCount() > 0) {
		evict(dummy1, dummy2);
	}

	m_cache.clear();
	m_freeList.clear();
	m_decTileFile.close();
	m_chunkIndex = Static::SortedOffsetIndex();
	m_chunkStorage.clear();
	
	//and close the file
	m_size = 0;
	::close(m_fd);
	m_fd = -1;
	return true;
}

CompressedMmappedFilePrivate::MmapChunkParams
CompressedMmappedFilePrivate::mmapChunkParameters(sserialize::CompressedMmappedFilePrivate::ChunkIndexType chunk) {
	MmapChunkParams params;
	SizeType offset = m_chunkIndex.at(chunk);
	ChunkSizeType chunkLen;
	if (chunk+1 < m_chunkIndex.size()) {
		chunkLen = m_chunkIndex.at(chunk+1) - offset;
	}
	else {
		chunkLen = m_compressedSize - offset;
	}
	//Take care of the header
	SizeType offset_in_file = offset+COMPRESSED_MMAPPED_FILE_HEADER_SIZE;
	
	params.chunk_begin_in_mmap = offset_in_file % m_pageSize;
	params.mmap_begin = offset_in_file - params.chunk_begin_in_mmap;
	params.mmap_size = params.chunk_begin_in_mmap + chunkLen;
	params.mmap_size = (params.mmap_size/m_pageSize)*m_pageSize + size_t(params.mmap_size%m_pageSize!=0)*m_pageSize;
	params.chunk_size = chunkLen;
	return params;
}

uint8_t* CompressedMmappedFilePrivate::mmapChunk(sserialize::CompressedMmappedFilePrivate::ChunkIndexType chunk) {
	
	MmapChunkParams params = mmapChunkParameters(chunk);
	
	uint8_t * data = (uint8_t*) mmap(0, params.mmap_size, PROT_READ, MAP_SHARED, m_fd, params.mmap_begin);
	if (data == MAP_FAILED) {
		sserialize::err("CompressedMmappedFile", "Maping a chunk failed");
		return 0;
	}
	return data+params.chunk_begin_in_mmap;
}

void CompressedMmappedFilePrivate::ummapChunk(sserialize::CompressedMmappedFilePrivate::ChunkIndexType chunk, uint8_t* data) {
	auto params = mmapChunkParameters(chunk);
	
	::munmap(data-params.chunk_begin_in_mmap, params.mmap_size);
}



bool CompressedMmappedFilePrivate::do_unpack(sserialize::CompressedMmappedFilePrivate::ChunkIndexType chunk, uint8_t* dest) {
	auto params = mmapChunkParameters(chunk);
	
	uint8_t * data = (uint8_t*) mmap(nullptr, params.mmap_size, PROT_READ, MAP_SHARED, m_fd, params.mmap_begin);
	if (data == MAP_FAILED) {
		throw sserialize::IOException("CompressedMmappedFile mmapping chunk failed with " + std::string(strerrorname_np(errno)));
	}
	
	lzo_uint destLen = chunkSize();
	int err = ::lzo1x_decompress_safe(data+params.chunk_begin_in_mmap, params.chunk_size, dest, &destLen, 0);
	if (err != LZO_E_OK) {
		throw sserialize::IOException("lzo1x_decompress_safe failed with error " + std::to_string(err));
	}
	
	::munmap(data, params.mmap_size);
	
	return true;
}

bool CompressedMmappedFilePrivate::populate(sserialize::CompressedMmappedFilePrivate::ChunkIndexType chunk, uint8_t*& dest) {
	if (m_chunkTypeBitSet.isSet(chunk)) {
		//get a free chunk from the free list (there always has to be one, otherwise this function was called without evition)
		SizeType decChunkOffset = m_freeList.back() * (SizeType) chunkSize();
		m_freeList.pop_back();
		dest = m_decTileFile.data()+decChunkOffset;
		return do_unpack(chunk, dest);
	}
	else { //no compression, just mmapp the chunk
		dest = mmapChunk(chunk);
		return (dest != 0);
	}
}

bool CompressedMmappedFilePrivate::evict(sserialize::CompressedMmappedFilePrivate::ChunkIndexType& evictedChunk, sserialize::CompressedMmappedFilePrivate::ChunkIndexType& associatedChunkStoragePostion) {
	if (m_cache.occupyCount() == 0)
		return false;
	evictedChunk = m_cache.findVictim();
	associatedChunkStoragePostion = m_cache.directAccess(evictedChunk);
	m_cache.evict(evictedChunk);
	
	//check the type of the cacheline
	if (m_chunkTypeBitSet.isSet(evictedChunk)) { //compressed tile
		uint8_t * decTileDataBegin = m_decTileFile.data();
		std::ptrdiff_t decTileNum = m_chunkStorage[associatedChunkStoragePostion] - decTileDataBegin;
		decTileNum /= chunkSize();
		m_freeList.push_back(decTileNum);
	}
	else {
		ummapChunk(evictedChunk, m_chunkStorage[associatedChunkStoragePostion]);
	}
	
	m_chunkStorage[associatedChunkStoragePostion] = 0;
	return true;
}


CompressedMmappedFilePrivate::ChunkIndexType CompressedMmappedFilePrivate::chunkCount() const {
	ChunkIndexType tmp = (ChunkIndexType) (m_compressedSize / chunkSize());
	return ((m_compressedSize % chunkSize()) ? tmp+1 : tmp); 
}

uint8_t * CompressedMmappedFilePrivate::chunkData(const sserialize::CompressedMmappedFilePrivate::ChunkIndexType chunk) {
	uint32_t chunkStoragePostion = m_cache[chunk]; //returns -1 if not set, usage will be increased by one, but also reset to zero below due to insertion
	if (chunkStoragePostion < std::numeric_limits<uint32_t>::max()) {
		return m_chunkStorage[chunkStoragePostion];
	}
	else {
		uint32_t evictedChunk;
		if (m_cache.occupyCount() >= m_maxOccupyCount) {
			evict(evictedChunk, chunkStoragePostion);
		}
		else {
			chunkStoragePostion = m_cache.occupyCount();
		}
		
		uint8_t* & data = m_chunkStorage[chunkStoragePostion];
		if (!populate(chunk, data))
			data = 0;
		
		m_cache.insert(chunk, chunkStoragePostion);
		
		return data;
	}
}

uint8_t * CompressedMmappedFilePrivate::data(const CompressedMmappedFilePrivate::SizeType offset) {
	ChunkIndexType chunk = this->chunk(offset);
	SizeType inChunkOffSet = this->inChunkOffSet(offset);
	uint8_t * data = chunkData(chunk);
	return data + inChunkOffSet;
}

void CompressedMmappedFilePrivate::read(const CompressedMmappedFilePrivate::SizeType offset, uint8_t * dest, SizeType & len) {
	if (offset > m_size || len == 0) {
		len = 0;
		return;
	}
	if (offset+len > m_size) {
		len =  m_size - offset;
	}
	
	SizeType chunkSize = this->chunkSize();
	
	ChunkIndexType beginChunk = chunk(offset);
	ChunkIndexType endChunk = chunk(offset+len-1);

	::memmove(dest, chunkData(beginChunk)+inChunkOffSet(offset), sizeof(uint8_t)*std::min<SizeType>(len, chunkSize-inChunkOffSet(offset)));
	dest += sizeof(uint8_t)*std::min<ChunkSizeType>(len, chunkSize-inChunkOffSet(offset));
	for(ChunkIndexType i = beginChunk+1; i < endChunk; ++i) {//copy all chunks from within
		::memmove(dest, chunkData(i), sizeof(uint8_t)*chunkSize);
		dest += chunkSize;
	}
	if (beginChunk < endChunk) { //copy things from the last chunk
		::memmove(dest, chunkData(endChunk), sizeof(uint8_t)*inChunkOffSet(offset+len));
	}
}


CompressedMmappedFilePrivate::ChunkIndexType CompressedMmappedFilePrivate::chunk(const CompressedMmappedFilePrivate::SizeType offset) const {
	return (ChunkIndexType) (offset >> m_chunkShift);
}

CompressedMmappedFilePrivate::ChunkSizeType CompressedMmappedFilePrivate::inChunkOffSet(const CompressedMmappedFilePrivate::SizeType offset) const {
	return (ChunkSizeType) (offset & m_chunkMask);
}

bool CompressedMmappedFilePrivate::valid() const {
	return m_fd >= 0;
}


}//end namespace
