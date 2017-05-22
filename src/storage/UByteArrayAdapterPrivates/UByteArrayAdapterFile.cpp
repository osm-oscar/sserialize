#include "UByteArrayAdapterFile.h"
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/constants.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/assert.h>
#include <sserialize/storage/FileHandler.h>
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

#define BUFFER_SIZE 4096

namespace sserialize {
namespace UByteArrayAdapterNonContiguous {

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateFile::populateCache(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const {
	SSERIALIZE_CHEAP_ASSERT(len <= m_bufferSize);
	uint64_t tmp = pos-m_bufferOffset;
	if (pos < m_bufferOffset || tmp+len > m_bufferSize) {
		FileHandler::pread(m_fd, m_buffer, m_bufferSize, pos);
		m_bufferOffset = pos;
		tmp = 0;
	}
	return tmp;
}

void UByteArrayAdapterPrivateFile::updateBufferAfterWrite(sserialize::UByteArrayAdapter::OffsetType pos, const uint8_t* src, UByteArrayAdapter::OffsetType len) {
	if (pos >= m_bufferOffset && pos < m_bufferOffset+m_bufferSize) {
		UByteArrayAdapter::OffsetType offInBuff = pos-m_bufferOffset;
		UByteArrayAdapter::OffsetType remBuffLen = m_bufferSize-offInBuff;
		::memmove(m_buffer+offInBuff, src, std::min<UByteArrayAdapter::OffsetType>(remBuffLen, len));
	}
}


UByteArrayAdapterPrivateFile::UByteArrayAdapterPrivateFile() :
UByteArrayAdapterPrivate(),
m_fd(-1),
m_bufferSize(0),
m_size(0),
m_bufferOffset(0),
m_buffer(0)
{}

UByteArrayAdapterPrivateFile::UByteArrayAdapterPrivateFile(const std::string& filePath, bool writable) :
UByteArrayAdapterPrivate(),
m_fd(-1),
m_bufferSize(BUFFER_SIZE),
m_size(0),
m_bufferOffset(0),
m_buffer(new uint8_t[m_bufferSize])
{
	int mode = (writable ? O_RDWR : O_RDONLY);
	m_fd = ::open64(filePath.c_str(), mode);
	if (m_fd < 0) {
		throw sserialize::MissingDataException("UByteArrayAdapterPrivateFile: could not open file");
	}
	struct ::stat64 stFileInfo;
	if (::fstat64(m_fd,&stFileInfo) == 0 && stFileInfo.st_size >= 0 && static_cast<OffsetType>(stFileInfo.st_size) < std::numeric_limits<OffsetType>::max()) {
		m_size = stFileInfo.st_size;
	}
	else {
		throw sserialize::MissingDataException("UByteArrayAdapterPrivateFile: could not get file size");
	}
	SignedOffsetType readSize = std::min<UByteArrayAdapter::OffsetType>(m_bufferSize, m_size);
	FileHandler::pread(m_fd, m_buffer, readSize, 0);
}

UByteArrayAdapterPrivateFile::~UByteArrayAdapterPrivateFile() {
	delete[] m_buffer;
	::close(m_fd);
	if (m_deleteOnClose) {
		//unlink, ignore for now
	}
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateFile::size() const {
	return m_size;
}

bool UByteArrayAdapterPrivateFile::isContiguous() const {
	return false;
}

//support opertions

bool UByteArrayAdapterPrivateFile::shrinkStorage(UByteArrayAdapter::OffsetType size) {
	return ::ftruncate64(m_fd, size) == 0;
}

bool UByteArrayAdapterPrivateFile::growStorage(UByteArrayAdapter::OffsetType size) {
	return ::ftruncate64(m_fd, size) == 0;
}

//manipulators
void UByteArrayAdapterPrivateFile::setDeleteOnClose(bool del) {
	m_deleteOnClose = del;
}

//Access functions
uint8_t & UByteArrayAdapterPrivateFile::operator[](UByteArrayAdapter::OffsetType pos) {
	auto offsetInCache = populateCache(pos, 1);
	return m_buffer[offsetInCache];
}

const uint8_t & UByteArrayAdapterPrivateFile::operator[](UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 1);
	return m_buffer[offsetInCache];
}

int64_t UByteArrayAdapterPrivateFile::getInt64(UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 8);
	return up_s64(m_buffer+offsetInCache);
}

uint64_t UByteArrayAdapterPrivateFile::getUint64(UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 8);
	return up_u64(m_buffer+offsetInCache);
}

int32_t UByteArrayAdapterPrivateFile::getInt32(UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 4);
	return up_s32(m_buffer+offsetInCache);
}

uint32_t UByteArrayAdapterPrivateFile::getUint32(UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 4);
	return up_u32(m_buffer+offsetInCache);
}

uint32_t UByteArrayAdapterPrivateFile::getUint24(UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 3);
	return up_u24(m_buffer+offsetInCache);
}

uint16_t UByteArrayAdapterPrivateFile::getUint16(UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 2);
	return up_u16(m_buffer+offsetInCache);
}

uint8_t UByteArrayAdapterPrivateFile::getUint8(UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 1);
	return m_buffer[offsetInCache];
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateFile::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 5);
	return up_s40(m_buffer+offsetInCache);
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateFile::getOffset(UByteArrayAdapter::OffsetType pos) const {
	auto offsetInCache = populateCache(pos, 5);
	return up_u40(m_buffer+offsetInCache);
}

uint64_t UByteArrayAdapterPrivateFile::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	auto offsetInCache = populateCache(pos, 9);
	return up_vu64(m_buffer+offsetInCache, length);
}

int64_t UByteArrayAdapterPrivateFile::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	auto offsetInCache = populateCache(pos, 9);
	return up_vs64(m_buffer+offsetInCache, length);
}

uint32_t UByteArrayAdapterPrivateFile::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	auto offsetInCache = populateCache(pos, 5);
	return up_vu32(m_buffer+offsetInCache, length);
}

int32_t UByteArrayAdapterPrivateFile::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	auto offsetInCache = populateCache(pos, 5);
	return up_vs32(m_buffer+offsetInCache, length);
}

void UByteArrayAdapterPrivateFile::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	FileHandler::pread(m_fd, dest, len, pos);
}

std::string UByteArrayAdapterPrivateFile::getString(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const {
	int myLen;
	auto strLen = getVlPackedUint32(pos, &myLen);
	if (myLen < 1)
		return std::string();
	char buf[strLen];
	FileHandler::pread(m_fd, buf, strLen, pos+len);
	return std::string(buf, strLen);
}

/** If the supplied memory is not writable then you're on your own! **/

void UByteArrayAdapterPrivateFile::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateFile::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateFile::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateFile::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateFile::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[3];
	p_u24(value, buf);
	FileHandler::pwrite(m_fd, buf, 3, pos);
	updateBufferAfterWrite(pos, buf, 3);
}

void UByteArrayAdapterPrivateFile::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
	updateBufferAfterWrite(pos, buf, sizeof(value));
}

void UByteArrayAdapterPrivateFile::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
	FileHandler::pwrite(m_fd, &value, sizeof(value), pos);
	updateBufferAfterWrite(pos, &value, sizeof(value));
}

void UByteArrayAdapterPrivateFile::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	uint8_t buf[5];
	p_u40(value, buf);
	FileHandler::pwrite(m_fd, buf, 5, pos);
	updateBufferAfterWrite(pos, buf, 5);
}

void UByteArrayAdapterPrivateFile::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	uint8_t buf[5];
	p_s40(value, buf);
	FileHandler::pwrite(m_fd, buf, 5, pos);
	updateBufferAfterWrite(pos, buf, 5);
}

int UByteArrayAdapterPrivateFile::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateFile::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateFile::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateFile::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_vu32pad4(value, buf);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateFile::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

int UByteArrayAdapterPrivateFile::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_vs32pad4(value, buf);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	updateBufferAfterWrite(pos, buf, myLen);
	return myLen;
}

void UByteArrayAdapterPrivateFile::put(UByteArrayAdapter::OffsetType pos, const uint8_t * src, UByteArrayAdapter::OffsetType len) {
	FileHandler::pwrite(m_fd, src, len, pos);
	updateBufferAfterWrite(pos, src, len);
}

}}//end namespace sserialize::UByteArrayAdapterNonContiguous
