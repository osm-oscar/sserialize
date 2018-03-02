#include "UByteArrayAdapterPrivateMmappedFile.h"

namespace sserialize {

UByteArrayAdapterPrivateMmappedFile::UByteArrayAdapterPrivateMmappedFile(sserialize::MmappedFile file) :
UByteArrayAdapterPrivateArray(file.data()), m_file(file)
{
	UByteArrayAdapterPrivate::setDeleteOnClose(false);
}

UByteArrayAdapterPrivateMmappedFile::~UByteArrayAdapterPrivateMmappedFile() {}


void UByteArrayAdapterPrivateMmappedFile::advice(UByteArrayAdapter::AdviseType at, UByteArrayAdapter::SizeType begin, UByteArrayAdapter::SizeType end) {
	switch(at) {
	case UByteArrayAdapter::AT_READ:
	case UByteArrayAdapter::AT_LOAD:
		m_file.cache(begin, end-begin);
		break;
	case UByteArrayAdapter::AT_DROP:
		m_file.drop(begin, end-begin);
		break;
	case UByteArrayAdapter::AT_LOCK:
		m_file.lock(begin, end-begin);
		break;
	case UByteArrayAdapter::AT_UNLOCK:
		m_file.unlock(begin, end-begin);
		break;
	default:
		break;
	};
}

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
