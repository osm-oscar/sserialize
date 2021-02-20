#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>

namespace sserialize {
namespace Static {

SortedOffsetIndex::SortedOffsetIndex() : MyParentClass(new SortedOffsetIndexPrivateEmpty()) {}

SortedOffsetIndex::SortedOffsetIndex(const UByteArrayAdapter & data) :
MyParentClass(new SortedOffsetIndexPrivate(data))
{}

SortedOffsetIndex::SortedOffsetIndex(UByteArrayAdapter & data, UByteArrayAdapter::ConsumeTag) :
SortedOffsetIndex(data) {
	data += getSizeInBytes();
}

SortedOffsetIndex::SortedOffsetIndex(UByteArrayAdapter const & data, UByteArrayAdapter::NoConsumeTag) :
SortedOffsetIndex(data)
{}

SortedOffsetIndex::SortedOffsetIndex(const SortedOffsetIndex & other) :
MyParentClass(other)
{}

SortedOffsetIndex::~SortedOffsetIndex() {}

SortedOffsetIndex & SortedOffsetIndex::operator=(const SortedOffsetIndex & other) {
	MyParentClass::operator=(other);
	return *this;
}

UByteArrayAdapter::OffsetType SortedOffsetIndex::getSizeInBytes() const {
	return priv()->getSizeInBytes();
}

SizeType SortedOffsetIndex::size() const {
	return priv()->size();
}

UByteArrayAdapter::OffsetType SortedOffsetIndex::at(SizeType pos) const {
	return priv()->at(pos);
}

}}//end namespace
