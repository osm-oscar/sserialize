#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_COMPRESSED_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_COMPRESSED_H
#include "UByteArrayAdapterPrivate.h"
#include <sserialize/containers/CompactUintArray.h>
#ifdef SSERIALIZE_WITH_THREADS
#include <mutex>
#endif

/* Compressed Storage Layout:
 * The uncompressed storage is compressed in chunks of 2^n KB
 * -------------------------------------------------------
 * CHUNKSIZE|ITEMINDEX|COMPRESSEDCHUNKS
 * -------------------------------------------------------
 *    1     |   m     |
 * 
 * ItemIndex has to positions of the begining of the compressed data
 * Every chunk will decompress into 2^n KB
 * 
 * These chunks will be stored in a decompression Storage (simple array or file-backed)
 * Cache is implemented via a single indirection look-up table consisting of CHUNK-Index to DECOMP-Offset table.
 * A Single Table entry:
 * -----------------------
 * OFFSET-BITS|CACHED-BIT
 * -----------------------
 * Page-eviction strategy: random select
 * 
 */

namespace sserialize {
 
class UByteArrayAdapterPrivateCompressed: public UByteArrayAdapterPrivate {
private:
	UByteArrayAdapter m_compresseData;
	UByteArrayAdapter m_decompressedData;
	CompactUintArray m_pageTable;
	uint32_t m_maxFrameCount;
	uint32_t m_freeFrameCount;
#ifdef SSERIALIZE_WITH_THREADS
	mutable std::mutex m_fileLock;
#endif
private:
	uint32_t getFreeCacheFrame();
	bool decompress(uint32_t chunkNum, uint32_t destinationOffset);
	

public:
    UByteArrayAdapterPrivateCompressed(const UByteArrayAdapter & compressedData, UByteArrayAdapter & decompressionStorage);
    virtual ~UByteArrayAdapterPrivateCompressed();
	virtual uint8_t & operator[](uint32_t pos);
	virtual const uint8_t & operator[](uint32_t pos) const;

	virtual int64_t getInt64(UByteArrayAdapter::OffsetType pos) const;
	virtual uint64_t getUint64(UByteArrayAdapter::OffsetType pos) const;
	virtual uint32_t getUint32(uint32_t pos) const;
	virtual uint32_t getUint24(uint32_t pos) const;
	virtual uint16_t getUint16(uint32_t pos) const;
	virtual uint8_t getUint8(uint32_t pos) const;
	
	virtual UByteArrayAdapter::NegativeOffsetType getNegativeOffset(UByteArrayAdapter::OffsetType pos) const;
	virtual UByteArrayAdapter::OffsetType getOffset(UByteArrayAdapter::OffsetType pos) const;

	virtual uint64_t getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const;
	virtual int64_t getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const;
	virtual uint32_t getVlPackedUint32(uint32_t pos, int * length) const;
	virtual int32_t getVlPackedInt32(uint32_t pos, int * length) const;

	/** If the supplied memory is not writable then you're on your own! **/
	virtual void putInt64(UByteArrayAdapter::OffsetType pos, int64_t value);
	virtual void putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value);
	virtual void putUint32(uint32_t pos, uint32_t value);
	virtual void putUint24(uint32_t pos, uint32_t value);
	virtual void putUint16(uint32_t pos, uint16_t value);
	virtual void putUint8(uint32_t pos, uint8_t value);

	virtual void putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value);
	virtual void putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value);
	
	/** @return: Length of the number, -1 on failure **/
	virtual int putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType maxLen);
	virtual int putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType maxLen);
	virtual int putVlPackedUint32(uint32_t pos, uint32_t value, uint32_t maxLen);
	virtual int putVlPackedPad4Uint32(uint32_t pos, uint32_t value, uint32_t maxLen);
	virtual int putVlPackedInt32(uint32_t pos, int32_t value, uint32_t maxLen);

};
	
}//end namespace

#endif