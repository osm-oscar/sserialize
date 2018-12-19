#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateNative.h>
#include <string.h>
#include <sserialize/storage/SerializationInfo.h>
#include <sserialize/containers/ItemIndex.h>

namespace sserialize {
namespace detail {
namespace ItemIndexPrivate {

ItemIndexPrivateNative::MyIterator::MyIterator(uint8_t const * data) :
m_data(data)
{}

ItemIndexPrivateNative::MyIterator::~MyIterator() {}

uint32_t
ItemIndexPrivateNative::MyIterator::get() const {
	uint32_t res;
	::memmove(&res, m_data, sizeof(uint32_t));
	return res;
}

void
ItemIndexPrivateNative::MyIterator::next() {
	m_data += sizeof(uint32_t);
}

bool
ItemIndexPrivateNative::MyIterator::notEq(const ItemIndexPrivate::const_iterator_base_type * other) const {
	SSERIALIZE_CHEAP_ASSERT(dynamic_cast<MyIterator const*>(other));
	return m_data !=  static_cast<MyIterator const*>(other)->m_data;
}

bool
ItemIndexPrivateNative::MyIterator::eq(const ItemIndexPrivate::const_iterator_base_type * other) const {
	SSERIALIZE_CHEAP_ASSERT(dynamic_cast<MyIterator const*>(other));
	return m_data ==  static_cast<MyIterator const*>(other)->m_data;
}

sserialize::ItemIndexPrivate::const_iterator_base_type *
ItemIndexPrivateNative::MyIterator::copy() const {
	return new ItemIndexPrivateNative::MyIterator(m_data);
}

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

void ItemIndexPrivateNative::putInto(sserialize::DynamicBitSet & bitSet) const {
	if (!size()) {
		return;
	}
	UByteArrayAdapter & dest = bitSet.data();
	const uint8_t * data = m_dataMem.get();
	{
		uint32_t fdestMaxBytePos = last()/8 + (last()%8 > 0);
		if (dest.size() <= fdestMaxBytePos) {
			dest.resize(fdestMaxBytePos+1);
		}
	}
	if (dest.isContiguous()) {
		UByteArrayAdapter::MemoryView destMv(dest.asMemView());
		uint8_t * destMvd = destMv.data();
		for(uint32_t i(0), s(size()); i < s; ++i, data += sizeof(uint32_t)) {
			uint32_t value;
			::memmove(&value, data, sizeof(uint32_t));
			uint32_t bytePos = value/8;
			uint32_t inBytePos = value%8;
			destMvd[bytePos] |= uint8_t(1) << inBytePos;
		}
	}
	else {
		for(uint32_t i(0), s(size()); i < s; ++i, data += sizeof(uint32_t)) {
			uint32_t value;
			::memmove(&value, data, sizeof(uint32_t));
			uint32_t bytePos = value/8;
			uint32_t inBytePos = value%8;
			dest[bytePos] |= uint8_t(1) << inBytePos;
		}
	}
}

void ItemIndexPrivateNative::putInto(uint32_t* dest) const {
	::memmove(dest, m_dataMem.get(), sizeof(uint32_t)*m_size);
}

uint8_t ItemIndexPrivateNative::bpn() const {
	return getSizeInBytes()*8/(size()+1);
}

UByteArrayAdapter::SizeType ItemIndexPrivateNative::getSizeInBytes() const {
	return m_dataMem.size()+SerializationInfo<uint32_t>::length;
}

UByteArrayAdapter ItemIndexPrivateNative::data() const {
	UByteArrayAdapter ret(m_dataMem.dataBase());
	ret -= sserialize::SerializationInfo<uint32_t>::length;
	ret.resetPtrs();
	return ret;
}

sserialize::ItemIndex::Types ItemIndexPrivateNative::type() const {
	return sserialize::ItemIndex::T_NATIVE;
}


ItemIndexPrivateNative::const_iterator ItemIndexPrivateNative::cbegin() const {
	return new MyIterator(m_dataMem.get());
}

ItemIndexPrivateNative::const_iterator ItemIndexPrivateNative::cend() const {
	return new MyIterator(m_dataMem.get()+size()*sizeof(uint32_t));
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

ItemIndexNativeCreator::ItemIndexNativeCreator(uint32_t maxSize) :
m_mem((sserialize::UByteArrayAdapter::SizeType(maxSize)+1)*sizeof(uint32_t), MM_PROGRAM_MEMORY),
m_it(m_mem.begin()+sizeof(uint32_t))
{
	SSERIALIZE_CHEAP_ASSERT_SMALLER(maxSize, std::numeric_limits<uint32_t>::max());
	SSERIALIZE_CHEAP_ASSERT_SMALLER(uint64_t(m_mem.begin()), uint64_t(m_it));
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(uint64_t(m_it), uint64_t(m_mem.end()));
}

ItemIndexNativeCreator::~ItemIndexNativeCreator()
{}

uint32_t ItemIndexNativeCreator::size() const {
	return uint32_t( (m_it - (m_mem.begin()+4))/sizeof(uint32_t) );
}

void ItemIndexNativeCreator::push_back(uint32_t id) {
	SSERIALIZE_CHEAP_ASSERT_SMALLER(uint64_t(m_it), uint64_t(m_mem.end()));
	if (UNLIKELY_BRANCH(m_it >= m_mem.end())) {
		throw sserialize::OutOfBoundsException("sserialize::ItemIndexNativeCreator::push_back");
	}
	::memmove(m_it, &id, sizeof(uint32_t));
	m_it += sizeof(uint32_t);
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(uint64_t(m_it), uint64_t(m_mem.end()));
}

void ItemIndexNativeCreator::flush() {
	SSERIALIZE_CHEAP_ASSERT_SMALLER(uint64_t(m_mem.begin()), uint64_t(m_it));
	auto finalSize = m_it - m_mem.begin();
	m_mem.resize(finalSize); //resize invalidates iterators
	m_it = m_mem.begin()+finalSize;
	data().putUint32(0, size());
}

ItemIndex ItemIndexNativeCreator::getIndex() {
	flush();
	return ItemIndex(data(), sserialize::ItemIndex::T_NATIVE);
}

sserialize::ItemIndexPrivate * ItemIndexNativeCreator::getPrivateIndex() {
	flush();
	return new ItemIndexPrivateNative(data());
}

UByteArrayAdapter ItemIndexNativeCreator::data() {
	return UByteArrayAdapter(m_mem);
}

}}}//end namespace
