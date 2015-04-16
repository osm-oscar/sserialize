#include "UByteArrayAdapterPrivateCompressedMmappedFile.h"
#include <sserialize/utility/pack_unpack_functions.h>

namespace sserialize {

UByteArrayAdapterPrivateCompressedMmappedFile::UByteArrayAdapterPrivateCompressedMmappedFile(const CompressedMmappedFile& file) : m_file(file) {}
UByteArrayAdapterPrivateCompressedMmappedFile::~UByteArrayAdapterPrivateCompressedMmappedFile() {}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateCompressedMmappedFile::size() const {
	return m_file.size();
}


/** Shrink data to size bytes */
bool UByteArrayAdapterPrivateCompressedMmappedFile::shrinkStorage(UByteArrayAdapter::OffsetType /*size*/) {
	return false;
}

/** grow data to at least! size bytes */
bool UByteArrayAdapterPrivateCompressedMmappedFile::growStorage(UByteArrayAdapter::OffsetType /*size*/) {
	return false;
}

void UByteArrayAdapterPrivateCompressedMmappedFile::setDeleteOnClose(bool /*del*/) {}

//Access functions
uint8_t & UByteArrayAdapterPrivateCompressedMmappedFile::operator[](UByteArrayAdapter::OffsetType pos) {
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	return m_file.operator[](pos);
}

const uint8_t & UByteArrayAdapterPrivateCompressedMmappedFile::operator[](UByteArrayAdapter::OffsetType pos) const {
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	return m_file.operator[](pos);
}

int64_t UByteArrayAdapterPrivateCompressedMmappedFile::getInt64(UByteArrayAdapter::OffsetType pos) const {
	SizeType len = 8;
	uint8_t buf[len];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, len);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_s64(buf);
}

uint64_t UByteArrayAdapterPrivateCompressedMmappedFile::getUint64(UByteArrayAdapter::OffsetType pos) const {
	SizeType len = 8;
	uint8_t buf[len];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, len);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_u64(buf);
}

int32_t UByteArrayAdapterPrivateCompressedMmappedFile::getInt32(UByteArrayAdapter::OffsetType pos) const {
	SizeType len= 4;
	uint8_t buf[len];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, len);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_s32(buf);
}

uint32_t UByteArrayAdapterPrivateCompressedMmappedFile::getUint32(UByteArrayAdapter::OffsetType pos) const {
	SizeType len= 4;
	uint8_t buf[len];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, len);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_u32(buf);
}

uint32_t UByteArrayAdapterPrivateCompressedMmappedFile::getUint24(UByteArrayAdapter::OffsetType pos) const {
	SizeType len= 3;
	uint8_t buf[len];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, len);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_u24(buf);
}

uint16_t UByteArrayAdapterPrivateCompressedMmappedFile::getUint16(UByteArrayAdapter::OffsetType pos) const {
	SizeType len= 2;
	uint8_t buf[len];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, len);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_u16(buf);
}

uint8_t UByteArrayAdapterPrivateCompressedMmappedFile::getUint8(UByteArrayAdapter::OffsetType pos) const {
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	return m_file[pos];
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateCompressedMmappedFile::getOffset(UByteArrayAdapter::OffsetType pos) const {
	SizeType len = 5;
	uint8_t buf[len];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, len);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_u40(buf);
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateCompressedMmappedFile::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	SizeType len = 5;
	uint8_t buf[len];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, len);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_s40(buf);
}

int64_t UByteArrayAdapterPrivateCompressedMmappedFile::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	SizeType bufLen = (*length > 9 ? 9 : *length);
	*length = bufLen;
	uint8_t buf[bufLen];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, bufLen);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_vs64(buf, length);
}

uint64_t UByteArrayAdapterPrivateCompressedMmappedFile::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	SizeType bufLen = (*length > 9 ? 9 : *length);
	*length = bufLen;
	uint8_t buf[bufLen];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, bufLen);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_vu64(buf, length);
}

uint32_t UByteArrayAdapterPrivateCompressedMmappedFile::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	SizeType bufLen = (*length > 5 ? 5 : *length);
	*length = bufLen;
	uint8_t buf[bufLen];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, bufLen);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_vu32(buf, length);
}

int32_t UByteArrayAdapterPrivateCompressedMmappedFile::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	SizeType bufLen = (*length > 5 ? 5 : *length);
	uint8_t buf[bufLen];
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, buf, bufLen);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
	return up_vs32(buf, length);
}

void UByteArrayAdapterPrivateCompressedMmappedFile::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	SizeType mightOverFlow = len;
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.read(pos, dest, mightOverFlow);
}

/** If the supplied memory is not writable then you're on your own! **/

void UByteArrayAdapterPrivateCompressedMmappedFile::putInt64(UByteArrayAdapter::OffsetType /*pos*/, int64_t /*value*/) {}

void UByteArrayAdapterPrivateCompressedMmappedFile::putUint64(UByteArrayAdapter::OffsetType /*pos*/, uint64_t /*value*/) {}

void UByteArrayAdapterPrivateCompressedMmappedFile::putInt32(UByteArrayAdapter::OffsetType /*pos*/, int32_t /*value*/) {}

void UByteArrayAdapterPrivateCompressedMmappedFile::putUint32(UByteArrayAdapter::OffsetType /*pos*/, uint32_t /*value*/) {}

void UByteArrayAdapterPrivateCompressedMmappedFile::putUint24(UByteArrayAdapter::OffsetType /*pos*/, uint32_t /*value*/) {}

void UByteArrayAdapterPrivateCompressedMmappedFile::putUint16(UByteArrayAdapter::OffsetType /*pos*/, uint16_t /*value*/) {}

void UByteArrayAdapterPrivateCompressedMmappedFile::putUint8(UByteArrayAdapter::OffsetType /*pos*/, uint8_t /*value*/) {}

void UByteArrayAdapterPrivateCompressedMmappedFile::putNegativeOffset(UByteArrayAdapter::OffsetType /*pos*/, UByteArrayAdapter::NegativeOffsetType /*value*/) {}

void UByteArrayAdapterPrivateCompressedMmappedFile::putOffset(UByteArrayAdapter::OffsetType /*pos*/, UByteArrayAdapter::OffsetType /*value*/) {}


/** @return: Length of the number, -1 on failure **/
int UByteArrayAdapterPrivateCompressedMmappedFile::putVlPackedInt64(UByteArrayAdapter::OffsetType /*pos*/, int64_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return -1;
}

int UByteArrayAdapterPrivateCompressedMmappedFile::putVlPackedUint64(UByteArrayAdapter::OffsetType /*pos*/, uint64_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return -1;
}


int UByteArrayAdapterPrivateCompressedMmappedFile::putVlPackedUint32(UByteArrayAdapter::OffsetType /*pos*/, uint32_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return -1;
}

int UByteArrayAdapterPrivateCompressedMmappedFile::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType /*pos*/, uint32_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return -1;
}

int UByteArrayAdapterPrivateCompressedMmappedFile::putVlPackedInt32(UByteArrayAdapter::OffsetType /*pos*/, int32_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return -1;
}

int UByteArrayAdapterPrivateCompressedMmappedFile::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType /*pos*/, int32_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return -1;
}

void UByteArrayAdapterPrivateCompressedMmappedFile::put(sserialize::UByteArrayAdapter::OffsetType /*pos*/, const uint8_t * /*src*/, sserialize::UByteArrayAdapter::OffsetType /*len*/) {}



}//end namespace