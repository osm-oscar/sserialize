#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_VECTOR_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_VECTOR_H
#include "UByteArrayAdapterPrivateContainer.h"
#include "UByteArrayAdapterPrivateArray.h"
#include <vector>

namespace sserialize {

#if 1
class UByteArrayAdapterPrivateVector: public UByteArrayAdapterPrivateArray {
	std::vector<uint8_t> * m_data;
public:
	UByteArrayAdapterPrivateVector(std::vector<uint8_t> * data) : UByteArrayAdapterPrivateArray(0), m_data(data) {
		if (m_data->size())
			this->data() = &(*m_data)[0];
	}
	virtual ~UByteArrayAdapterPrivateVector() {
		if (m_deleteOnClose) {
			delete m_data;
			m_deleteOnClose = false;
		}
	}

	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType size) {
		if (m_data->size() < size)
			size = m_data->size();
		m_data->resize(m_data->size() - size);
		if (m_data->size())
			data() = &(*m_data)[0];
		return true;
	}

	virtual bool growStorage(UByteArrayAdapter::OffsetType size) {
		if (m_data->size() < size) {
			m_data->resize(size);
			data() = &(*m_data)[0];
		}
		return true;
	}
};
#else
	typedef UByteArrayAdapterPrivateContainer< std::vector<uint8_t> > UByteArrayAdapterPrivateVector;
#endif
}

#endif
