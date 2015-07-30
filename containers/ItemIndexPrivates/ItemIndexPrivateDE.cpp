#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateDE.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {

ItemIndexPrivateDE::ItemIndexPrivateDE(const UByteArrayAdapter & data) :
m_data(UByteArrayAdapter(data, 8, data.getUint32(0))),
m_size(data.getUint32(4)),
m_dataOffset(0),
m_curId(0),
m_cache(UByteArrayAdapter::createCache(m_size*4, sserialize::MM_PROGRAM_MEMORY) ),
m_cacheOffset(0)
{
	if (m_size > m_data.size())
		throw sserialize::CorruptDataException("ItemIndexPrivateDE");
	
}

ItemIndexPrivateDE::ItemIndexPrivateDE() : m_size(0),  m_dataOffset(0), m_curId(0), m_cacheOffset(0) {}

ItemIndexPrivateDE::~ItemIndexPrivateDE() {}

ItemIndex::Types ItemIndexPrivateDE::type() const {
	return ItemIndex::T_DE;
}


int ItemIndexPrivateDE::find(uint32_t id) const {
	return sserialize::ItemIndexPrivate::find(id);
}

uint32_t ItemIndexPrivateDE::at(uint32_t pos) const {
	if (!size() || size() <= pos)
		return 0;
	int len = 0;
	for(;m_cacheOffset <= pos;) {
		m_curId += m_data.getVlPackedUint32(m_dataOffset, &len);
		m_cache.putUint32(m_cacheOffset*4, m_curId);
		m_dataOffset += len;
		++m_cacheOffset;
	}
	return m_cache.getUint32(pos*4);
}

uint32_t ItemIndexPrivateDE::first() const {
	if (size())
		return at(0);
	return 0;
}

uint32_t ItemIndexPrivateDE::last() const {
	if (size())
		return at(size()-1);
	return 0;
}

uint32_t ItemIndexPrivateDE::size() const {
	return m_size;
}

uint8_t ItemIndexPrivateDE::bpn() const { return m_data.size()*8/m_size; } //This shouldn't cause an overflow here

uint32_t ItemIndexPrivateDE::getSizeInBytes() const { return m_data.size() + 8; }

void ItemIndexPrivateDE::putInto(DynamicBitSet & bitSet) const {
	UByteArrayAdapter tmpData(m_data);
	uint32_t mySize = size();
	uint32_t count = 0;
	uint32_t prev = 0;
	while(count < mySize) {
		prev += tmpData.getVlPackedUint32();
		bitSet.set(prev);
		++count;
	}
}

void ItemIndexPrivateDE::putInto(uint32_t * dest) const {
	UByteArrayAdapter tmpData(m_data);
	uint32_t prev = 0;
	for(uint32_t * destEnd(dest + size()); dest != destEnd; ++dest) {
		prev += tmpData.getVlPackedUint32();
		*dest = prev;
	}
}

ItemIndexPrivate * ItemIndexPrivateDE::fromBitSet(const DynamicBitSet & bitSet) {
	const UByteArrayAdapter & bitSetData(bitSet.data());
	UByteArrayAdapter cacheData( UByteArrayAdapter::createCache(bitSetData.size(), sserialize::MM_PROGRAM_MEMORY));
	ItemIndexPrivateDECreator creator(cacheData);
	uint32_t curId = 0;
	uint32_t myDataSize = bitSetData.size();
	for(uint32_t dataOffset = 0; dataOffset < myDataSize; ++dataOffset, curId += 8) {
		uint8_t data = bitSetData.at(dataOffset);
		for(uint32_t i = 0; i < 8 && data; ++i) {
			if (data & 0x1)
				creator.push_back(curId+i);
			data >>= 1;
		}
	}
	creator.flush();
	return creator.getPrivateIndex();
}

ItemIndexPrivate * ItemIndexPrivateDE::intersect(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateDE * cother = dynamic_cast<const ItemIndexPrivateDE*>(other);
	if (!cother)
		return ItemIndexPrivate::doIntersect(other);
	return genericOp<sserialize::detail::ItemIndexImpl::IntersectOp>(cother);
}

ItemIndexPrivate * ItemIndexPrivateDE::unite(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateDE * cother = dynamic_cast<const ItemIndexPrivateDE*>(other);
	if (!cother)
		return ItemIndexPrivate::doUnite(other);
	return genericOp<sserialize::detail::ItemIndexImpl::UniteOp>(cother);
}

ItemIndexPrivate* ItemIndexPrivateDE::difference(const ItemIndexPrivate* other) const {
	const ItemIndexPrivateDE * cother = dynamic_cast<const ItemIndexPrivateDE*>(other);
	if (!cother)
		return ItemIndexPrivate::doDifference(other);
	return genericOp<sserialize::detail::ItemIndexImpl::DifferenceOp>(cother);
}

ItemIndexPrivate* ItemIndexPrivateDE::symmetricDifference(const ItemIndexPrivate* other) const {
	const ItemIndexPrivateDE * cother = dynamic_cast<const ItemIndexPrivateDE*>(other);
	if (!cother)
		return ItemIndexPrivate::doSymmetricDifference(other);
	return genericOp<sserialize::detail::ItemIndexImpl::SymmetricDifferenceOp>(cother);
}

}//end namespace