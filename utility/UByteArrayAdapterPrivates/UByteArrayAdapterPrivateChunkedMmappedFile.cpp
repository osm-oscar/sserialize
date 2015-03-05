#include "UByteArrayAdapterPrivateChunkedMmappedFile.h"
#include <sserialize/utility/pack_unpack_functions.h>

namespace sserialize {

UByteArrayAdapterPrivateChunkedMmappedFile::UByteArrayAdapterPrivateChunkedMmappedFile(const ChunkedMmappedFile& file) : m_file(file) {}
UByteArrayAdapterPrivateChunkedMmappedFile::~UByteArrayAdapterPrivateChunkedMmappedFile() {}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateChunkedMmappedFile::size() const {
	return m_file.size();
}


/** Shrink data to size bytes */
bool UByteArrayAdapterPrivateChunkedMmappedFile::shrinkStorage(UByteArrayAdapter::OffsetType size) {
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	return m_file.resize(size);
}

/** grow data to at least! size bytes */
bool UByteArrayAdapterPrivateChunkedMmappedFile::growStorage(UByteArrayAdapter::OffsetType size) {
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	if (m_file.size() < size)
		return m_file.resize(size);
	return true;
}

void UByteArrayAdapterPrivateChunkedMmappedFile::setDeleteOnClose(bool del) {
	m_file.setDeleteOnClose(del);
}

//Access functions
uint8_t & UByteArrayAdapterPrivateChunkedMmappedFile::operator[](UByteArrayAdapter::OffsetType pos) {
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	return m_file.operator[](pos);
}

const uint8_t & UByteArrayAdapterPrivateChunkedMmappedFile::operator[](UByteArrayAdapter::OffsetType pos) const {
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	return m_file.operator[](pos);
}

int64_t UByteArrayAdapterPrivateChunkedMmappedFile::getInt64(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len = 8;
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

uint64_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint64(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len = 8;
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

int32_t UByteArrayAdapterPrivateChunkedMmappedFile::getInt32(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len= 4;
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

uint32_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint32(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len= 4;
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

uint32_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint24(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len= 3;
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

uint16_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint16(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len= 2;
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

uint8_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint8(UByteArrayAdapter::OffsetType pos) const {
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	return m_file[pos];
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateChunkedMmappedFile::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len = 5;
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

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateChunkedMmappedFile::getOffset(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len = 5;
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

uint64_t UByteArrayAdapterPrivateChunkedMmappedFile::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t bufLen = (*length > 9 ? 9 : *length);
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


int64_t UByteArrayAdapterPrivateChunkedMmappedFile::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t bufLen = (*length > 9 ? 9 : *length);
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

uint32_t UByteArrayAdapterPrivateChunkedMmappedFile::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t bufLen = (*length > 5 ? 5 : *length);
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

int32_t UByteArrayAdapterPrivateChunkedMmappedFile::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t bufLen = (*length > 5 ? 5 : *length);
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

void UByteArrayAdapterPrivateChunkedMmappedFile::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	uint32_t mightOverFlow= len;
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.lock();
#endif
	m_file.read(pos, dest, mightOverFlow);
#ifdef SSERIALIZE_WITH_THREADS
	m_fileLock.unlock();
#endif
}


/** If the supplied memory is not writable then you're on your own! **/

void UByteArrayAdapterPrivateChunkedMmappedFile::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	uint32_t len = 8;
	uint8_t buf[len];
	p_s64(value, buf);
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	uint32_t len = 8;
	uint8_t buf[len];
	p_u64(value, buf);
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	uint32_t len = 4;
	uint8_t buf[len];
	p_s32(value, buf);
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint32_t len = 4;
	uint8_t buf[len];
	p_u32(value, buf);
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint32_t len = 3;
	uint8_t buf[len];
	p_u24(value, buf);
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value) {
	uint32_t len = 2;
	uint8_t buf[len];
	p_u16(value, buf);
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file[pos] = value;
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	uint32_t len = 5;
	uint8_t buf[len];
	p_u40(value, buf);
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	uint32_t len = 5;
	uint8_t buf[len];
	p_s40(value, buf);
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.write(buf, pos, len);
}


/** @return: Length of the number, -1 on failure **/
int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[9];
	int len = p_vu64(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
#ifdef SSERIALIZE_WITH_THREADS
		std::unique_lock<std::mutex> locker(m_fileLock);
#endif
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[9];
	int len = p_vs64(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
#ifdef SSERIALIZE_WITH_THREADS
		std::unique_lock<std::mutex> locker(m_fileLock);
#endif
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int len = p_vu32(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
#ifdef SSERIALIZE_WITH_THREADS
		std::unique_lock<std::mutex> locker(m_fileLock);
#endif
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int len = p_vu32pad4(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
#ifdef SSERIALIZE_WITH_THREADS
		std::unique_lock<std::mutex> locker(m_fileLock);
#endif
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int len = p_vs32(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
#ifdef SSERIALIZE_WITH_THREADS
		std::unique_lock<std::mutex> locker(m_fileLock);
#endif
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int len = p_vs32pad4(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
#ifdef SSERIALIZE_WITH_THREADS
		std::unique_lock<std::mutex> locker(m_fileLock);
#endif
		m_file.write(buf, pos, mlen);
	}
	return len;
}

void UByteArrayAdapterPrivateChunkedMmappedFile::put(sserialize::UByteArrayAdapter::OffsetType pos, const uint8_t * src, sserialize::UByteArrayAdapter::OffsetType len) {
	uint32_t mightOverFlow = len;
#ifdef SSERIALIZE_WITH_THREADS
	std::unique_lock<std::mutex> locker(m_fileLock);
#endif
	m_file.write(src, pos, mightOverFlow);
}


}//end namespace