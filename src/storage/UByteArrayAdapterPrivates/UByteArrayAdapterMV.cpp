#include "UByteArrayAdapterMV.h"

namespace sserialize {

UByteArrayAdapterPrivateMV::UByteArrayAdapterPrivateMV(const UByteArrayAdapter::MemoryView & d) :
UByteArrayAdapterPrivateArray(0),
m_data(d)
{
	if (m_data.size()) {
		this->data() = m_data.data();
	}
}
UByteArrayAdapterPrivateMV::~UByteArrayAdapterPrivateMV() {}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateMV::size() const {
	return m_data.size();
}

void UByteArrayAdapterPrivateMV::setDeleteOnClose(bool /*del*/) {}

bool UByteArrayAdapterPrivateMV::shrinkStorage(UByteArrayAdapter::OffsetType /*size*/) {
	return false;
}

bool UByteArrayAdapterPrivateMV::growStorage(UByteArrayAdapter::OffsetType /*size*/) {
	return false;
}

}//end namespace sserialize