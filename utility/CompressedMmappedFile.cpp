#include "CompressedMmappedFile.h"
#include <sserialize/vendor/libs/minilzo/minilzo.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>
#include "log.h"
#include "ProgressInfo.h"
#include <containers/SortedOffsetIndexPrivate.h>
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
#define COMPRESSED_MMAPPED_FILE_VERSION 1
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

void CompressedMmappedFile::read(const SizeType offset, uint8_t * dest, uint32_t & len) const {
	priv()->read(offset, dest, len);
}


UByteArrayAdapter CompressedMmappedFile::dataAdapter() {
	return UByteArrayAdapter(*this);
}

void CompressedMmappedFile::setCacheCount(uint32_t count){
	priv()->setCacheCount(count);
}


//Private implementation

#define HEAP_ALLOC_MINI_LZO(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]


bool CompressedMmappedFile::create(const UByteArrayAdapter & src, UByteArrayAdapter & dest, uint8_t chunkSizeExponent, double compressionRatio) {
	if (chunkSizeExponent < 16)
		return false;
	sserialize::UByteArrayAdapter::OffsetType beginning = dest.tellPutPtr();
	std::vector<SizeType> destOffsets;
	DynamicBitSet chunkTypeBitSet(UByteArrayAdapter(new std::vector<uint8_t>(), true));
	uint32_t chunkSize = (static_cast<uint32_t>(1) << chunkSizeExponent);

	chunkTypeBitSet.data().growStorage(src.size()/chunkSize + ((src.size() % chunkSize) ? 1 : 0));

	uint8_t * inBuf = new uint8_t[chunkSize];
	uint8_t * outBuf = new uint8_t[chunkSize*2];

	HEAP_ALLOC_MINI_LZO(wrkmem, LZO1X_1_MEM_COMPRESS);
	
	
	ProgressInfo  progressInfo;
	progressInfo.begin(src.size(), "CompressedMmappedFile::create");
	for(std::size_t i = 0, chunkNum = 0; i < src.size(); i += chunkSize, ++chunkNum) {
		progressInfo(i);
		destOffsets.push_back(dest.tellPutPtr()-beginning);
		
		lzo_uint inBufLen = 0;
		lzo_uint outBufLen = chunkSize*2;
		for(; inBufLen < chunkSize && inBufLen+i < src.size(); ++inBufLen) {
			inBuf[inBufLen] = src.at(inBufLen+i);
		}
		int r = lzo1x_1_compress(inBuf,inBufLen,outBuf,&outBufLen,wrkmem);
		if (r != LZO_E_OK) {
			delete[] inBuf;
			delete[] outBuf;
			return false;
		}
		
		double cmpRatio = (double)inBufLen / outBufLen;
		if (cmpRatio >= compressionRatio) {
			dest.put(outBuf, outBufLen);
			chunkTypeBitSet.set(chunkNum);
		}
		else {
			dest.put(inBuf, inBufLen);
		}
	}
	delete[] inBuf;
	delete[] outBuf;
	
	progressInfo.end("CompressedMmappedFile::create completed");

	sserialize::UByteArrayAdapter::OffsetType dataSize = dest.tellPutPtr() - beginning;
	
	Static::SortedOffsetIndexPrivate::create(destOffsets, dest);
	dest.put(chunkTypeBitSet.data());

	std::vector<uint8_t> header(COMPRESSED_MMAPPED_FILE_HEADER_SIZE, 0);
	header[0] = chunkSizeExponent;
	pack_uint40_t(src.size(), &header[1]);
	pack_uint40_t(dataSize, &header[6]);
	header[COMPRESSED_MMAPPED_FILE_HEADER_SIZE-1] = COMPRESSED_MMAPPED_FILE_VERSION;
	
	dest.put(header);
	
	return true;
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

void CompressedMmappedFilePrivate::setCacheCount(uint32_t count) {
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
	std::size_t fileSize;

	if (m_fileName.size() <= COMPRESSED_MMAPPED_FILE_HEADER_SIZE)
		return false;
	m_fd = open(m_fileName.c_str(), O_RDONLY);
	if (m_fd < 0)
		return false;
	struct stat stFileInfo;
	if (fstat(m_fd,&stFileInfo) == 0) {
		fileSize = stFileInfo.st_size;
	}
	else {
		return false;
	}
	if (!S_ISREG (stFileInfo.st_mode))
		return false;
	
	uint8_t headerData[COMPRESSED_MMAPPED_FILE_HEADER_SIZE];
	lseek(m_fd, fileSize-COMPRESSED_MMAPPED_FILE_HEADER_SIZE, SEEK_SET);
	::read(m_fd, headerData, COMPRESSED_MMAPPED_FILE_HEADER_SIZE);
	lseek(m_fd, 0, SEEK_SET);
	
	if (headerData[COMPRESSED_MMAPPED_FILE_HEADER_SIZE-1] != COMPRESSED_MMAPPED_FILE_VERSION) {
		osmfindlog::err("CompressedMmappedFile::open", "Wrong version for file " + m_fileName);
		close(m_fd);
		m_fd = -1;
		return false;
	}
	
	m_chunkShift = headerData[0];
	m_chunkMask = createMask(m_chunkShift);
	m_size = unPack_uint40_t(&headerData[1]);
	m_compressedSize = unPack_uint40_t(&headerData[6]);
	
	//prepare the data for the index
	size_t mapOverHead = (m_compressedSize % m_pageSize);
	off_t beginOffset = m_compressedSize - mapOverHead; //offset in pageSizes
	size_t mapLen = fileSize - COMPRESSED_MMAPPED_FILE_HEADER_SIZE - beginOffset;
	m_chunkIndexData = (uint8_t*) mmap(0, mapLen, PROT_READ, MAP_SHARED, m_fd, beginOffset);
	if (m_chunkIndexData == MAP_FAILED) {
		osmfindlog::err("CompressedMmappedFile", "Maping the chunk index data failed");
		perror("CompressedMmappedFile map index data");
		close(m_fd);
		m_fd = -1;
		return false;
	}
	
	UByteArrayAdapter chunkIndexAdap(m_chunkIndexData+mapOverHead, 0, mapLen-mapOverHead);
	try {
		m_chunkIndex = Static::SortedOffsetIndex(chunkIndexAdap);
	}
	catch (sserialize::Exception & e) {
		osmfindlog::err("CompressedMmappedFile::open", e.what());
		munmap(m_chunkIndexData, mapLen);
		close(m_fd);
		m_fd = -1;
		return false;
	}
	m_chunkTypeBitSet = DynamicBitSet(chunkIndexAdap+m_chunkIndex.getSizeInBytes());
	
	//prepare the cache
	if (!MmappedFile::createCacheFile(chunkSize()*m_maxOccupyCount, m_decTileFile)) {
		osmfindlog::err("CompressedMmappedFile", "Creating the decompression storage failed");
		m_chunkIndex = Static::SortedOffsetIndex();
		munmap(m_chunkIndexData, mapLen);
		close(m_fd);
		m_fd = -1;
		return false;
	}
	
	m_freeList.reserve(m_maxOccupyCount);
	for(std::size_t i = 0; i < m_maxOccupyCount; ++i)
		m_freeList.push_back(i);
	m_chunkStorage = std::vector<uint8_t*>(m_maxOccupyCount, 0);
	
	
	m_cache = DirectLRUCache<uint32_t>(m_chunkIndex.size(), std::numeric_limits<uint32_t>::max());
	
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
	
	//unmap the chunk index
	
	std::size_t fileSize;
	struct stat stFileInfo;
	if (fstat(m_fd,&stFileInfo) == 0) {
		fileSize = stFileInfo.st_size;
	}
	else
		return false;

	size_t beginOffset = m_compressedSize / m_pageSize; //offset in pageSizes
	size_t mapLen = fileSize - (m_pageSize*beginOffset+6);
	munmap(m_chunkIndexData, mapLen);
	m_chunkIndexData = 0;
	
	//and close the file
	m_size = 0;
	close(m_fd);
	m_fd = -1;
	return true;
}

void CompressedMmappedFilePrivate::mmapChunkParameters(CompressedMmappedFilePrivate::SizeType chunk, size_t& mapOverHead, off_t& beginOffset, size_t& mapLen) {
	SizeType offset = m_chunkIndex.at(chunk);
	uint32_t chunkLen;
	if (chunk+1 < m_chunkIndex.size())
		chunkLen = m_chunkIndex.at(chunk+1) - offset;
	else
		chunkLen = m_compressedSize - offset;

	mapOverHead = offset % m_pageSize;
	beginOffset = offset - mapOverHead; //offset in pageSizes
	mapLen = mapOverHead + chunkLen;
}


uint8_t* CompressedMmappedFilePrivate::mmapChunk(CompressedMmappedFilePrivate::SizeType chunk) {
	size_t mapOverHead;
	off_t beginOffset;
	size_t mapLen;
	
	mmapChunkParameters(chunk, mapOverHead, beginOffset, mapLen);
	
	uint8_t * data = (uint8_t*) mmap(0, mapLen, PROT_READ, MAP_SHARED, m_fd, beginOffset);
	if (data == MAP_FAILED) {
		osmfindlog::err("CompressedMmappedFile", "Maping a chunk failed");
		return 0;
	}
	return data+mapOverHead;
}

void CompressedMmappedFilePrivate::ummapChunk(CompressedMmappedFilePrivate::SizeType chunk, uint8_t * data) {
	size_t mapOverHead;
	off_t beginOffset;
	size_t mapLen;
	
	mmapChunkParameters(chunk, mapOverHead, beginOffset, mapLen);
	
	munmap(data-mapOverHead, mapLen);
}



bool CompressedMmappedFilePrivate::do_unpack(const CompressedMmappedFilePrivate::SizeType chunk, uint8_t * dest) {
	size_t mapOverHead;
	off_t beginOffset;
	size_t mapLen;
	
	mmapChunkParameters(chunk, mapOverHead, beginOffset, mapLen);
	
	uint8_t * data = (uint8_t*) mmap(0, mapLen, PROT_READ, MAP_SHARED, m_fd, beginOffset);
	if (data == MAP_FAILED) {
		osmfindlog::err("CompressedMmappedFile", "Maping a chunk failed");
		perror("CompressedMmappedFile::do_unpack::mmap");
		return false;
	}
	
	lzo_uint destLen = chunkSize();
	int ok = lzo1x_decompress(data+mapOverHead, mapLen-mapOverHead, dest, &destLen, 0);
	
	munmap(data, mapLen);
	
	return (ok == LZO_E_OK);
}

bool CompressedMmappedFilePrivate::populate(sserialize::CompressedMmappedFilePrivate::SizeType chunk, uint8_t*& dest) {
	if (m_chunkTypeBitSet.isSet(chunk)) {
		//get a free chunk from the free list (there always has to be one,  otherwise theis function was called without evition)
		uint32_t decChunkOffset = m_freeList.back() * chunkSize();
		m_freeList.pop_back();
		dest = m_decTileFile.data()+decChunkOffset;
		return do_unpack(chunk, dest);
	}
	else { //no compression, just mmapp the chunk
		dest = mmapChunk(chunk);
		return (dest != 0);
	}
}

bool CompressedMmappedFilePrivate::evict(uint32_t & evictedChunk, uint32_t & associatedChunkStoragePostion) {
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


uint32_t CompressedMmappedFilePrivate::chunkCount() const {
	uint32_t tmp = m_compressedSize / chunkSize();
	return ((m_compressedSize % chunkSize()) ? tmp+1 : tmp); 
}

uint8_t * CompressedMmappedFilePrivate::chunkData(const CompressedMmappedFilePrivate::SizeType chunk) {
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
	SizeType chunk = this->chunk(offset);
	SizeType inChunkOffSet = this->inChunkOffSet(offset);
	uint8_t * data = chunkData(chunk);
	return data + inChunkOffSet;
}

void CompressedMmappedFilePrivate::read(const CompressedMmappedFilePrivate::SizeType offset, uint8_t * dest, uint32_t & len) {
	if (offset > m_size || len == 0) {
		len = 0;
		return;
	}
	if (offset+len > m_size)
		len =  m_size - offset;
		
	uint32_t chunkSize = this->chunkSize();
	
	uint32_t beginChunk = chunk(offset);
	uint32_t endChunk = chunk(offset+len-1);

	memmove(dest, chunkData(beginChunk)+inChunkOffSet(offset), sizeof(uint8_t)*std::min<uint32_t>(len, chunkSize-inChunkOffSet(offset)));
	dest += sizeof(uint8_t)*std::min<uint32_t>(len, chunkSize-inChunkOffSet(offset));
	for(uint32_t i = beginChunk+1; i < endChunk; ++i) {//copy all chunks from within
		memmove(dest, chunkData(i), sizeof(uint8_t)*chunkSize);
		dest += chunkSize;
	}
	if (beginChunk < endChunk) { //copy things from the last chunk
		memmove(dest, chunkData(endChunk), sizeof(uint8_t)*inChunkOffSet(offset+len));
	}
}


inline uint32_t CompressedMmappedFilePrivate::chunk(const CompressedMmappedFilePrivate::SizeType offset) const {
	return offset >> m_chunkShift;
}

inline uint32_t CompressedMmappedFilePrivate::inChunkOffSet(const CompressedMmappedFilePrivate::SizeType offset) const {
	return offset & m_chunkMask;
}

inline bool CompressedMmappedFilePrivate::valid() const {
	return m_fd >= 0;
}


}//end namespace