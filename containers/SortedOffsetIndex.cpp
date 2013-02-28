#include "SortedOffsetIndex.h"
#include "SortedOffsetIndexPrivate.h"

namespace sserialize {
namespace Static {

SortedOffsetIndex::SortedOffsetIndex() : MyParentClass(new SortedOffsetIndexPrivateEmpty()) {}

SortedOffsetIndex::SortedOffsetIndex(const UByteArrayAdapter & data) : MyParentClass(new SortedOffsetIndexPrivate(data)) {}
SortedOffsetIndex::SortedOffsetIndex(const SortedOffsetIndex & other) : MyParentClass(other) {}
SortedOffsetIndex::~SortedOffsetIndex() {}
SortedOffsetIndex & SortedOffsetIndex::operator=(const SortedOffsetIndex & other) {
	MyParentClass::operator=(other);
	return *this;
}

UByteArrayAdapter::OffsetType SortedOffsetIndex::getSizeInBytes() const {
	return priv()->getSizeInBytes();
}

uint32_t SortedOffsetIndex::size() const {
	return priv()->size();
}

UByteArrayAdapter::OffsetType SortedOffsetIndex::at(uint32_t pos) const {
	return priv()->at(pos);
}

}}//end namespace