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
	virtual UByteArrayAdapter::OffsetType size() const override;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL bool isContiguous() const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE { return true; }
	
	/** Shrink data to size bytes */
	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType /*size*/) override { return false; }
	/** grow data to at least! size bytes */
	virtual bool growStorage(UByteArrayAdapter::OffsetType /*size*/) override { return false; }

	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL uint8_t & operator[](UByteArrayAdapter::OffsetType pos) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL const uint8_t & operator[](UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;

	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int64_t getInt64(UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL uint64_t getUint64(UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int32_t getInt32(UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL uint32_t getUint32(UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL uint32_t getUint24(UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL uint16_t getUint16(UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL uint8_t getUint8(UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL UByteArrayAdapter::NegativeOffsetType getNegativeOffset(UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL UByteArrayAdapter::OffsetType getOffset(UByteArrayAdapter::OffsetType pos) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;

	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL uint64_t getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int64_t getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL uint32_t getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int32_t getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void get(sserialize::UByteArrayAdapter::OffsetType pos, uint8_t * dest, sserialize::UByteArrayAdapter::OffsetType len) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;

	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL std::string getString(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;


	/** If the supplied memory is not writable then you're on your own! **/
	
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;

	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;

	
	/** @return: Length of the number, -1 on failure **/
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType maxLen) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType maxLen) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL int putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
	
	SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL void put(UByteArrayAdapter::OffsetType pos, const uint8_t * src, UByteArrayAdapter::OffsetType len) SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE;
};

}

#endif