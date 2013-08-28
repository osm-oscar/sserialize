#include "UByteArrayAdapterPrivateArray.h"
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {

UByteArrayAdapterPrivateArray::~UByteArrayAdapterPrivateArray() {
	if (m_deleteOnClose) {
		delete[] m_data;
	}
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateArray::size() const {
	throw sserialize::UnimplementedFunctionException("UByteArrayAdapterPrivateArray::size");
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
	return up_s64(&m_data[pos]);
}

uint64_t UByteArrayAdapterPrivateArray::getUint64(UByteArrayAdapter::OffsetType pos) const {
	return up_u64(&m_data[pos]);
}

int32_t UByteArrayAdapterPrivateArray::getInt32(UByteArrayAdapter::OffsetType pos) const {
	return up_s32(&m_data[pos]);
}

uint32_t UByteArrayAdapterPrivateArray::getUint32(UByteArrayAdapter::OffsetType pos) const {
	return up_u32(m_data+pos);
}

uint32_t UByteArrayAdapterPrivateArray::getUint24(UByteArrayAdapter::OffsetType pos) const {
	return up_u24(&m_data[pos]);
}

uint16_t UByteArrayAdapterPrivateArray::getUint16(UByteArrayAdapter::OffsetType pos) const {
	return up_u16(&m_data[pos]);
}

uint8_t UByteArrayAdapterPrivateArray::getUint8(UByteArrayAdapter::OffsetType pos) const {
	return m_data[pos];
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateArray::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	return up_s40(&m_data[pos]);
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateArray::getOffset(UByteArrayAdapter::OffsetType pos) const {
	return up_u40(&m_data[pos]);
}

uint64_t UByteArrayAdapterPrivateArray::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	return up_vu64(&(m_data[pos]), length);
}

int64_t UByteArrayAdapterPrivateArray::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	return up_vs64(&(m_data[pos]), length);
}


uint32_t UByteArrayAdapterPrivateArray::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	return up_vu32(&(m_data[pos]), length);
}

int32_t UByteArrayAdapterPrivateArray::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	return up_vs32(&(m_data[pos]), length);
}

void UByteArrayAdapterPrivateArray::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	uint8_t * start = m_data+pos;
	memmove(dest, start, sizeof(uint8_t)*len);
}


void UByteArrayAdapterPrivateArray::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	p_u64(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	p_s64(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	p_s32(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	p_u32(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	p_u24(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value)  {
	p_u16(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
	m_data[pos] = value;
}

void UByteArrayAdapterPrivateArray::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	p_u40(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	p_s40(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType maxLen) {
	return p_vu64(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType maxLen) {
	return p_vs64(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) {
	return p_vu32(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) {
	return p_vu32pad4(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) {
	return p_vs32(value, &(m_data[pos]));
}

int UByteArrayAdapterPrivateArray::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) {
	return p_vs32pad4(value, &(m_data[pos]));
}

void UByteArrayAdapterPrivateArray::put(sserialize::UByteArrayAdapter::OffsetType pos, const uint8_t * src, sserialize::UByteArrayAdapter::OffsetType len) {
	uint8_t * start = m_data+pos;
	memmove(start, src, sizeof(uint8_t)*len);
}


}//end namespace