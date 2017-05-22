#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_MV_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_MV_H
#include "UByteArrayAdapterPrivateArray.h"

namespace sserialize {

class UByteArrayAdapterPrivateMV: public UByteArrayAdapterPrivateArray {
	UByteArrayAdapter::MemoryView m_data;
public:
	UByteArrayAdapterPrivateMV(const UByteArrayAdapter::MemoryView & d);
	virtual ~UByteArrayAdapterPrivateMV();
	
	virtual UByteArrayAdapter::OffsetType size() const override;
	
	virtual void setDeleteOnClose(bool /*del*/) override;

	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType /*size*/) override;
	virtual bool growStorage(UByteArrayAdapter::OffsetType /*size*/) override;
};

}

#endif
