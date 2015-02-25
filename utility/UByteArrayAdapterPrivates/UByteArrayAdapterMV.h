#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_MV_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_MV_H
#include "UByteArrayAdapterPrivateArray.h"

namespace sserialize {

class UByteArrayAdapterPrivateMV: public UByteArrayAdapterPrivateArray {
	UByteArrayAdapter::MemoryView m_data;
public:
	UByteArrayAdapterPrivateMV(const UByteArrayAdapter::MemoryView & d) : UByteArrayAdapterPrivateArray(0), m_data(d) {
		if (m_data.size()) {
			this->data() = m_data.data();
		}
	}
	virtual ~UByteArrayAdapterPrivateMV() {}
	
	virtual UByteArrayAdapter::OffsetType size() const { return m_data.size(); }
	
	virtual void setDeleteOnClose(bool /*del*/) override {}

	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType /*size*/) override {
		return false;
	}

	virtual bool growStorage(UByteArrayAdapter::OffsetType /*size*/) override {
		return false;
	}
};

}

#endif
