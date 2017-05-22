#include "UByteArrayAdapterPrivateVector.h"

namespace sserialize {

UByteArrayAdapterPrivateVector::UByteArrayAdapterPrivateVector(std::vector<uint8_t> * data) : UByteArrayAdapterPrivateArray(0), m_data(data) {
	if (m_data->size())
		this->data() = &(*m_data)[0];
}

UByteArrayAdapterPrivateVector::~UByteArrayAdapterPrivateVector() {
	if (m_deleteOnClose) {
		delete m_data;
		m_data = 0;
		m_deleteOnClose = false;
		this->data() = 0;
	}
}

OffsetType UByteArrayAdapterPrivateVector::size() const {
	return m_data->size();
}

bool UByteArrayAdapterPrivateVector::shrinkStorage(UByteArrayAdapter::OffsetType size) {
	if (m_data->size() < size)
		size = m_data->size();
	m_data->resize(m_data->size() - size);
	if (m_data->size())
		data() = &(*m_data)[0];
	return true;
}

bool UByteArrayAdapterPrivateVector::growStorage(UByteArrayAdapter::OffsetType size) {
	if (m_data->size() < size) {
		m_data->resize(size);
		data() = &(*m_data)[0];
	}
	return true;
}

} //end namespace sserialize