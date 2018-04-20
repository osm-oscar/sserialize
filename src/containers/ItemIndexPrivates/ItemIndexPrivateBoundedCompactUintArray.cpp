#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateBoundedCompactUintArray.h>

namespace sserialize {
	

ItemIndexPrivateBoundedCompactUintArray::
ItemIndexPrivateBoundedCompactUintArray() {}

ItemIndexPrivateBoundedCompactUintArray::
ItemIndexPrivateBoundedCompactUintArray(const sserialize::BoundedCompactUintArray & index) :
m_data(index)
{}

ItemIndexPrivateBoundedCompactUintArray::
~ItemIndexPrivateBoundedCompactUintArray() {}

ItemIndex::Types
ItemIndexPrivateBoundedCompactUintArray::
type() const {
	return ItemIndex::T_BOUNDED_COMPACT_UINT_ARRAY;
}

uint32_t
ItemIndexPrivateBoundedCompactUintArray::at(uint32_t pos) const {
	return m_data.at(pos);
}

uint32_t
ItemIndexPrivateBoundedCompactUintArray::first() const {
	return m_data.at(0);
}

uint32_t
ItemIndexPrivateBoundedCompactUintArray::last() const {
	return m_data.at(size()-1);
}

uint32_t
ItemIndexPrivateBoundedCompactUintArray::size() const {
	return m_data.size();
}

uint8_t
ItemIndexPrivateBoundedCompactUintArray::bpn() const {
	return m_data.bpn();
}

sserialize::UByteArrayAdapter::SizeType
ItemIndexPrivateBoundedCompactUintArray::getSizeInBytes() const {
	return m_data.getSizeInBytes();
}

}//end namespace sserialize
