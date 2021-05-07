#ifndef SSERIALIZE_CHUNKED_MMAPPED_FILE_H
#define SSERIALIZE_CHUNKED_MMAPPED_FILE_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/containers/DirectRandomCache.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/types.h>
#include <limits>

namespace sserialize {
namespace detail {
namespace ChunkedMmappedFile {

class MmappedRegion {
	uint8_t * m_data;
	OffsetType m_beginOffset;
	uint32_t m_size;
public:
	MmappedRegion();
	virtual ~MmappedRegion();
	bool contains(OffsetType offset) {
		return (offset >= m_beginOffset && offset < (m_beginOffset+m_size));
	}
	uint8_t & operator[](OffsetType globalposition) {
		return *(m_data + (globalposition-m_beginOffset));
	}
};

}}//end namespace detail::ChunkedMmappedFile

class ChunkedMmappedFilePrivate;

/** This class implements a chunked mmapped file access. The minimum chunksize is 1 MebiByte and a default cache count of 16
  * 
  * This class is NOT thread-safe (and never can: i.e. thread-one holds a refernce to a cacheLine by oeprator[] and thread 2 evicts that cacheLine. There's no way to lock the cacheline without user-interaction in thread 19
  * 
  */
class ChunkedMmappedFile: public RCWrapper<ChunkedMmappedFilePrivate>  {
public:
	typedef OffsetType SizeType;
	typedef SignedOffsetType NegativeSizeType;
protected:
	typedef RCWrapper<ChunkedMmappedFilePrivate> MyParentClass;
public:
    ChunkedMmappedFile();
	ChunkedMmappedFile(const ChunkedMmappedFile & other);
	ChunkedMmappedFile(const std::string & fileName, uint8_t chunkExponent, bool writable);
	ChunkedMmappedFile & operator=(const ChunkedMmappedFile & other);
	virtual ~ChunkedMmappedFile();

	SizeType size() const;
	std::string fileName() const;
	bool valid() const;

	bool open();
	bool close();

	uint8_t & operator[](const SizeType offset);
	const uint8_t & operator[](const SizeType offset) const;
	uint8_t * data(const SizeType offset);
	void read(const ChunkedMmappedFile::SizeType offset, uint8_t* dest, SizeType& len) const;
	void write(const uint8_t * src, const SizeType destOffset, SizeType & len);
	
	#if defined(SSERIALIZE_UBA_NON_CONTIGUOUS) || defined(SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL)
	UByteArrayAdapter dataAdapter();
	#endif

	void setDeleteOnClose(bool deleteOnClose);
	void setSyncOnClose(bool syncOnClose);
	void setCacheCount(uint32_t count);

	/** resizes the file to size bytes. All former data references are invalid after this */
	bool resize(sserialize::ChunkedMmappedFile::SizeType size);
};

class ChunkedMmappedFilePrivate: public RefCountObject  {
public:
	typedef ChunkedMmappedFile::SizeType SizeType;
	typedef ChunkedMmappedFile::NegativeSizeType NegativeSizeType;
	typedef uint32_t ChunkIndexType;
	typedef SizeType ChunkSizeType;
private:
	std::string m_fileName;
	SizeType m_size{0}; //total size of the array
	int m_fd{-1};
	bool m_writable{false};
	bool m_deleteOnClose{false};
	bool m_syncOnClose{false};
	
	/** 1 << m_chunkShift = chunkSize */
	uint8_t m_chunkShift;
	uint32_t m_chunkMask;
	
	ChunkIndexType m_maxOccupyCount{16};
	DirectRandomCache<uint8_t*> m_cache;
private:
	uint8_t * do_map(const sserialize::ChunkedMmappedFilePrivate::ChunkIndexType chunk);
	
	/** unmaps a chunk but does not remove it from the cache */
	bool do_unmap(const ChunkIndexType chunk);
	/** Does an unchecked sync on @param chunk */
	bool do_sync(const ChunkIndexType chunk);
	inline ChunkSizeType chunkSize() { return static_cast<ChunkSizeType>(1) << m_chunkShift; }
	ChunkSizeType sizeOfChunk(ChunkIndexType chunk);
public:
	ChunkedMmappedFilePrivate(uint8_t chunkSizeExponent);
	virtual ~ChunkedMmappedFilePrivate();
	inline void setFileName(std::string fileName) { m_fileName = fileName; }
	inline void setWriteableFlag(bool writable) { m_writable = writable; }
	inline void setDeleteOnClose(bool deleteOnClose) { m_deleteOnClose = deleteOnClose; }
	inline void setSyncOnClose(bool syncOnClose) { m_syncOnClose = syncOnClose; }
	void setCacheCount(uint32_t count);

	inline SizeType size() const { return m_size; }
	inline std::string fileName() const { return m_fileName;}
	bool valid() const;
	
	/** Open file */
	bool do_open();
	/** Close all maps an the file */
	bool do_close();
	
	/** unmaps all open mappings and removes them from the cache */
	bool do_unmap();
	
	bool resize(const SizeType size);

	///This does not do any kind of correctnes checks! 
	inline uint8_t & operator[](const SizeType pos) { return *data(pos); }
	///This does not do any kind of correctnes checks! 
	uint8_t * data(const SizeType offset);
	
	///copys at most len bytes starting from offset into dest, len contains the read bytes
	void read(const ChunkedMmappedFilePrivate::SizeType offset, uint8_t* dest, SizeType& len);
	
	///writes src to destOffset at most len bytes, len contains the number of written bytes
	void write(const uint8_t * src, const SizeType destOffset, SizeType & len);
	
	///This does not do any kind of correctnes checks! 
	uint8_t * chunkData(const sserialize::ChunkedMmappedFilePrivate::ChunkIndexType chunk);
	   ChunkIndexType chunk(const sserialize::ChunkedMmappedFilePrivate::SizeType offset) const;
	   ChunkSizeType inChunkOffSet(const sserialize::ChunkedMmappedFilePrivate::SizeType offset) const;
};

}//end namespace

#endif
