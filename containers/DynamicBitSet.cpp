#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateDE.h>


namespace sserialize {

DynamicBitSet::DynamicBitSet() : m_data(UByteArrayAdapter::createCache(0, false)) {}
DynamicBitSet::DynamicBitSet(const UByteArrayAdapter & data) : m_data(data) {}
DynamicBitSet::~DynamicBitSet() {}

bool DynamicBitSet::align(uint8_t shift) {
	UByteArrayAdapter::OffsetType newSize = m_data.size() >> shift;
	if ((newSize << shift) & m_data.size())
		++newSize;
	newSize <<= shift;
	return m_data.resize(newSize);
}


uint32_t DynamicBitSet::smallestEntry() const {
	uint32_t id = 0;
	for(uint32_t i = 0; i < m_data.size(); ++i) {
		uint8_t d = m_data.getUint8(i);
		if (d) {
			return id + msb(d);
		}
		else {
			id += 8;
		}
	}
	return id;
}

uint32_t DynamicBitSet::largestEntry() const {
	if (m_data.size() == 0)
		return 0;
	uint32_t id = m_data.size()*8-1;
	for(uint32_t i = m_data.size(); i > 0; --i) {
		uint8_t d = m_data.getUint8(i-1);
		if (d) {
			return id - (8-msb(d));
		}
		else {
			id -= 8;
		}
	}
	return id;
}

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
	UByteArrayAdapter::SizeType i;
	for(i = 0; i+4 <= s; i += 4) {//we can use this here as the order of the bits is not relevant
		resSize += popCount( m_data.getUint32(i));
	}
	for(; i < s; ++i) {
		resSize += popCount( m_data.at(i));
	}
	return resSize;
}

DynamicBitSet DynamicBitSet::operator&(const DynamicBitSet & other) const {
	UByteArrayAdapter::SizeType s = std::min<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size());
	UByteArrayAdapter d(UByteArrayAdapter::createCache(s, false) );
	for(UByteArrayAdapter::SizeType i = 0; i < s; ++i) {
		d[i] = m_data[i] & other.m_data[i];
	}
	return DynamicBitSet(d);
}

DynamicBitSet DynamicBitSet::operator|(const DynamicBitSet & other) const {
	UByteArrayAdapter::SizeType smax = std::max<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size());
	UByteArrayAdapter d(UByteArrayAdapter::createCache(smax, false) );
	
	UByteArrayAdapter::SizeType i = 0;
	UByteArrayAdapter::SizeType s;
	for(s = std::min<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size()); i < s; ++i) {
		d[i] = m_data[i] | other.m_data[i];
	}
	for(s = m_data.size(); i < s; ++i) {
		d[i] = m_data[i];
	}
	for(s = other.m_data.size(); i < s; ++i) {
		d[i] = other.m_data[i];
	}
	return DynamicBitSet(d);
}

DynamicBitSet DynamicBitSet::operator-(const DynamicBitSet & other) const {
	UByteArrayAdapter::SizeType s = std::min<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size());
	UByteArrayAdapter d(UByteArrayAdapter::createCache(s, false) );
	for(UByteArrayAdapter::SizeType i = 0; i < s; ++i) {
		d[i] = m_data[i] & (~other.m_data[i]);
	}
	return DynamicBitSet(d);
}
DynamicBitSet DynamicBitSet::operator^(const DynamicBitSet & other) const {
	UByteArrayAdapter::SizeType smax = std::max<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size());
	UByteArrayAdapter d(UByteArrayAdapter::createCache(smax, false) );
	
	UByteArrayAdapter::SizeType i = 0;
	UByteArrayAdapter::SizeType s;
	for(s = std::min<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size()); i < s; ++i) {
		d[i] = m_data[i] ^ other.m_data[i];
	}
	for(s = m_data.size(); i < s; ++i) {
		d[i] = m_data[i];
	}
	for(s = other.m_data.size(); i < s; ++i) {
		d[i] = other.m_data[i];
	}
	return DynamicBitSet(d);
}

DynamicBitSet DynamicBitSet::operator~() const {
	UByteArrayAdapter d(UByteArrayAdapter::createCache(m_data.size(), false) );
	UByteArrayAdapter::SizeType s = m_data.size();
	for(UByteArrayAdapter::SizeType i = 0; i < s; ++i) {
		d[i] = ~m_data[i];
	}
	return DynamicBitSet(d);
}

}//end namespace