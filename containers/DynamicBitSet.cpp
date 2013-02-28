#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateDE.h>


namespace sserialize {

DynamicBitSet::DynamicBitSet() : m_data(UByteArrayAdapter::createCache(0, false)) {}
DynamicBitSet::DynamicBitSet(const UByteArrayAdapter & data) : m_data(data) {}
DynamicBitSet::~DynamicBitSet() {}

bool DynamicBitSet::isSet(uint32_t pos) const {
	return m_data.at( pos/8 ) & (static_cast<uint8_t>(1) << (pos % 8));
}

void DynamicBitSet::set(uint32_t pos) {
	uint32_t offset = pos / 8;
	uint32_t inByteOffset = pos % 8;
	if (offset >= m_data.size())
		m_data.growStorage(offset+1-m_data.size());
	m_data[offset] |= (static_cast<uint8_t>(1) << inByteOffset);
}

void DynamicBitSet::unset(uint32_t pos) {
	uint32_t offset = pos / 8;
	if (offset < m_data.size()) {
		uint32_t inByteOffset = pos % 8;
		m_data[offset] &= ~(static_cast<uint8_t>(1) << inByteOffset);
	}
}

ItemIndex DynamicBitSet::toIndex(int type) const {
	return ItemIndex::fromBitSet(*this, (ItemIndex::Types) type);
}


//TODO:SpeedUp!
uint32_t DynamicBitSet::size() const {
	uint32_t resSize = 0;
	UByteArrayAdapter::SizeType s = m_data.size();
	for(UByteArrayAdapter::SizeType it = 0; it < s; ++it) {
		resSize += popCount( m_data.at(it) );
	}
	return resSize;
}



}//end namespace