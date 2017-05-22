#ifndef SSERIALIZE_UBA_ONLY_CONTIGUOUS
#include "UByteArrayAdapterPrivateCompressed.h"
#ifdef SSERIALIZE_WITH_THREADS
#include <sserialize/utility/MutexLocker.h>
#endif
#include <stdlib.h>
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
namespace UByteArrayAdapterNonContiguous {

uint32_t UByteArrayAdapterPrivateCompressed::getFreeCacheFrame() {
	if (m_freeFrameCount > 0) {
			return m_maxFrameCount-m_freeFrameCount;
	}
	else {
		uint32_t evictFrame = rand() % m_maxFrameCount;
		m_pageTable.set(evictFrame, 0);
		return evictFrame;
	}
}

bool UByteArrayAdapterPrivateCompressed::decompress(uint32_t chunkNum, uint32_t destinationOffset) {
	uint32_t destinationFrame = getFreeCacheFrame();
	
}


UByteArrayAdapterPrivateCompressed::
UByteArrayAdapterPrivateCompressed(const UByteArrayAdapter & compressedData, UByteArrayAdapter & decompressionStorage);
uint8_t & UByteArrayAdapterPrivateCompressed::operator[](uint32_t pos);
const uint8_t & UByteArrayAdapterPrivateCompressed::operator[](uint32_t pos) const;

uint32_t UByteArrayAdapterPrivateCompressed::getUint32(uint32_t pos) const;
uint32_t UByteArrayAdapterPrivateCompressed::getUint24(uint32_t pos) const;
uint16_t UByteArrayAdapterPrivateCompressed::getUint16(uint32_t pos) const;
uint8_t UByteArrayAdapterPrivateCompressed::getUint8(uint32_t pos) const;

uint32_t UByteArrayAdapterPrivateCompressed::getVlPackedUint32(uint32_t pos, int * length) const;
int32_t UByteArrayAdapterPrivateCompressed::getVlPackedInt32(uint32_t pos, int * length) const;

/** If the supplied memory is not writable then you're on your own! **/

void UByteArrayAdapterPrivateCompressed::putUint32(uint32_t pos, uint32_t value);
void UByteArrayAdapterPrivateCompressed::putUint24(uint32_t pos, uint32_t value);
void UByteArrayAdapterPrivateCompressed::putUint16(uint32_t pos, uint16_t value);
void UByteArrayAdapterPrivateCompressed::putUint8(uint32_t pos, uint8_t value);

/** @return: Length of the number, -1 on failure **/
int UByteArrayAdapterPrivateCompressed::putVlPackedUint32(uint32_t pos, uint32_t value, uint32_t maxLen);
int UByteArrayAdapterPrivateCompressed::putVlPackedPad4Uint32(uint32_t pos, uint32_t value, uint32_t maxLen);
int UByteArrayAdapterPrivateCompressed::putVlPackedInt32(uint32_t pos, int32_t value, uint32_t maxLen);
	
}} //end namespace sserialize::UByteArrayAdapterNonContiguous
