#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_CONTAINER_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_CONTAINER_H
#include "UByteArrayAdapterPrivate.h"
#include <sserialize/utility/pack_unpack_functions.h>
#include <array>

namespace sserialize {

template<typename T_CONTAINER>
class UByteArrayAdapterPrivateContainer: public UByteArrayAdapterPrivate {
private:
	T_CONTAINER * m_data;
public:
    UByteArrayAdapterPrivateContainer(T_CONTAINER * data) : UByteArrayAdapterPrivate(), m_data(data) {}
    virtual ~UByteArrayAdapterPrivateContainer();
	virtual sserialize::UByteArrayAdapter::OffsetType size() const;

	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType size);
	virtual bool growStorage(UByteArrayAdapter::OffsetType size);

	virtual uint8_t & operator[](UByteArrayAdapter::OffsetType pos);
	virtual const uint8_t & operator[](UByteArrayAdapter::OffsetType pos) const;
	
	virtual int64_t getInt64(UByteArrayAdapter::OffsetType pos) const;
	virtual uint64_t getUint64(UByteArrayAdapter::OffsetType pos) const;

	virtual int32_t getInt32(UByteArrayAdapter::OffsetType pos) const;
	virtual uint32_t getUint32(UByteArrayAdapter::OffsetType pos) const;
	virtual uint32_t getUint24(UByteArrayAdapter::OffsetType pos) const;
	virtual uint16_t getUint16(UByteArrayAdapter::OffsetType pos) const;
	virtual uint8_t getUint8(UByteArrayAdapter::OffsetType pos) const;

	virtual UByteArrayAdapter::NegativeOffsetType getNegativeOffset(UByteArrayAdapter::OffsetType pos) const;
	virtual UByteArrayAdapter::OffsetType getOffset(UByteArrayAdapter::OffsetType pos) const;
	
	virtual uint64_t getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const;
	virtual int64_t getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const;
	virtual uint32_t getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const;
	virtual int32_t getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const;
	
	virtual void get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const;

	/** If the supplied memory is not writable then you're on your own! **/
	virtual void putInt64(UByteArrayAdapter::OffsetType pos, int64_t value);
	virtual void putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value);
	virtual void putInt32(UByteArrayAdapter::OffsetType pos, int32_t value);
	virtual void putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value);
	virtual void putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value);
	virtual void putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value);
	virtual void putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value);

	virtual void putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value);
	virtual void putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value);
	
	/** @return: Length of the number, -1 on failure **/
	virtual int putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType maxLen);
	virtual int putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType maxLen);
	virtual int putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen);
	virtual int putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen);
	virtual int putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen);
	virtual int putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen);
	
	virtual void put(UByteArrayAdapter::OffsetType pos, const uint8_t * src, UByteArrayAdapter::OffsetType len);
};

template<typename T_CONTAINER>
UByteArrayAdapterPrivateContainer<T_CONTAINER>::~UByteArrayAdapterPrivateContainer() {
	if (m_deleteOnClose) {
		delete m_data;
	}
}

template<typename T_CONTAINER>
UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateContainer<T_CONTAINER>::size() const {
	return m_data->size();
}

template<typename T_CONTAINER>
bool UByteArrayAdapterPrivateContainer<T_CONTAINER>::shrinkStorage(UByteArrayAdapter::OffsetType size) {
	if (m_data->size() < size)
		size = m_data->size();
	m_data->resize(m_data->size() - size);
	return true;
}

template<typename T_CONTAINER>
bool UByteArrayAdapterPrivateContainer<T_CONTAINER>::growStorage(UByteArrayAdapter::OffsetType size) {
	if (m_data->size() < size) {
		m_data->resize(size);
	}
	return true;
}

template<typename T_CONTAINER>
uint8_t & UByteArrayAdapterPrivateContainer<T_CONTAINER>::operator[](UByteArrayAdapter::OffsetType pos) {
	return (*m_data)[pos];
}

template<typename T_CONTAINER>
const uint8_t & UByteArrayAdapterPrivateContainer<T_CONTAINER>::operator[](UByteArrayAdapter::OffsetType pos) const {
	return (*m_data)[pos];
}
template<typename T_CONTAINER>
int64_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getInt64(UByteArrayAdapter::OffsetType pos) const {
	uint8_t tmp[] = {(*m_data)[pos], (*m_data)[pos+1], (*m_data)[pos+2], (*m_data)[pos+3],
							(*m_data)[pos+4], (*m_data)[pos+5], (*m_data)[pos+6], (*m_data)[pos+7]};
	return up_s64(tmp);
}

template<typename T_CONTAINER>
uint64_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getUint64(UByteArrayAdapter::OffsetType pos) const {
	uint8_t tmp[] = {(*m_data)[pos], (*m_data)[pos+1], (*m_data)[pos+2], (*m_data)[pos+3],
							(*m_data)[pos+4], (*m_data)[pos+5], (*m_data)[pos+6], (*m_data)[pos+7]};
	return up_u64(tmp);
}


template<typename T_CONTAINER>
int32_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getInt32(UByteArrayAdapter::OffsetType pos) const {
	uint8_t tmp[] = {(*m_data)[pos], (*m_data)[pos+1], (*m_data)[pos+2], (*m_data)[pos+3]};
	return up_s32(tmp);
}

template<typename T_CONTAINER>
uint32_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getUint32(UByteArrayAdapter::OffsetType pos) const {
	uint8_t tmp[] = {(*m_data)[pos], (*m_data)[pos+1], (*m_data)[pos+2], (*m_data)[pos+3]};
	return up_u32(tmp);
}

template<typename T_CONTAINER>
uint32_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getUint24(UByteArrayAdapter::OffsetType pos) const {
	uint8_t tmp[] = {(*m_data)[pos], (*m_data)[pos+1], (*m_data)[pos+2]};
	return up_u24(tmp);
}

template<typename T_CONTAINER>
uint16_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getUint16(UByteArrayAdapter::OffsetType pos) const {
	uint8_t tmp[] = {(*m_data)[pos], (*m_data)[pos+1]};
	return up_u16(tmp);
}

template<typename T_CONTAINER>
uint8_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getUint8(UByteArrayAdapter::OffsetType pos) const {
	return (*m_data)[pos];
}

template<typename T_CONTAINER>
UByteArrayAdapter::NegativeOffsetType UByteArrayAdapterPrivateContainer<T_CONTAINER>::getNegativeOffset(UByteArrayAdapter::OffsetType pos) const {
	uint8_t tmp[] = {(*m_data)[pos], (*m_data)[pos+1], (*m_data)[pos+2], (*m_data)[pos+3], (*m_data)[pos+4]};
	return up_s40(tmp);
}

template<typename T_CONTAINER>
UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateContainer<T_CONTAINER>::getOffset(UByteArrayAdapter::OffsetType pos) const {
	uint8_t tmp[] = {(*m_data)[pos], (*m_data)[pos+1], (*m_data)[pos+2], (*m_data)[pos+3], (*m_data)[pos+4]};
	return up_u40(tmp);
}

template<typename T_CONTAINER>
uint64_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t bufLen = std::min(9, *length);
	*length = bufLen;
	uint8_t buf[bufLen];
	for(uint8_t i = 0; i < bufLen; i++) {
		buf[i] = (*m_data)[pos+i];
	}
	return up_vu64(buf, length);
}

template<typename T_CONTAINER>
int64_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t bufLen = std::min(9, *length);
	uint8_t buf[bufLen];
	for(uint8_t i = 0; i < bufLen; i++) {
		buf[i] = (*m_data)[pos+i];
	}
	return up_vs64(buf, length);
}

template<typename T_CONTAINER>
uint32_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t bufLen = (*length > 5 ? 5 : *length);
	*length = bufLen;
	uint8_t buf[bufLen];
	for(uint8_t i = 0; i < bufLen; i++) {
		buf[i] = (*m_data)[pos+i];
	}
	return up_vu32(buf, length);
}

template<typename T_CONTAINER>
int32_t UByteArrayAdapterPrivateContainer<T_CONTAINER>::getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const {
	uint8_t bufLen = (*length > 5 ? 5 : *length);
	uint8_t buf[bufLen];
	for(uint8_t i = 0; i < bufLen; i++) {
		buf[i] = (*m_data)[pos+i];
	}
	return up_vs32(buf, length);
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	for(UByteArrayAdapter::OffsetType i = 0; i < len; ++i)
		dest[i] = (*m_data)[pos+i];
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) {
	uint8_t buf[8];
	p_s64(value, buf);
	for(uint8_t i = 0; i < 8; i++)
		(*m_data)[pos+i] = buf[i];
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) {
	uint8_t buf[8];
	p_u64(value, buf);
	for(uint8_t i = 0; i < 8; i++) (*m_data)[pos+i] = buf[i];
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) {
	uint8_t buf[4];
	p_s32(value, buf);
	for(uint8_t i = 0; i < 4; i++)
		(*m_data)[pos+i] = buf[i];
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[4];
	p_u32(value, buf);
	for(uint8_t i = 0; i < 4; i++) (*m_data)[pos+i] = buf[i];
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) {
	uint8_t buf[3];
	p_u24(value, buf);
	for(uint8_t i = 0; i < 3; i++) (*m_data)[pos+i] = buf[i];
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value)  {
	uint8_t buf[2];
	p_u16(value, buf);
	for(uint8_t i = 0; i < 2; i++) (*m_data)[pos+i] = buf[i];
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) {
	(*m_data)[pos] = value;
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) {
	uint8_t buf[5];
	p_u40(value, buf);
	for(uint8_t i = 0; i < 5; i++)
		(*m_data)[pos+i] = buf[i];
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) {
	uint8_t buf[5];
	p_s40(value, buf);
	for(uint8_t i = 0; i < 5; i++)
		(*m_data)[pos+i] = buf[i];
}

template<typename T_CONTAINER>
int UByteArrayAdapterPrivateContainer<T_CONTAINER>::putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[9];
	int len = p_vu64(value, buf);
	if (len > 0) {
		for(uint8_t i = 0; i < len; i++) {
			(*m_data)[pos+i] = buf[i];
		}
	}
	return len;
}

template<typename T_CONTAINER>
int UByteArrayAdapterPrivateContainer<T_CONTAINER>::putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[9];
	int len = p_vs64(value, buf);
	if (len > 0) {
		for(uint8_t i = 0; i < len; i++) {
			(*m_data)[pos+i] = buf[i];
		}
	}
	return len;
}

template<typename T_CONTAINER>
int UByteArrayAdapterPrivateContainer<T_CONTAINER>::putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int len = p_vu32(value, buf);
	if (len > 0) {
		for(uint8_t i = 0; i < len; i++) {
			(*m_data)[pos+i] = buf[i];
		}
	}
	return len;
}

template<typename T_CONTAINER>
int UByteArrayAdapterPrivateContainer<T_CONTAINER>::putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int len = p_vu32pad4(value, buf);
	if (len > 0) {
		for(uint8_t i = 0; i < len; i++) {
			(*m_data)[pos+i] = buf[i];
		}
	}
	return len;
}

template<typename T_CONTAINER>
int UByteArrayAdapterPrivateContainer<T_CONTAINER>::putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int len = p_vs32(value, buf);
	if (len > 0) {
		for(uint8_t i = 0; i < len; i++) {
			(*m_data)[pos+i] = buf[i];
		}
	}
	return len;
}

template<typename T_CONTAINER>
int UByteArrayAdapterPrivateContainer<T_CONTAINER>::putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType /*maxLen*/) {
	uint8_t buf[5];
	int len = p_vs32pad4(value, buf);
	if (len > 0) {
		for(uint8_t i = 0; i < len; i++) {
			(*m_data)[pos+i] = buf[i];
		}
	}
	return len;
}

template<typename T_CONTAINER>
void UByteArrayAdapterPrivateContainer<T_CONTAINER>::put(UByteArrayAdapter::OffsetType pos, const uint8_t * src, UByteArrayAdapter::OffsetType len) {
	for(UByteArrayAdapter::OffsetType i = 0; i < len; ++i)
		(*m_data)[pos+i] = src[i];
}


}

#endif
