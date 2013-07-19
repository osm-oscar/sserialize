#include "UByteArrayAdapterPrivateMmappedFile.h"

namespace sserialize {

UByteArrayAdapterPrivateMmappedFile::UByteArrayAdapterPrivateMmappedFile(sserialize::MmappedFile& file) :
UByteArrayAdapterPrivateArray(file.data()), m_file(file)
{
	UByteArrayAdapterPrivate::setDeleteOnClose(false);
}

UByteArrayAdapterPrivateMmappedFile::~UByteArrayAdapterPrivateMmappedFile() {}

UByteArrayAdapter::OffsetType UByteArrayAdapterPrivateMmappedFile::size() const {
	return m_file.size();
}


void UByteArrayAdapterPrivateMmappedFile::setDeleteOnClose(bool del) {
	m_file.setDeleteOnClose(del);
}

bool UByteArrayAdapterPrivateMmappedFile::growStorage(UByteArrayAdapter::OffsetType size) {
	if (m_file.size() < size) {
		bool ok = m_file.resize(size);
		data() = m_file.data();
		return ok;
	}
	return true;
}

bool UByteArrayAdapterPrivateMmappedFile::shrinkStorage(UByteArrayAdapter::OffsetType size) {
	bool ok = m_file.resize(size);
	data() = m_file.data();
	return ok;
}


}//end namespace
