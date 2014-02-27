#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_ARRAY_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_ARRAY_H
#include "UByteArrayAdapterPrivate.h"

namespace sserialize {

class UByteArrayAdapterPrivateArray: public UByteArrayAdapterPrivate {
private:
	uint8_t * m_data;
protected:
	uint8_t* & data();
public:
    UByteArrayAdapterPrivateArray(uint8_t * data) : UByteArrayAdapterPrivate(), m_data(data) {}
	virtual ~UByteArrayAdapterPrivateArray();
	virtual UByteArrayAdapter::OffsetType size() const;
	
	/** Shrink data to size bytes */
	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType /*size*/) { return false; }
	/** grow data to at least! size bytes */
	virtual bool growStorage(UByteArrayAdapter::OffsetType /*size*/) { return false; }
	

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
	
	void get(sserialize::UByteArrayAdapter::OffsetType pos, uint8_t * dest, sserialize::UByteArrayAdapter::OffsetType len) const;

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

}

#endif