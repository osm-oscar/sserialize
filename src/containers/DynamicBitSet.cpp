#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateDE.h>


namespace sserialize {
namespace detail {
namespace DynamicBitSet {

DynamicBitSetIdIterator::DynamicBitSetIdIterator() :
m_p(0),
m_off(0),
m_curId(0),
m_d(0),
m_curShift(0)
{}

DynamicBitSetIdIterator::DynamicBitSetIdIterator(const sserialize::DynamicBitSet * p, SizeType offset) :
m_p(p),
m_off(offset),
m_curId(offset*8),
m_d(0),
m_curShift(8)
{
	next();
}

DynamicBitSetIdIterator::~DynamicBitSetIdIterator() {}

SizeType DynamicBitSetIdIterator::get() const {
	return m_curId;
}

void DynamicBitSetIdIterator::next() {
	if (m_curShift >= 8) {
		const UByteArrayAdapter & d = m_p->data();
		UByteArrayAdapter::OffsetType ds = d.size();
		//skip to first byte != 0
		for(; ds > m_off && (m_d = d.at(m_off)) == 0; ++m_off);
		m_curShift = (ds > m_off ? 0 : 8);
		next();
	}
	else {
		if (m_d) {
			for(; m_curShift < 8 && (m_d & 0x1) == 0; ++m_curShift, ++m_curId, m_d >>= 1);
		}
		else {
			m_curId += 8-m_curShift;
			next();
		}
	}
}

bool DynamicBitSetIdIterator::notEq(const AbstractArrayIterator<SizeType> * other) const {
	const DynamicBitSetIdIterator * o = static_cast<const DynamicBitSetIdIterator*>(other);
	return o->m_off != m_off || o->m_curShift != m_curShift || o->m_curId != m_curId || o->m_d != m_d;
}

bool DynamicBitSetIdIterator::eq(const AbstractArrayIterator<SizeType> * other) const {
	const DynamicBitSetIdIterator * o = static_cast<const DynamicBitSetIdIterator*>(other);
	return o->m_off == m_off && o->m_curShift == m_curShift && o->m_curId == m_curId && o->m_d == m_d;
}

AbstractArrayIterator<SizeType> * DynamicBitSetIdIterator::copy() const {
	return new DynamicBitSetIdIterator(*this);
}

}}


DynamicBitSet::DynamicBitSet() : m_data(UByteArrayAdapter::createCache(0, sserialize::MM_PROGRAM_MEMORY)) {}
DynamicBitSet::DynamicBitSet(const UByteArrayAdapter & data) : m_data(data) {}
DynamicBitSet::~DynamicBitSet() {}

void DynamicBitSet::resize(UByteArrayAdapter::OffsetType size) {
	m_data.resize(size/8);
}

bool DynamicBitSet::align(uint8_t shift) {
	UByteArrayAdapter::OffsetType newSize = m_data.size() >> shift;
	if ((newSize << shift) & m_data.size())
		++newSize;
	newSize <<= shift;
	return m_data.resize(newSize);
}


IdType DynamicBitSet::smallestEntry() const {
	IdType id = 0;
	for(UByteArrayAdapter::OffsetType i = 0, s = m_data.size(); i < s; ++i) {
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

IdType DynamicBitSet::largestEntry() const {
	if (m_data.size() == 0)
		return 0;
	IdType id = m_data.size()*8-1;
	for(UByteArrayAdapter::OffsetType i = m_data.size(); i > 0; --i) { //this i substracted by 1 before geting the byte
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

bool DynamicBitSet::isSet(sserialize::SizeType pos) const {
	return m_data.at( pos/8 ) & (static_cast<uint8_t>(1) << (pos % 8));
}

void DynamicBitSet::set(sserialize::SizeType pos) {
	UByteArrayAdapter::OffsetType offset = static_cast<UByteArrayAdapter::OffsetType>(pos) / 8;
	uint32_t inByteOffset = pos % 8;
	if (offset >= m_data.size())
		m_data.growStorage(offset+1-m_data.size());
	m_data[offset] |= (static_cast<uint8_t>(1) << inByteOffset);
}

void DynamicBitSet::unset(sserialize::SizeType pos) {
	UByteArrayAdapter::OffsetType offset = static_cast<UByteArrayAdapter::OffsetType>(pos) / 8;
	if (offset < m_data.size()) {
		uint32_t inByteOffset = pos % 8;
		m_data[offset] &= ~(static_cast<uint8_t>(1) << inByteOffset);
	}
}

ItemIndex DynamicBitSet::toIndex(int type) const {
	return ItemIndex::fromBitSet(*this, (ItemIndex::Types) type);
}

SizeType DynamicBitSet::size() const {
	uint32_t resSize = 0;
	UByteArrayAdapter::OffsetType s = m_data.size();
	UByteArrayAdapter::OffsetType i;
	for(i = 0; i+4 <= s; i += 4) {//we can use this here as the order of the bits is not relevant
		resSize += popCount<uint32_t>( m_data.getUint32(i) );
	}
	for(; i < s; ++i) {
		resSize += popCount<uint8_t>( m_data.at(i));
	}
	return resSize;
}

DynamicBitSet DynamicBitSet::operator&(const DynamicBitSet & other) const {
	UByteArrayAdapter::OffsetType s = std::min<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size());
	UByteArrayAdapter d(UByteArrayAdapter::createCache(s, sserialize::MM_PROGRAM_MEMORY) );
	for(UByteArrayAdapter::OffsetType i = 0; i < s; ++i) {
		d[i] = m_data[i] & other.m_data[i];
	}
	return DynamicBitSet(d);
}

DynamicBitSet DynamicBitSet::operator|(const DynamicBitSet & other) const {
	UByteArrayAdapter::OffsetType smax = std::max<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size());
	UByteArrayAdapter d(UByteArrayAdapter::createCache(smax, sserialize::MM_PROGRAM_MEMORY) );
	
	UByteArrayAdapter::OffsetType i = 0;
	UByteArrayAdapter::OffsetType s;
	for(s = std::min<UByteArrayAdapter::OffsetType>( m_data.size(), other.m_data.size()); i < s; ++i) {
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
	UByteArrayAdapter::OffsetType s = std::min<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size());
	UByteArrayAdapter d(UByteArrayAdapter::createCache(s, sserialize::MM_PROGRAM_MEMORY) );
	for(UByteArrayAdapter::OffsetType i = 0; i < s; ++i) {
		d[i] = m_data[i] & (~other.m_data[i]);
	}
	return DynamicBitSet(d);
}
DynamicBitSet DynamicBitSet::operator^(const DynamicBitSet & other) const {
	UByteArrayAdapter::OffsetType smax = std::max<UByteArrayAdapter::SizeType>( m_data.size(), other.m_data.size());
	UByteArrayAdapter d(UByteArrayAdapter::createCache(smax, sserialize::MM_PROGRAM_MEMORY) );
	
	UByteArrayAdapter::OffsetType i = 0;
	UByteArrayAdapter::OffsetType s;
	for(s = std::min<UByteArrayAdapter::OffsetType>( m_data.size(), other.m_data.size()); i < s; ++i) {
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
	UByteArrayAdapter d(UByteArrayAdapter::createCache(m_data.size(), sserialize::MM_PROGRAM_MEMORY) );
	UByteArrayAdapter::OffsetType s = m_data.size();
	for(UByteArrayAdapter::OffsetType i = 0; i < s; ++i) {
		d[i] = ~m_data[i];
	}
	return DynamicBitSet(d);
}

}//end namespace