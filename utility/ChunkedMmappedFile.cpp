#include <sserialize/utility/ChunkedMmappedFile.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/log.h>
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

namespace sserialize {


ChunkedMmappedFile::ChunkedMmappedFile() : MyParentClass(new ChunkedMmappedFilePrivate(0)) {}
ChunkedMmappedFile::ChunkedMmappedFile(const sserialize::ChunkedMmappedFile& other) : MyParentClass(other) {}
ChunkedMmappedFile::ChunkedMmappedFile(const std::string& fileName, uint8_t chunkSize, bool writable) :
MyParentClass(new ChunkedMmappedFilePrivate(chunkSize))
{
	priv()->setWriteableFlag(writable);
	priv()->setFileName(fileName);
}
ChunkedMmappedFile::~ChunkedMmappedFile() {}
ChunkedMmappedFile& ChunkedMmappedFile::operator=(const ChunkedMmappedFile& other) {
	MyParentClass::operator=(other);
	return *this;
}

ChunkedMmappedFile::SizeType ChunkedMmappedFile::size() const {
	return priv()->size();
}

std::string ChunkedMmappedFile::fileName() const {
	return priv()->fileName();
}

bool ChunkedMmappedFile::valid() const {
	return priv()->valid();
}

bool ChunkedMmappedFile::open() {
	return priv()->do_open();
}

bool ChunkedMmappedFile::close() {
	return priv()->do_close();
}

uint8_t & ChunkedMmappedFile::operator[](const SizeType offset) {
	return priv()->operator[](offset);
}

const uint8_t & ChunkedMmappedFile::operator[](const SizeType offset) const {
	return priv()->operator[](offset);
}


uint8_t * ChunkedMmappedFile::data(const SizeType offset) {
	return priv()->data(offset);
}

void ChunkedMmappedFile::read(const ChunkedMmappedFile::SizeType offset, uint8_t* dest, uint32_t& len) const {
	priv()->read(offset, dest, len);
}

void ChunkedMmappedFile::write(const uint8_t * src, const sserialize::ChunkedMmappedFile::SizeType destOffset, uint32_t & len) {
	priv()->write(src, destOffset, len);
}

UByteArrayAdapter ChunkedMmappedFile::dataAdapter() {
	return UByteArrayAdapter(*this);
}

void ChunkedMmappedFile::setDeleteOnClose(bool deleteOnClose) {
	priv()->setDeleteOnClose(deleteOnClose);
}

void ChunkedMmappedFile::setSyncOnClose(bool syncOnClose) {
	priv()->setSyncOnClose(syncOnClose);
}

void ChunkedMmappedFile::setCacheCount(uint32_t count){
	priv()->setCacheCount(count);
}


bool ChunkedMmappedFile::resize(SizeType size) {
	return priv()->resize(size);
}

//Private implementation


ChunkedMmappedFilePrivate::ChunkedMmappedFilePrivate(uint8_t chunkSizeExponent) :
m_size(0),
m_fd(-1),
m_writable(false),
m_deleteOnClose(false),
m_syncOnClose(false),
m_maxOccupyCount(16)
{
	uint8_t pageSizeExponent = msb((uint32_t) sysconf(_SC_PAGE_SIZE) );
	m_chunkShift = (chunkSizeExponent > 31 ? 31 : (chunkSizeExponent < pageSizeExponent ? pageSizeExponent : chunkSizeExponent)  );
	m_chunkMask = createMask(m_chunkShift);
}

ChunkedMmappedFilePrivate::~ChunkedMmappedFilePrivate() {
	if (valid())
		do_close();
}

void ChunkedMmappedFilePrivate::setCacheCount(uint32_t count) {
	while (m_cache.occupyCount() > count) {
		uint32_t victim = m_cache.findVictim();
		do_unmap(victim);
		m_cache.evict(victim);
	}
	m_maxOccupyCount = count;
}


bool ChunkedMmappedFilePrivate::do_open() {
	int proto = O_RDONLY;
	if (m_writable)
		proto = O_RDWR;
	if (m_fileName.size() == 0)
		return false;
	m_fd = open(m_fileName.c_str(), proto);
	if (m_fd < 0)
		return false;
	struct stat stFileInfo;
	if (fstat(m_fd,&stFileInfo) == 0 && stFileInfo.st_size < std::numeric_limits<SizeType>::max()) {
		m_size = stFileInfo.st_size;
	}
	else {
		return false;
	}
	if (!S_ISREG (stFileInfo.st_mode))
		return false;
	
	//init the cache
	m_cache = DirectLFUCache<uint8_t*>(size()/chunkSize() + 1, 0);
	
	return true;
}


bool ChunkedMmappedFilePrivate::do_close() {
	if (m_fd < 0)
		return false;

	bool allOk = do_unmap();
	
	if (close(m_fd) == -1) {
		return false;
	}
	m_fd = -1;
	m_size = 0;
	
	if (m_deleteOnClose && unlink(m_fileName.c_str()) < 0) {
		return false;
	}
	return allOk;
	
}

bool ChunkedMmappedFilePrivate::do_unmap() {
	uint32_t victim;
	bool allOk = true;
	while(m_cache.occupyCount()) {
		victim = m_cache.findVictim();
		allOk = do_unmap(victim) && allOk;
		m_cache.evict(victim);
	}
	return allOk;
}


inline uint8_t * ChunkedMmappedFilePrivate::do_map(const ChunkedMmappedFilePrivate::SizeType chunk) {
	int mmap_proto = PROT_READ;
	if (m_writable)
		mmap_proto |= PROT_WRITE;
	off_t chunkOffSet = chunk*chunkSize(); //This will always be page aligned as a chunk has a minimum size of PAGE_SIZE
	uint8_t * data = (uint8_t*) mmap(0, sizeOfChunk(chunk), mmap_proto, MAP_SHARED, m_fd, chunkOffSet);
	
	if (data == MAP_FAILED) {
		osmfindlog::err("ChunkedMmappedFile", "Maping a chunk failed");
		return 0;
	}
	return data;
}

bool ChunkedMmappedFilePrivate::do_unmap(const ChunkedMmappedFilePrivate::SizeType chunk) {
	if (m_syncOnClose) {
		do_sync(chunk);
	}
	
	uint8_t * data = m_cache.directAccess(chunk);
	
	if (munmap(data, sizeOfChunk(chunk)) == -1) {
		return false;
	}
	return true;
}

bool ChunkedMmappedFilePrivate::do_sync(const ChunkedMmappedFilePrivate::SizeType chunk) {
	uint8_t * data = m_cache.directAccess(chunk);
	int result = msync(data, sizeOfChunk(chunk), MS_SYNC);
	return result == 0;
}


uint8_t * ChunkedMmappedFilePrivate::data(const ChunkedMmappedFilePrivate::SizeType offset) {
	SizeType chunk = this->chunk(offset);
	SizeType inChunkOffSet = this->inChunkOffSet(offset);
	uint8_t * data = chunkData(chunk);
	return data + inChunkOffSet;
}

void ChunkedMmappedFilePrivate::read(const ChunkedMmappedFilePrivate::SizeType offset, uint8_t * dest, uint32_t& len) {
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

void ChunkedMmappedFilePrivate::write(const uint8_t * src, const sserialize::ChunkedMmappedFilePrivate::SizeType destOffset, uint32_t & len) {
	if (destOffset > m_size || len == 0) {
		len = 0;
		return;
	}
	if (destOffset +len > m_size) {
		len =  m_size - destOffset;
	}
		
	uint32_t chunkSize = this->chunkSize();
	
	uint32_t beginChunk = chunk(destOffset);
	uint32_t endChunk = chunk(destOffset +len-1);

	memmove(chunkData(beginChunk)+inChunkOffSet(destOffset), src, sizeof(uint8_t)*std::min<uint32_t>(len, chunkSize-inChunkOffSet(destOffset)));
	src += sizeof(uint8_t)*std::min<uint32_t>(len, chunkSize-inChunkOffSet(destOffset));
	for(uint32_t i = beginChunk+1; i < endChunk; ++i) {//copy all chunks from within
		memmove(chunkData(i), src, sizeof(uint8_t)*chunkSize);
		src += chunkSize;
	}
	if (beginChunk < endChunk) { //copy things from the last chunk
		memmove(chunkData(endChunk), src, sizeof(uint8_t)*inChunkOffSet(destOffset +len));
	}
}


uint8_t* ChunkedMmappedFilePrivate::chunkData(const sserialize::ChunkedMmappedFilePrivate::SizeType chunk) {
	uint8_t * data = m_cache[chunk]; //returns null if not set, usage will be increased by one, but also reset to zero below due to insertion
	if (data) {
		return data;
	}
	else {
		if (m_cache.occupyCount() >= m_maxOccupyCount) {
			uint32_t victim = m_cache.findVictim(); //this should be valid!
			do_unmap(victim);
			m_cache.evict(victim);
		}
		
		uint8_t * data = do_map(chunk);
		
		m_cache.insert(chunk, data);
		
		return data;
	}
}

uint32_t ChunkedMmappedFilePrivate::sizeOfChunk(uint32_t chunk) {
	uint32_t chunkSize = this->chunkSize();
	if (chunk*chunkSize+chunkSize > m_size) {
		return m_size - chunk*chunkSize;
	}
	else
		return chunkSize;
}


ChunkedMmappedFilePrivate::SizeType ChunkedMmappedFilePrivate::chunk(const sserialize::ChunkedMmappedFilePrivate::SizeType offset) const {
	return offset >> m_chunkShift;
}

ChunkedMmappedFilePrivate::SizeType ChunkedMmappedFilePrivate::inChunkOffSet(const ChunkedMmappedFilePrivate::SizeType offset) const {
	return offset & m_chunkMask;
}


bool ChunkedMmappedFilePrivate::resize(const ChunkedMmappedFilePrivate::SizeType size) {
	if (!do_unmap()) {
		return false;
	}

	bool allOk = true;
	int result = ftruncate(m_fd, size);
	if (result < 0) {
		perror("MmappedFilePrivate::resize");
		allOk = false;
	}
	else {
		m_size = size;
	}
	return allOk;
}

bool ChunkedMmappedFilePrivate::valid() const {
	return m_fd >= 0;
}


}//end namespace