#include "UByteArrayAdapterPrivateChunkedMmappedFile.h"
#include <sserialize/utility/pack_unpack_functions.h>

namespace sserialize {

UByteArrayAdapterPrivateChunkedMmappedFile::UByteArrayAdapterPrivateChunkedMmappedFile(const ChunkedMmappedFile& file) : m_file(file) {}
UByteArrayAdapterPrivateChunkedMmappedFile::~UByteArrayAdapterPrivateChunkedMmappedFile() {}


/** Shrink data to size bytes */
bool UByteArrayAdapterPrivateChunkedMmappedFile::shrinkStorage(UByteArrayAdapter::OffsetType size) {
	return m_file.resize(size);
}

/** grow data to at least! size bytes */
bool UByteArrayAdapterPrivateChunkedMmappedFile::growStorage(UByteArrayAdapter::OffsetType size) {
	if (m_file.size() < size)
		return m_file.resize(size);
	return true;
}

void UByteArrayAdapterPrivateChunkedMmappedFile::setDeleteOnClose(bool del) {
	m_file.setDeleteOnClose(del);
}

//Access functions
uint8_t & UByteArrayAdapterPrivateChunkedMmappedFile::operator[](UByteArrayAdapter::OffsetType pos) {
	return m_file.operator[](pos);
}

const uint8_t & UByteArrayAdapterPrivateChunkedMmappedFile::operator[](UByteArrayAdapter::OffsetType pos) const {
	return m_file.operator[](pos);
}

int64_t UByteArrayAdapterPrivateChunkedMmappedFile::getInt64(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len = 8;
	uint8_t buf[len];
	m_file.read(pos, buf, len);
	return unPack_int64_t(buf);
}

uint64_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint64(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len = 8;
	uint8_t buf[len];
	m_file.read(pos, buf, len);
	return unPack_uint64_t(buf);
}

int32_t UByteArrayAdapterPrivateChunkedMmappedFile::getInt32(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len= 4;
	uint8_t buf[len];
	m_file.read(pos, buf, len);
	return unPack_int32_t(buf[0], buf[1], buf[2], buf[3]);
}

uint32_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint32(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len= 4;
	uint8_t buf[len];
	m_file.read(pos, buf, len);
	return unPack_uint32_t(buf[0], buf[1], buf[2], buf[3]);
}

uint32_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint24(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len= 3;
	uint8_t buf[len];
	m_file.read(pos, buf, len);
	return unPack_uint24_t(buf[0], buf[1], buf[2]);
}

uint16_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint16(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len= 2;
	uint8_t buf[len];
	m_file.read(pos, buf, len);
	return unPack_uint16_t(buf[0], buf[1]);
}

uint8_t UByteArrayAdapterPrivateChunkedMmappedFile::getUint8(UByteArrayAdapter::OffsetType pos) const {
	return m_file[pos];
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateChunkedMmappedFile::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len = 5;
	uint8_t buf[len];
	m_file.read(pos, buf, len);
	return unPack_int40_t(buf);
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateChunkedMmappedFile::getOffset(UByteArrayAdapter::OffsetType pos) const {
	uint32_t len = 5;
	uint8_t buf[len];
	m_file.read(pos, buf, len);
	return unPack_uint40_t(buf);
}

uint64_t UByteArrayAdapterPrivateChunkedMmappedFile::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t bufLen = (*length > 9 ? 9 : *length);
	*length = bufLen;
	uint8_t buf[bufLen];
	m_file.read(pos, buf, bufLen);
	return vl_unpack_uint64_t(buf, length);
}


int64_t UByteArrayAdapterPrivateChunkedMmappedFile::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t bufLen = (*length > 9 ? 9 : *length);
	*length = bufLen;
	uint8_t buf[bufLen];
	m_file.read(pos, buf, bufLen);
	return vl_unpack_int64_t(buf, length);
}

uint32_t UByteArrayAdapterPrivateChunkedMmappedFile::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t bufLen = (*length > 5 ? 5 : *length);
	*length = bufLen;
	uint8_t buf[bufLen];
	m_file.read(pos, buf, bufLen);
	return vl_unpack_uint32_t(buf, length);
}

int32_t UByteArrayAdapterPrivateChunkedMmappedFile::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint32_t bufLen = (*length > 5 ? 5 : *length);
	uint8_t buf[bufLen];
	m_file.read(pos, buf, bufLen);
	return vl_unpack_int32_t(buf, length);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	uint32_t mightOverFlow= len;
	m_file.read(pos, dest, mightOverFlow);
}


/** If the supplied memory is not writable then you're on your own! **/

void UByteArrayAdapterPrivateChunkedMmappedFile::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	uint32_t len = 8;
	uint8_t buf[len];
	pack_int64_t(value, buf);
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	uint32_t len = 8;
	uint8_t buf[len];
	pack_uint64_t(value, buf);
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	uint32_t len = 4;
	uint8_t buf[len];
	pack_int32_t(value, buf);
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint32_t len = 4;
	uint8_t buf[len];
	pack_uint32_t(value, buf);
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint32_t len = 3;
	uint8_t buf[len];
	pack_uint24_t(value, buf);
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value) {
	uint32_t len = 2;
	uint8_t buf[len];
	pack_uint16_t(value, buf);
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
	m_file[pos] = value;
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	uint32_t len = 5;
	uint8_t buf[len];
	pack_uint40_t(value, buf);
	m_file.write(buf, pos, len);
}

void UByteArrayAdapterPrivateChunkedMmappedFile::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	uint32_t len = 5;
	uint8_t buf[len];
	pack_int40_t(value, buf);
	m_file.write(buf, pos, len);
}


/** @return: Length of the number, -1 on failure **/
int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType maxLen) {
	uint8_t buf[9];
	int len = vl_pack_uint64_t(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType maxLen) {
	uint8_t buf[9];
	int len = vl_pack_int64_t(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) {
	uint8_t buf[5];
	int len = vl_pack_uint32_t(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) {
	uint8_t buf[5];
	int len = vl_pack_uint32_t_pad4(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) {
	uint8_t buf[5];
	int len = vl_pack_int32_t(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
		m_file.write(buf, pos, mlen);
	}
	return len;
}

int UByteArrayAdapterPrivateChunkedMmappedFile::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) {
	uint8_t buf[5];
	int len = vl_pack_int32_t_pad4(value, buf);
	if (len > 0) {
		uint32_t mlen = len;
		m_file.write(buf, pos, mlen);
	}
	return len;
}

void UByteArrayAdapterPrivateChunkedMmappedFile::put(sserialize::UByteArrayAdapter::OffsetType pos, const uint8_t * src, sserialize::UByteArrayAdapter::OffsetType len) {
	uint32_t mightOverFlow = len;
	m_file.write(src, pos, mightOverFlow);
}


}//end namespace