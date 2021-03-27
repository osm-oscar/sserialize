#include "UByteArrayAdapterThreadSafeFile.h"
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
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/constants.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
SSERIALIZE_NAMESPACE_INLINE_UBA_NON_CONTIGUOUS
namespace UByteArrayAdapterNonContiguous {

UByteArrayAdapterPrivateThreadSafeFile::UByteArrayAdapterPrivateThreadSafeFile() :
UByteArrayAdapterPrivate(),
m_fd(-1),
m_size(0),
m_buffer(0)
{}

UByteArrayAdapterPrivateThreadSafeFile::UByteArrayAdapterPrivateThreadSafeFile(const std::string& filePath, bool writable) :
UByteArrayAdapterPrivate(),
m_fd(-1),
m_size(0),
m_buffer(0)
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
}

UByteArrayAdapterPrivateThreadSafeFile::~UByteArrayAdapterPrivateThreadSafeFile() {
	::close(m_fd);
	if (m_deleteOnClose) {
		//unlink, ignore for now
	}
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateThreadSafeFile::size() const {
	return m_size;
}

bool UByteArrayAdapterPrivateThreadSafeFile::isContiguous() const {
	return false;
}

//support opertions

bool UByteArrayAdapterPrivateThreadSafeFile::shrinkStorage(UByteArrayAdapter::OffsetType size) {
	return ::ftruncate64(m_fd, size) == 0;
}

bool UByteArrayAdapterPrivateThreadSafeFile::growStorage(UByteArrayAdapter::OffsetType size) {
	return ::ftruncate64(m_fd, size) == 0;
}

//manipulators
void UByteArrayAdapterPrivateThreadSafeFile::setDeleteOnClose(bool del) {
	m_deleteOnClose = del;
}

//Access functions
uint8_t & UByteArrayAdapterPrivateThreadSafeFile::operator[](UByteArrayAdapter::OffsetType pos) {
	FileHandler::pread(m_fd, &m_buffer, 1, pos);
	return m_buffer;
}

const uint8_t & UByteArrayAdapterPrivateThreadSafeFile::operator[](UByteArrayAdapter::OffsetType pos) const {
	FileHandler::pread(m_fd, &m_buffer, 1, pos);
	return m_buffer;
}

int64_t UByteArrayAdapterPrivateThreadSafeFile::getInt64(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[8];
	FileHandler::pread(m_fd, buf, 8, pos);
	return up_s64(buf);
}

uint64_t UByteArrayAdapterPrivateThreadSafeFile::getUint64(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[8];
	FileHandler::pread(m_fd, buf, 8, pos);
	return up_u64(buf);
}

int32_t UByteArrayAdapterPrivateThreadSafeFile::getInt32(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[4];
	FileHandler::pread(m_fd, buf, 4, pos);
	return up_s32(buf);
}

uint32_t UByteArrayAdapterPrivateThreadSafeFile::getUint32(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[4];
	FileHandler::pread(m_fd, buf, 4, pos);
	return up_u32(buf);
}

uint32_t UByteArrayAdapterPrivateThreadSafeFile::getUint24(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[3];
	FileHandler::pread(m_fd, buf, 3, pos);
	return up_u24(buf);
}

uint16_t UByteArrayAdapterPrivateThreadSafeFile::getUint16(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[2];
	FileHandler::pread(m_fd, buf, 2, pos);
	return up_u16(buf);
}

uint8_t UByteArrayAdapterPrivateThreadSafeFile::getUint8(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[1];
	FileHandler::pread(m_fd, buf, 1, pos);
	return buf[0];
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateThreadSafeFile::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[5];
	FileHandler::pread(m_fd, buf, 5, pos);
	return up_s40(buf);
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateThreadSafeFile::getOffset(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[5];
	FileHandler::pread(m_fd, buf, 5, pos);
	return up_u40(buf);
}

uint64_t UByteArrayAdapterPrivateThreadSafeFile::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t buf[10];
	SignedOffsetType readSize = std::min<sserialize::OffsetType>(10, m_size-pos);
	FileHandler::pread(m_fd, buf, readSize, pos);
	return up_vu64(buf, buf+readSize, length);
}

int64_t UByteArrayAdapterPrivateThreadSafeFile::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t buf[10];
	SignedOffsetType readSize = std::min<sserialize::OffsetType>(10, m_size-pos);
	FileHandler::pread(m_fd, buf, readSize, pos);
	return up_vs64(buf, buf+readSize, length);
}

uint32_t UByteArrayAdapterPrivateThreadSafeFile::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t buf[5];
	SignedOffsetType readSize = std::min<sserialize::OffsetType>(5, m_size-pos);
	FileHandler::pread(m_fd, buf, readSize, pos);
	return up_vu32(buf, buf+readSize, length);
}

int32_t UByteArrayAdapterPrivateThreadSafeFile::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t buf[5];
	SignedOffsetType readSize = std::min<sserialize::OffsetType>(5, m_size-pos);
	FileHandler::pread(m_fd, buf, readSize, pos);
	return up_vs32(buf, buf+readSize, length);
}

void UByteArrayAdapterPrivateThreadSafeFile::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	FileHandler::pread(m_fd, dest, len, pos);
}

std::string UByteArrayAdapterPrivateThreadSafeFile::getString(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const {
	int myLen;
	uint32_t strLen = getVlPackedUint32(pos, &myLen);
	if (myLen < 1)
		return std::string();
	std::string result(strLen, '\0');
	FileHandler::pread(m_fd, result.data(), strLen, pos+len);
	return result;
}

/** If the supplied memory is not writable then you're on your own! **/

void UByteArrayAdapterPrivateThreadSafeFile::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
}

void UByteArrayAdapterPrivateThreadSafeFile::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[3];
	p_u24(value, buf);
	FileHandler::pwrite(m_fd, buf, 3, pos);
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	FileHandler::pwrite(m_fd, buf, sizeof(value), pos);
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
	FileHandler::pwrite(m_fd, &value, sizeof(value), pos);
}

void UByteArrayAdapterPrivateThreadSafeFile::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	uint8_t buf[5];
	p_u40(value, buf);
	FileHandler::pwrite(m_fd, buf, 5, pos);
}

void UByteArrayAdapterPrivateThreadSafeFile::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	uint8_t buf[5];
	p_s40(value, buf);
	FileHandler::pwrite(m_fd, buf, 5, pos);
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[10];
	int myLen = p_v<decltype(value)>(value, buf, buf+10);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[10];
	int myLen = p_v<decltype(value)>(value, buf, buf+10);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int myLen = p_v<decltype(value)>(value, buf, buf+5);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int myLen = p_vu32pad4(value, buf);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int myLen = p_v<decltype(value)>(value, buf, buf+5);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int myLen = p_vs32pad4(value, buf);
	FileHandler::pwrite(m_fd, buf, myLen, pos);
	return myLen;
}

void UByteArrayAdapterPrivateThreadSafeFile::put(UByteArrayAdapter::OffsetType pos, const uint8_t * src, UByteArrayAdapter::OffsetType len) {
	FileHandler::pwrite(m_fd, src, len, pos);
}

}}//end namespace
