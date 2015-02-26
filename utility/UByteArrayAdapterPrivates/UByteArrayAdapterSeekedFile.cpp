#include "UByteArrayAdapterSeekedFile.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/param.h>
#include <errno.h>
#include <string.h>
#include <limits>
#include <iostream>
#include <sserialize/utility/pack_unpack_functions.h>

namespace sserialize {

uint32_t UByteArrayAdapterPrivateSeekedFile::populateCache(sserialize::UByteArrayAdapter::OffsetType pos, uint32_t len) const {
	assert(len <= m_bufferSize);
	uint64_t tmp = pos-m_bufferOffset;
	if (pos < m_bufferOffset || tmp+len > m_bufferSize) {
		::lseek64(m_fd, pos, SEEK_SET);
		::read(m_fd, m_buffer, m_bufferSize);
		m_bufferOffset = pos;
		tmp = 0;
	}
	return tmp;
}

void UByteArrayAdapterPrivateSeekedFile::updateBufferAfterWrite(UByteArrayAdapter::OffsetType pos, const uint8_t* src, uint32_t len) {
	if (pos >= m_bufferOffset && pos < m_bufferOffset+m_bufferSize) {
		UByteArrayAdapter::OffsetType offInBuff = pos-m_bufferOffset;
		UByteArrayAdapter::OffsetType remBuffLen = m_bufferSize-offInBuff;
		::memmove(m_buffer+offInBuff, src, std::min<UByteArrayAdapter::OffsetType>(remBuffLen, len));
	}
}

UByteArrayAdapterPrivateSeekedFile::UByteArrayAdapterPrivateSeekedFile() :
UByteArrayAdapterPrivate(),
m_fd(-1),
m_bufferSize(0),
m_size(0),
m_bufferOffset(0),
m_buffer(0)
{}

UByteArrayAdapterPrivateSeekedFile::UByteArrayAdapterPrivateSeekedFile(const std::string& filePath, bool writable) :
UByteArrayAdapterPrivate(),
m_fd(-1),
m_bufferSize(4096),
m_size(0),
m_bufferOffset(0),
m_buffer(new uint8_t[m_bufferSize])
{
	int mode = (writable ? O_RDWR : O_RDONLY);
	m_fd = ::open64(filePath.c_str(), mode);
	if (m_fd < 0) {
		throw sserialize::MissingDataException("UByteArrayAdapterPrivateSeekedFile: could not open file");
	}
	struct ::stat64 stFileInfo;
	if (::fstat64(m_fd,&stFileInfo) == 0 && stFileInfo.st_size >= 0 && static_cast<OffsetType>(stFileInfo.st_size) < std::numeric_limits<OffsetType>::max()) {
		m_size = stFileInfo.st_size;
	}
	else {
		throw sserialize::MissingDataException("UByteArrayAdapterPrivateSeekedFile: could not get file size");
	}
	::lseek64(m_fd, 0, SEEK_SET);
	::read(m_fd, m_buffer, std::min<UByteArrayAdapter::OffsetType>(m_bufferSize, m_size));
}

UByteArrayAdapterPrivateSeekedFile::~UByteArrayAdapterPrivateSeekedFile() {
	delete[] m_buffer;
	::close(m_fd);
	if (m_deleteOnClose) {
		//unlink, ignore for now
	}
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateSeekedFile::size() const {
	return m_size;
}

bool UByteArrayAdapterPrivateSeekedFile::isContiguous() const {
	return false;
}

//support opertions

bool UByteArrayAdapterPrivateSeekedFile::shrinkStorage(UByteArrayAdapter::OffsetType size) {
	return ::ftruncate64(m_fd, size) == 0;
}

bool UByteArrayAdapterPrivateSeekedFile::growStorage(UByteArrayAdapter::OffsetType size) {
	return ::ftruncate64(m_fd, size) == 0;
}

//manipulators
void UByteArrayAdapterPrivateSeekedFile::setDeleteOnClose(bool del) {
	m_deleteOnClose = del;
}

//Access functions
uint8_t & UByteArrayAdapterPrivateSeekedFile::operator[](UByteArrayAdapter::OffsetType pos) {
	uint32_t offsetInCache = populateCache(pos, 1);
	return m_buffer[offsetInCache];
}

const uint8_t & UByteArrayAdapterPrivateSeekedFile::operator[](UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 1);
	return m_buffer[offsetInCache];
}

int64_t UByteArrayAdapterPrivateSeekedFile::getInt64(UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 8);
	return up_s64(m_buffer+offsetInCache);
}

uint64_t UByteArrayAdapterPrivateSeekedFile::getUint64(UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 8);
	return up_u64(m_buffer+offsetInCache);
}

int32_t UByteArrayAdapterPrivateSeekedFile::getInt32(UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 4);
	return up_s32(m_buffer+offsetInCache);
}

uint32_t UByteArrayAdapterPrivateSeekedFile::getUint32(UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 4);
	return up_u32(m_buffer+offsetInCache);
}

uint32_t UByteArrayAdapterPrivateSeekedFile::getUint24(UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 3);
	return up_u24(m_buffer+offsetInCache);
}

uint16_t UByteArrayAdapterPrivateSeekedFile::getUint16(UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 2);
	return up_u16(m_buffer+offsetInCache);
}

uint8_t UByteArrayAdapterPrivateSeekedFile::getUint8(UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 1);
	return m_buffer[offsetInCache];
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateSeekedFile::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 5);
	return up_s40(m_buffer+offsetInCache);
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateSeekedFile::getOffset(UByteArrayAdapter::OffsetType pos) const {
	uint32_t offsetInCache = populateCache(pos, 5);
	return up_u40(m_buffer+offsetInCache);
}

uint64_t UByteArrayAdapterPrivateSeekedFile::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t offsetInCache = populateCache(pos, 9);
	return up_vu64(m_buffer+offsetInCache, length);
}

int64_t UByteArrayAdapterPrivateSeekedFile::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t offsetInCache = populateCache(pos, 9);
	return up_vs64(m_buffer+offsetInCache, length);
}

uint32_t UByteArrayAdapterPrivateSeekedFile::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t offsetInCache = populateCache(pos, 5);
	return up_vu32(m_buffer+offsetInCache, length);
}

int32_t UByteArrayAdapterPrivateSeekedFile::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t offsetInCache = populateCache(pos, 5);
	return up_vs32(m_buffer+offsetInCache, length);
}

void UByteArrayAdapterPrivateSeekedFile::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	::lseek64(m_fd, pos, SEEK_SET);
	::read(m_fd, dest, len);
}

std::string UByteArrayAdapterPrivateSeekedFile::getString(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const {
	int myLen;
	uint32_t strLen = getVlPackedUint32(pos, &myLen);
	if (myLen < 1)
		return std::string();
	char buf[strLen];
	::lseek64(m_fd, pos+len, SEEK_SET);
	::read(m_fd, buf, strLen);
	return std::string(buf, strLen);
}

/** If the supplied memory is not writable then you're on your own! **/

void UByteArrayAdapterPrivateSeekedFile::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, sizeof(value));
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateSeekedFile::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, sizeof(value));
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateSeekedFile::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, sizeof(value));
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateSeekedFile::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, sizeof(value));
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateSeekedFile::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[3];
	p_u24(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, 3);
	updateBufferAfterWrite(pos, buf, 3);
}

void UByteArrayAdapterPrivateSeekedFile::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, sizeof(value));
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateSeekedFile::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, &value, sizeof(value));
	updateBufferAfterWrite(pos, &value, sizeof(value));
}

void UByteArrayAdapterPrivateSeekedFile::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	uint8_t buf[5];
	p_u40(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, 5);
	updateBufferAfterWrite(pos, buf, 5);
}

void UByteArrayAdapterPrivateSeekedFile::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	uint8_t buf[5];
	p_s40(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, 5);
	updateBufferAfterWrite(pos, buf, 5);
}

int UByteArrayAdapterPrivateSeekedFile::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, myLen);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateSeekedFile::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, myLen);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateSeekedFile::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, myLen);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateSeekedFile::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_vu32pad4(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, myLen);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateSeekedFile::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, myLen);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateSeekedFile::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_vs32pad4(value, buf);
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, buf, myLen);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

void UByteArrayAdapterPrivateSeekedFile::put(UByteArrayAdapter::OffsetType pos, const uint8_t * src, UByteArrayAdapter::OffsetType len) {
	::lseek64(m_fd, pos, SEEK_SET);
	::write(m_fd, src, len);
	updateBufferAfterWrite(pos, src, len);
}

}//end namespace