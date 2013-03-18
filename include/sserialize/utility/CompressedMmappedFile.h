#ifndef SSERIALIZE_COMPRESSED_MMAPPED_FILE_H
#define SSERIALIZE_COMPRESSED_MMAPPED_FILE_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/templated/DirectCaches.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/utility/mmappedfile.h>
#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/utility/types.h>
#include <limits>
#include <stack>

//TODO: improve by only compresssing compressible tiles, uncompressible tiles should be mmapped into memory

/** This class implements a compressed file with variable size chunks
  * It supports random access on the stored data
  * 
  *-------------------------------------------------------------------------------------
  *CompressedData|ChunkOffsets        |  ChunkType  |CHUNKEXP|DECSIZE |COMPSIZE|VERSION
  *-------------------------------------------------------------------------------------
  *       *      |  SortedOffsetIndex |DynamicBitSet|    1   |   5    |   5    |   1
  *
  * ChunkOffsets: Offset from the beginning of CompressedData to chunk i
  * ChunkType: Bit set which selects if the data is in compressed form or not
  *
  *
  */

namespace sserialize {

class CompressedMmappedFilePrivate;

/** This class implements a chunked mmapped file access. The minimum chunksize is 1 MebiByte and a default cache count of 16 */
class CompressedMmappedFile: public RCWrapper<CompressedMmappedFilePrivate>  {
public:
	typedef OffsetType SizeType;
protected:
	typedef RCWrapper<CompressedMmappedFilePrivate> MyParentClass;
public:
    CompressedMmappedFile();
	CompressedMmappedFile(const CompressedMmappedFile & other);
	CompressedMmappedFile(const std::string & fileName);
	CompressedMmappedFile & operator=(const CompressedMmappedFile & other);
	virtual ~CompressedMmappedFile();

	SizeType size() const;
	std::string fileName() const;
	bool valid() const;

	bool open();
	bool close();

	/** This only returns a reference to the UNPACKED data. You can safely write to it if it'S compressed data , but it will not be stored back to the file
	  * Furthermore, it may be removed if the cache lines get evicted! BUT: if it maps to uncompressed data, tehn you can't write to it
	  *
	  */
	uint8_t & operator[](const SizeType offset);
	const uint8_t & operator[](const SizeType offset) const;
	uint8_t * data(const SizeType offset);
	void read(const SizeType offset, uint8_t* dest, uint32_t& len) const;
	
	UByteArrayAdapter dataAdapter();
	void setCacheCount(uint32_t count);
	
	/** Creates a CompressedMmappedFile
	  *
	  * @param chunkSizeExponent: The size of the chunks (minimum 64 KiB)
	  * @param compressionRatio: The minimum compression ratio to achieve to store a compressed chunk
	  *
	  */
	static bool create(const UByteArrayAdapter & src, UByteArrayAdapter & dest, uint8_t chunkSizeExponent, double compressionRatio);
	
};



class CompressedMmappedFilePrivate: public RefCountObject  {
public:
	typedef CompressedMmappedFile::SizeType SizeType;
	typedef DirectRandomCache<uint32_t> MyCacheType;
private:
	std::string m_fileName;
	SizeType m_size; //Total size of the decompressed file
	int m_fd;
	size_t m_pageSize;
	std::size_t m_compressedSize;
	
	uint8_t * m_chunkIndexData;
	Static::SortedOffsetIndex m_chunkIndex;
	DynamicBitSet m_chunkTypeBitSet;
	
	/** 1 << m_chunkShift = chunkSize */
	uint8_t m_chunkShift;
	uint32_t m_chunkMask;
	
	uint32_t m_maxOccupyCount;
	MyCacheType m_cache; //cache which maps from File chunks to m_chunkStorage positions
	MmappedFile m_decTileFile; //the file storage to store the decompressed data
	std::vector<uint16_t> m_freeList; //list of free places in decTileFile
	std::vector<uint8_t*> m_chunkStorage;
private:
	
	void mmapChunkParameters(SizeType chunk, size_t & mapOverHead, off_t & beginOffset, size_t & mapLen);
	
	///@return ptr to the beginning of the chunk
	uint8_t * mmapChunk(sserialize::CompressedMmappedFilePrivate::SizeType chunk);
	///@param data: ptr to the beginning of the chunk
	void ummapChunk(sserialize::CompressedMmappedFilePrivate::SizeType chunk, uint8_t * data);

	///This function uncompresses chunk number @chunk into @dest
	bool do_unpack(const sserialize::CompressedMmappedFilePrivate::SizeType chunk, uint8_t * dest);
	
	/** This functions populates the chunkStorage with chunk @chunk */
	bool populate(SizeType chunk, uint8_t* & dest);
	
	/** This function evicts a chunk from the cache
	  * it returns the chosen chunk, 0xFFFFFFFF on failure
	  * It checks the type of the chosen chunk an removed mmapped locations or puts the freed decTileChunk back into the freeList
	  *
	  */
	bool evict(uint32_t & evictedChunk, uint32_t & associatedChunkStoragePostion);
	
	inline uint32_t chunkSize() const { return 1 << m_chunkShift; }
	uint32_t chunkCount() const;
public:
	CompressedMmappedFilePrivate();
	virtual ~CompressedMmappedFilePrivate();
	inline void setFileName(std::string fileName) { m_fileName = fileName; }
	
	///sets the cache count, costly operation
	void setCacheCount(uint32_t count);

	inline SizeType size() const { return m_size; }
	inline std::string fileName() const { return m_fileName;}
	bool valid() const;
	
	/** Open file */
	bool do_open();
	/** Close all maps an the file */
	bool do_close();

	///This does not do any kind of correctnes checks! 
	inline uint8_t & operator[](const SizeType pos) { return *data(pos); }
	///This does not do any kind of correctnes checks! 
	uint8_t * data(const SizeType offset);
	
	///copys at most len bytes starting from offset into dest, len contains the read bytes
	void read(const SizeType offset, uint8_t * dest, uint32_t & len);
	
	///This does not do any kind of correctnes checks! 
	uint8_t * chunkData(const SizeType chunk);
	uint32_t chunk(const sserialize::CompressedMmappedFilePrivate::SizeType offset) const;
	uint32_t inChunkOffSet(const sserialize::CompressedMmappedFilePrivate::SizeType offset) const;
};

}//end namespace

#endif