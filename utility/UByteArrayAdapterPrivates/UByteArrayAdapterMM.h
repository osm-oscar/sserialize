#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_MM_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_MM_H
#include "UByteArrayAdapterPrivateArray.h"
#include <sserialize/utility/MmappedMemory.h>

namespace sserialize {

class UByteArrayAdapterPrivateMM: public UByteArrayAdapterPrivateArray {
	MmappedMemory<uint8_t> m_data;
public:
	UByteArrayAdapterPrivateMM(const MmappedMemory<uint8_t> & d) : UByteArrayAdapterPrivateArray(0), m_data(d) {
		if (m_data.size()) {
			this->data() = m_data.data();
		}
	}
	virtual ~UByteArrayAdapterPrivateMM() {}

	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType size) {
		if (m_data.size() < size)
			size = m_data.size();
		m_data.resize(m_data.size() - size);
		if (m_data.size())
			data() = m_data.data();
		return true;
	}

	virtual bool growStorage(UByteArrayAdapter::OffsetType size) {
		if (m_data.size() < size) {
			m_data.resize(size);
			data() = m_data.data();
		}
		return true;
	}
};

}

#endif
