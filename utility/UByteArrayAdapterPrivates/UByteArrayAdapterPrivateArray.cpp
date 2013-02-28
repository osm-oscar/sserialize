#include "UByteArrayAdapterPrivateArray.h"
#include <sserialize/utility/pack_unpack_functions.h>

namespace sserialize {

UByteArrayAdapterPrivateArray::~UByteArrayAdapterPrivateArray() {
	if (m_deleteOnClose) {
		delete[] m_data;
	}
}

uint8_t*& UByteArrayAdapterPrivateArray::data() {
	return m_data;
}

uint8_t & UByteArrayAdapterPrivateArray::operator[](UByteArrayAdapter::OffsetType pos) {
	return m_data[pos];
}

const uint8_t & UByteArrayAdapterPrivateArray::operator[](UByteArrayAdapter::OffsetType pos) const {
	return m_data[pos];
}

int64_t UByteArrayAdapterPrivateArray::getInt64(UByteArrayAdapter::OffsetType pos) const {
	return unPack_int64_t(&m_data[pos]);
}

uint64_t UByteArrayAdapterPrivateArray::getUint64(UByteArrayAdapter::OffsetType pos) const {
	return unPack_uint64_t(&m_data[pos]);
}

int32_t UByteArrayAdapterPrivateArray::getInt32(UByteArrayAdapter::OffsetType pos) const {
	return unPack_int32_t(m_data[pos], m_data[pos+1], m_data[pos+2], m_data[pos+3]);
}

uint32_t UByteArrayAdapterPrivateArray::getUint32(UByteArrayAdapter::OffsetType pos) const {
	return unPack_uint32_t(m_data+pos);
}

uint32_t UByteArrayAdapterPrivateArray::getUint24(UByteArrayAdapter::OffsetType pos) const {
	return unPack_uint24_t(m_data[pos], m_data[pos+1], m_data[pos+2]);
}

uint16_t UByteArrayAdapterPrivateArray::getUint16(UByteArrayAdapter::OffsetType pos) const {
	return unPack_uint16_t(m_data[pos], m_data[pos+1]);
}

uint8_t UByteArrayAdapterPrivateArray::getUint8(UByteArrayAdapter::OffsetType pos) const {
	return m_data[pos];
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateArray::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	return unPack_int40_t(&m_data[pos]);
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateArray::getOffset(UByteArrayAdapter::OffsetType pos) const {
	return unPack_uint40_t(&m_data[pos]);
}

uint64_t UByteArrayAdapterPrivateArray::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	return vl_unpack_uint64_t(&(m_data[pos]), length);
}

int64_t UByteArrayAdapterPrivateArray::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	return vl_unpack_int64_t(&(m_data[pos]), length);
}


uint32_t UByteArrayAdapterPrivateArray::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	return vl_unpack_uint32_t(&(m_data[pos]), length);
}

int32_t UByteArrayAdapterPrivateArray::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	return vl_unpack_int32_t(&(m_data[pos]), length);
}

void UByteArrayAdapterPrivateArray::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	uint8_t * start = m_data+pos;
	memmove(dest, start, sizeof(uint8_t)*len);
}


void UByteArrayAdapterPrivateArray::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	pack_uint64_t(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	pack_int64_t(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	pack_int32_t(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	pack_uint32_t(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	pack_uint24_t(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value)  {
	pack_uint16_t(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
	m_data[pos] = value;
}

void UByteArrayAdapterPrivateArray::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	pack_uint40_t(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	pack_int40_t(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType maxLen) {
	return vl_pack_uint64_t(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType maxLen) {
	return vl_pack_int64_t(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) {
	return vl_pack_uint32_t(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) {
	return vl_pack_uint32_t_pad4(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) {
	return vl_pack_int32_t(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) {
	return vl_pack_int32_t_pad4(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::put(sserialize::UByteArrayAdapter::OffsetType pos, const uint8_t * src, sserialize::UByteArrayAdapter::OffsetType len) {
	uint8_t * start = m_data+pos;
	memmove(start, src, sizeof(uint8_t)*len);
}


}//end namespace