#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateNative.h>
#include <string.h>
#include <sserialize/storage/SerializationInfo.h>
#include <sserialize/containers/ItemIndex.h>

namespace sserialize {
namespace detail {
namespace ItemIndexPrivate {

ItemIndexPrivateNative::ItemIndexPrivateNative(const UByteArrayAdapter& data) :
m_size(data.getUint32(0)),
m_dataMem( UByteArrayAdapter(data, sserialize::SerializationInfo<uint32_t>::length, m_size*sizeof(uint32_t)/sizeof(uint8_t)).asMemView() )
{}

ItemIndexPrivateNative::ItemIndexPrivateNative() : m_size(0) {}

ItemIndexPrivateNative::~ItemIndexPrivateNative() {}

uint32_t ItemIndexPrivateNative::size() const {
	return m_size;
}

uint32_t ItemIndexPrivateNative::first() const {
	return at(0);
}

uint32_t ItemIndexPrivateNative::last() const {
	return at(size()-1);
}

uint32_t ItemIndexPrivateNative::uncheckedAt(uint32_t pos) const {
	return uncheckedAt(pos, m_dataMem.get());
}

uint32_t ItemIndexPrivateNative::at(uint32_t pos) const {
	if (pos < size()) {
		return uncheckedAt(pos);
	}
	return 0;
}

uint8_t ItemIndexPrivateNative::bpn() const {
	return getSizeInBytes()*8/(size()+1);
}

uint32_t ItemIndexPrivateNative::getSizeInBytes() const {
	return m_dataMem.size()+SerializationInfo<uint32_t>::length;
}

UByteArrayAdapter ItemIndexPrivateNative::data() const {
	UByteArrayAdapter ret(m_dataMem.dataBase());
	ret.resetPtrs();
	ret -= sserialize::SerializationInfo<uint32_t>::length;
	return ret;
}

sserialize::ItemIndex::Types ItemIndexPrivateNative::type() const {
	return sserialize::ItemIndex::T_NATIVE;
}

sserialize::ItemIndexPrivate* ItemIndexPrivateNative::intersect(const ItemIndexPrivate* other) const {
	const ItemIndexPrivateNative * cother = dynamic_cast<const ItemIndexPrivateNative*>(other);
	if (!cother) {
		return ItemIndexPrivate::doIntersect(other);
	}
	return ItemIndexPrivateNative::genericSetOp<sserialize::detail::ItemIndexImpl::IntersectOp>(cother);
}

sserialize::ItemIndexPrivate* ItemIndexPrivateNative::unite(const ItemIndexPrivate* other) const {
	const ItemIndexPrivateNative * cother = dynamic_cast<const ItemIndexPrivateNative*>(other);
	if (!cother) {
		return ItemIndexPrivate::doUnite(other);
	}
	return genericSetOp<sserialize::detail::ItemIndexImpl::UniteOp>(cother);
}

sserialize::ItemIndexPrivate* ItemIndexPrivateNative::difference(const ItemIndexPrivate* other) const {
	const ItemIndexPrivateNative * cother = dynamic_cast<const ItemIndexPrivateNative*>(other);
	if (!cother) {
		return ItemIndexPrivate::doDifference(other);
	}
	return genericSetOp<sserialize::detail::ItemIndexImpl::DifferenceOp>(cother);
}

sserialize::ItemIndexPrivate* ItemIndexPrivateNative::symmetricDifference(const ItemIndexPrivate* other) const {
	const ItemIndexPrivateNative * cother = dynamic_cast<const ItemIndexPrivateNative*>(other);
	if (!cother) {
		return ItemIndexPrivate::doSymmetricDifference(other);
	}
	return genericSetOp<sserialize::detail::ItemIndexImpl::SymmetricDifferenceOp>(cother);
}

}}}//end namespace