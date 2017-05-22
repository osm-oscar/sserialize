#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_MM_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_MM_H
#include "UByteArrayAdapterPrivateArray.h"
#include <sserialize/storage/MmappedMemory.h>

namespace sserialize {

class UByteArrayAdapterPrivateMM: public UByteArrayAdapterPrivateArray {
	MmappedMemory<uint8_t> m_data;
public:
	UByteArrayAdapterPrivateMM(const MmappedMemory<uint8_t> & d);
	virtual ~UByteArrayAdapterPrivateMM();
	
	virtual void setDeleteOnClose(bool /*del*/) override;

	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType size) override;
	virtual bool growStorage(UByteArrayAdapter::OffsetType size) override;
};

}

#endif
