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

#define EXCEPT_ON_UNEQUAL(__IS, __SHOULD, __MESSAGE) \
if ( UNLIKELY_BRANCH( __SHOULD != __IS ) ) { throw sserialize::IOException("sserialize::UByteArrayAdapterPrivateThreadSafeFile::" __MESSAGE); }


namespace sserialize {

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
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, &m_buffer, 1, pos), 1, "operator[]");
	return m_buffer;
}

const uint8_t & UByteArrayAdapterPrivateThreadSafeFile::operator[](UByteArrayAdapter::OffsetType pos) const {
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, &m_buffer, 1, pos), 1, "operator[]");
	return m_buffer;
}

int64_t UByteArrayAdapterPrivateThreadSafeFile::getInt64(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[8];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, 8, pos), 8, "getInt64");
	return up_s64(buf);
}

uint64_t UByteArrayAdapterPrivateThreadSafeFile::getUint64(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[8];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, 8, pos), 8, "getUint64");
	return up_u64(buf);
}

int32_t UByteArrayAdapterPrivateThreadSafeFile::getInt32(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[4];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, 4, pos), 4, "getInt32");
	return up_s32(buf);
}

uint32_t UByteArrayAdapterPrivateThreadSafeFile::getUint32(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[4];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, 4, pos), 4, "getUint32");
	return up_u32(buf);
}

uint32_t UByteArrayAdapterPrivateThreadSafeFile::getUint24(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[3];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, 3, pos), 3, "getUint24");
	return up_u24(buf);
}

uint16_t UByteArrayAdapterPrivateThreadSafeFile::getUint16(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[2];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, 2, pos), 2, "getUint16");
	return up_u16(buf);
}

uint8_t UByteArrayAdapterPrivateThreadSafeFile::getUint8(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[1];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, 1, pos), 1, "getUint8");
	return buf[0];
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateThreadSafeFile::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[5];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, 5, pos), 5, "getNegativeOffset");
	return up_s40(buf);
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateThreadSafeFile::getOffset(UByteArrayAdapter::OffsetType pos) const {
	uint8_t buf[5];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, 5, pos), 5, "getOffset");
	return up_u40(buf);
}

uint64_t UByteArrayAdapterPrivateThreadSafeFile::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t buf[9];
	SignedOffsetType readSize = std::min<sserialize::OffsetType>(9, m_size-pos);
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, readSize, pos), readSize, "getVlPackedUint64");
	return up_vu64(buf, length);
}

int64_t UByteArrayAdapterPrivateThreadSafeFile::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t buf[9];
	SignedOffsetType readSize = std::min<sserialize::OffsetType>(9, m_size-pos);
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, readSize, pos), readSize, "getVlPackedInt64");
	return up_vs64(buf, length);
}

uint32_t UByteArrayAdapterPrivateThreadSafeFile::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t buf[5];
	SignedOffsetType readSize = std::min<sserialize::OffsetType>(5, m_size-pos);
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, readSize, pos), readSize, "getVlPackedUint32");
	return up_vu32(buf, length);
}

int32_t UByteArrayAdapterPrivateThreadSafeFile::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t buf[5];
	SignedOffsetType readSize = std::min<sserialize::OffsetType>(5, m_size-pos);
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, readSize, pos), readSize, "getVlPackedInt32");
	return up_vs32(buf, length);
}

void UByteArrayAdapterPrivateThreadSafeFile::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, dest, len, pos), (SignedOffsetType)len, "get");
}

std::string UByteArrayAdapterPrivateThreadSafeFile::getString(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const {
	int myLen;
	uint32_t strLen = getVlPackedUint32(pos, &myLen);
	if (myLen < 1)
		return std::string();
	char buf[strLen];
	EXCEPT_ON_UNEQUAL(::pread64(m_fd, buf, strLen, pos+len), strLen, "getString");
	return std::string(buf, strLen);
}

/** If the supplied memory is not writable then you're on your own! **/

void UByteArrayAdapterPrivateThreadSafeFile::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, sizeof(value), pos), sizeof(value), "putInt64");
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, sizeof(value), pos), sizeof(value), "putUint64");
}

void UByteArrayAdapterPrivateThreadSafeFile::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, sizeof(value), pos), sizeof(value), "putInt32");
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, sizeof(value), pos), sizeof(value), "putUint32");
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[3];
	p_u24(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, 3, pos), 3, "putUint24");
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value) {
	uint8_t buf[sizeof(value)];
	p_cl<decltype(value)>(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, sizeof(value), pos), sizeof(value), "putUint16");
}

void UByteArrayAdapterPrivateThreadSafeFile::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, &value, sizeof(value), pos), sizeof(value), "putUint8");
}

void UByteArrayAdapterPrivateThreadSafeFile::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	uint8_t buf[5];
	p_u40(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, 5, pos), 5, "putOffset");
}

void UByteArrayAdapterPrivateThreadSafeFile::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	uint8_t buf[5];
	p_s40(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, 5, pos), 5, "putNegativeOffset");
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, myLen, pos), myLen, "putVlPackedUint64");
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, myLen, pos), myLen, "putVlPackedInt64");
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, myLen, pos), myLen, "putVlPackedUint32");
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_vu32pad4(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, myLen, pos), myLen, "putVlPackedPad4Uint32");
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_v<decltype(value)>(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, myLen, pos), myLen, "putVlPackedInt32");
	return myLen;
}

int UByteArrayAdapterPrivateThreadSafeFile::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[sizeof(value)+1];
	int myLen = p_vs32pad4(value, buf);
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, buf, myLen, pos), myLen, "putVlPackedPad4Int32");
	return myLen;
}

void UByteArrayAdapterPrivateThreadSafeFile::put(UByteArrayAdapter::OffsetType pos, const uint8_t * src, UByteArrayAdapter::OffsetType len) {
	EXCEPT_ON_UNEQUAL(::pwrite64(m_fd, src, len, pos), (SignedOffsetType)len, "put");
}

}//end namespace