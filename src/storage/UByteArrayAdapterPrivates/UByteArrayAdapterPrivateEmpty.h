#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_EMPTY_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_EMPTY_H
#include "UByteArrayAdapterPrivate.h"
#include "UByteArrayAdapterPrivateArray.h"

namespace sserialize {
#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS
inline
#endif
namespace UByteArrayAdapterOnlyContiguous {

class UByteArrayAdapterPrivateEmpty: public UByteArrayAdapterPrivateArray {
	uint8_t m_default;
public:
	UByteArrayAdapterPrivateEmpty() : UByteArrayAdapterPrivateArray(&m_default), m_default(0) {}
	virtual ~UByteArrayAdapterPrivateEmpty() {}
	virtual UByteArrayAdapter::OffsetType size() const { return 0; }
};

} //end namespace UByteArrayAdapterOnlyContiguous

#ifndef SSERIALIZE_UBA_ONLY_CONTIGUOUS
inline
#endif
namespace UByteArrayAdapterNonContiguous {

class UByteArrayAdapterPrivateEmpty: public UByteArrayAdapterPrivate {
	uint8_t m_default;
public:
	UByteArrayAdapterPrivateEmpty() : UByteArrayAdapterPrivate(), m_default(0) {}
	virtual ~UByteArrayAdapterPrivateEmpty() {}
	virtual UByteArrayAdapter::OffsetType size() const { return 0; }

	/** Shrink data to size bytes */
	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType /*size*/) { return false; }
	/** grow data to at least! size bytes */
	virtual bool growStorage(UByteArrayAdapter::OffsetType /*size*/) { return false; }

//Access functions
	virtual uint8_t & operator[](UByteArrayAdapter::OffsetType /*pos*/) { return m_default; }
	virtual const uint8_t & operator[](UByteArrayAdapter::OffsetType /*pos*/) const { return m_default; }

	virtual int64_t getInt64(UByteArrayAdapter::OffsetType /*pos*/) const { return 0; };
	virtual uint64_t getUint64(UByteArrayAdapter::OffsetType /*pos*/) const { return 0; };

	virtual int32_t getInt32(UByteArrayAdapter::OffsetType /*pos*/) const { return 0; };
	virtual uint32_t getUint32(UByteArrayAdapter::OffsetType /*pos*/) const { return 0; };
	virtual uint32_t getUint24(UByteArrayAdapter::OffsetType /*pos*/) const { return 0; };
	virtual uint16_t getUint16(UByteArrayAdapter::OffsetType /*pos*/) const { return 0; };
	virtual uint8_t getUint8(UByteArrayAdapter::OffsetType /*pos*/) const { return 0; };
	
	virtual UByteArrayAdapter::NegativeOffsetType getNegativeOffset(UByteArrayAdapter::OffsetType /*pos*/) const { return 0; };
	virtual UByteArrayAdapter::OffsetType getOffset(UByteArrayAdapter::OffsetType /*pos*/) const { return 0; };
	
	virtual uint64_t getVlPackedUint64(UByteArrayAdapter::OffsetType /*pos*/, int * /*length*/) const { return 0; };
	virtual int64_t getVlPackedInt64(UByteArrayAdapter::OffsetType /*pos*/, int * /*length*/) const { return 0; };
	virtual uint32_t getVlPackedUint32(UByteArrayAdapter::OffsetType /*pos*/, int * /*length*/) const { return 0; };
	virtual int32_t getVlPackedInt32(UByteArrayAdapter::OffsetType /*pos*/, int * /*length*/) const { return 0; };
	
	virtual void get(UByteArrayAdapter::OffsetType /*pos*/, uint8_t * /*dest*/, UByteArrayAdapter::OffsetType /*length*/) const {};

	/** If the supplied memory is not writable then you're on your own! **/

	virtual void putInt64(UByteArrayAdapter::OffsetType /*pos*/, int64_t /*value*/) {};
	virtual void putUint64(UByteArrayAdapter::OffsetType /*pos*/, uint64_t /*value*/) {};
	virtual void putInt32(UByteArrayAdapter::OffsetType /*pos*/, int32_t /*value*/) {};
	virtual void putUint32(UByteArrayAdapter::OffsetType /*pos*/, uint32_t /*value*/) {};
	virtual void putUint24(UByteArrayAdapter::OffsetType /*pos*/, uint32_t /*value*/) {};
	virtual void putUint16(UByteArrayAdapter::OffsetType /*pos*/, uint16_t /*value*/) {};
	virtual void putUint8(UByteArrayAdapter::OffsetType /*pos*/, uint8_t /*value*/) {};
	
	
	virtual void putOffset(UByteArrayAdapter::OffsetType /*pos*/, UByteArrayAdapter::OffsetType /*value*/) {};
	virtual void putNegativeOffset(UByteArrayAdapter::OffsetType /*pos*/, UByteArrayAdapter::NegativeOffsetType /*value*/) {};
	
	/** @return: Length of the number, -1 on failure **/
	virtual int putVlPackedUint64(UByteArrayAdapter::OffsetType /*pos*/, uint64_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) { return -1; };
	virtual int putVlPackedInt64(UByteArrayAdapter::OffsetType /*pos*/, int64_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) { return -1; };
	virtual int putVlPackedUint32(UByteArrayAdapter::OffsetType /*pos*/, uint32_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) { return -1; };
	virtual int putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType /*pos*/, uint32_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) { return -1; };
	virtual int putVlPackedInt32(UByteArrayAdapter::OffsetType /*pos*/, int32_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) { return -1; };
	virtual int putVlPackedPad4Int32(UByteArrayAdapter::OffsetType /*pos*/, int32_t /*value*/, UByteArrayAdapter::OffsetType /*maxLen*/) { return -1; };
	
	virtual void put(UByteArrayAdapter::OffsetType /*pos*/, const uint8_t * /*src*/, UByteArrayAdapter::OffsetType /*len*/) {};
};

} //end namespace UByteArrayAdapterNonContiguous
}//end namespace sserialize

#endif