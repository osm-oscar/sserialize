#include "UByteArrayAdapterMM.h"

namespace sserialize {

UByteArrayAdapterPrivateMM::UByteArrayAdapterPrivateMM(const MmappedMemory<uint8_t> & d) :
UByteArrayAdapterPrivateArray(0),
m_data(d)
{
	if (m_data.size()) {
		this->data() = m_data.data();
	}
}

UByteArrayAdapterPrivateMM::~UByteArrayAdapterPrivateMM() {}

void UByteArrayAdapterPrivateMM::setDeleteOnClose(bool /*del*/) {}

bool UByteArrayAdapterPrivateMM::shrinkStorage(UByteArrayAdapter::OffsetType size) {
	if (m_data.size() < size)
		size = m_data.size();
	m_data.resize(m_data.size() - size);
	if (m_data.size())
		data() = m_data.data();
	return true;
}

bool UByteArrayAdapterPrivateMM::growStorage(UByteArrayAdapter::OffsetType size) {
	if (m_data.size() < size) {
		m_data.resize(size);
		data() = m_data.data();
	}
	return true;
}

}//end namespace sserialize