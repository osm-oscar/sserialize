#include "UByteArrayAdapterPrivateArray.h"
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>

#if defined(SSERIALIZE_UBA_ONLY_CONTIGUOUS) && defined(INLINE_WITH_LTO)
	#define UBAA_INLINE_WITH_LTO INLINE_WITH_LTO
#else
	#define UBAA_INLINE_WITH_LTO
#endif

namespace sserialize {

UByteArrayAdapterPrivateArray::~UByteArrayAdapterPrivateArray() {
	if (m_deleteOnClose) {
		delete[] m_data;
	}
}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateArray::size() const {
	throw sserialize::UnimplementedFunctionException("UByteArrayAdapterPrivateArray::size");
}

UBAA_INLINE_WITH_LTO
uint8_t*& UByteArrayAdapterPrivateArray::data() {
	return m_data;
}

UBAA_INLINE_WITH_LTO
uint8_t & UByteArrayAdapterPrivateArray::operator[](UByteArrayAdapter::OffsetType pos) {
	return m_data[pos];
}

UBAA_INLINE_WITH_LTO
const uint8_t & UByteArrayAdapterPrivateArray::operator[](UByteArrayAdapter::OffsetType pos) const {
	return m_data[pos];
}

UBAA_INLINE_WITH_LTO
int64_t UByteArrayAdapterPrivateArray::getInt64(UByteArrayAdapter::OffsetType pos) const {
	return up_s64(&m_data[pos]);
}

UBAA_INLINE_WITH_LTO
uint64_t UByteArrayAdapterPrivateArray::getUint64(UByteArrayAdapter::OffsetType pos) const {
	return up_u64(&m_data[pos]);
}

UBAA_INLINE_WITH_LTO
int32_t UByteArrayAdapterPrivateArray::getInt32(UByteArrayAdapter::OffsetType pos) const {
	return up_s32(&m_data[pos]);
}

UBAA_INLINE_WITH_LTO
uint32_t UByteArrayAdapterPrivateArray::getUint32(UByteArrayAdapter::OffsetType pos) const {
	return up_u32(m_data+pos);
}

UBAA_INLINE_WITH_LTO
uint32_t UByteArrayAdapterPrivateArray::getUint24(UByteArrayAdapter::OffsetType pos) const {
	return up_u24(&m_data[pos]);
}

UBAA_INLINE_WITH_LTO
uint16_t UByteArrayAdapterPrivateArray::getUint16(UByteArrayAdapter::OffsetType pos) const {
	return up_u16(&m_data[pos]);
}

UBAA_INLINE_WITH_LTO
uint8_t UByteArrayAdapterPrivateArray::getUint8(UByteArrayAdapter::OffsetType pos) const {
	return m_data[pos];
}

UBAA_INLINE_WITH_LTO
UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateArray::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	return up_s40(&m_data[pos]);
}

UBAA_INLINE_WITH_LTO
UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateArray::getOffset(UByteArrayAdapter::OffsetType pos) const {
	return up_u40(&m_data[pos]);
}

UBAA_INLINE_WITH_LTO
uint64_t UByteArrayAdapterPrivateArray::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	return up_vu64(&(m_data[pos]), length);
}

UBAA_INLINE_WITH_LTO
int64_t UByteArrayAdapterPrivateArray::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	return up_vs64(&(m_data[pos]), length);
}

UBAA_INLINE_WITH_LTO
uint32_t UByteArrayAdapterPrivateArray::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	return up_vu32(&(m_data[pos]), length);
}

UBAA_INLINE_WITH_LTO
int32_t UByteArrayAdapterPrivateArray::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	return up_vs32(&(m_data[pos]), length);
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	uint8_t * start = m_data+pos;
	memmove(dest, start, sizeof(uint8_t)*len);
}

UBAA_INLINE_WITH_LTO
std::string UByteArrayAdapterPrivateArray::getString(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const {
    return std::string(m_data+pos, m_data+pos+len);
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	p_u64(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	p_s64(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	p_s32(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	p_u32(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	p_u24(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value)  {
	p_u16(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
	m_data[pos] = value;
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	p_u40(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	p_s40(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
int UByteArrayAdapterPrivateArray::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return p_vu64(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
int UByteArrayAdapterPrivateArray::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return p_vs64(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
int UByteArrayAdapterPrivateArray::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return p_vu32(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
int UByteArrayAdapterPrivateArray::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return p_vu32pad4(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
int UByteArrayAdapterPrivateArray::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return p_vs32(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
int UByteArrayAdapterPrivateArray::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	return p_vs32pad4(value, &(m_data[pos]));
}

UBAA_INLINE_WITH_LTO
void UByteArrayAdapterPrivateArray::put(sserialize::UByteArrayAdapter::OffsetType pos, const uint8_t * src, sserialize::UByteArrayAdapter::OffsetType len) {
	uint8_t * start = m_data+pos;
	memmove(start, src, sizeof(uint8_t)*len);
}


}//end namespace
