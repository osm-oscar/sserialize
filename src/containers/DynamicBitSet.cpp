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
m_remainingBits(0)
{}

DynamicBitSetIdIterator::DynamicBitSetIdIterator(const sserialize::DynamicBitSet * p, SizeType offset) :
m_p(p),
m_off(offset),
m_curId(m_off*8),
m_d(0),
m_remainingBits(0)
{
	if (m_off < m_p->data().size()) {
		m_d = m_p->data().at(m_off);
		m_remainingBits = 8;
		moveToNextSetBit();
	}
}

DynamicBitSetIdIterator::~DynamicBitSetIdIterator() {}

void DynamicBitSetIdIterator::moveToNextSetBit() {
	
	for(; m_remainingBits && m_d;) {
		if (m_d & 0x1) {
			return;
		}
		
		m_d >>= 1;
		m_remainingBits -= 1;
		m_curId += 1;
	}
	
	m_curId += m_remainingBits;
	m_off += 1;
	SSERIALIZE_CHEAP_ASSERT_EQUAL(m_curId, m_off*8);
	
	m_remainingBits = 8;
	for(; m_off < m_p->data().size();) {
		m_d = m_p->data().at(m_off);
		if (m_d) {
			break;
		}
		else {
			++m_off;
			m_curId += 8;
		}
	}
	
	if (m_off >= m_p->data().size()) {
		m_remainingBits = 0;
		SSERIALIZE_CHEAP_ASSERT_EQUAL(m_curId, m_off*8);
		return;
	}
	
	SSERIALIZE_CHEAP_ASSERT(m_d);
	
	for(; m_remainingBits && m_d;) {
		if (m_d & 0x1) {
			return;
		}
		
		m_d >>= 1;
		m_remainingBits -= 1;
		m_curId += 1;
	}
	//we should never get to here
	SSERIALIZE_CHEAP_ASSERT(false);
}

SizeType DynamicBitSetIdIterator::get() const {
	return m_curId;
}

void DynamicBitSetIdIterator::next() {
	if (m_off >= m_p->data().size()) {
		return;
	}
	//m_remainingBits points to the currently set bit or beyond the data
	SSERIALIZE_CHEAP_ASSERT(m_off >= m_p->data().size() || m_d & 0x1);
	m_remainingBits -= 1; //remove currently pointed to bit
	m_d >>= 1;
	m_curId += 1;
	SSERIALIZE_CHEAP_ASSERT(m_remainingBits < 8);
	moveToNextSetBit();
}

bool DynamicBitSetIdIterator::notEq(const AbstractArrayIterator<SizeType> * other) const {
	const DynamicBitSetIdIterator * o = static_cast<const DynamicBitSetIdIterator*>(other);
	return o->m_off != m_off || o->m_remainingBits != m_remainingBits || o->m_curId != m_curId || o->m_d != m_d;
}

bool DynamicBitSetIdIterator::eq(const AbstractArrayIterator<SizeType> * other) const {
	const DynamicBitSetIdIterator * o = static_cast<const DynamicBitSetIdIterator*>(other);
	return o->m_off == m_off && o->m_remainingBits == m_remainingBits && o->m_curId == m_curId && o->m_d == m_d;
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


bool DynamicBitSet::operator==(const DynamicBitSet & other) const {
	UByteArrayAdapter::SizeType i = 0;
	UByteArrayAdapter::SizeType s = std::min(m_data.size(), other.m_data.size());
	for(; i < s; ++i) {
		if (m_data[i] != other.m_data[i]) {
			return false;
		}
	}
	for(s = m_data.size(); i < s; ++i) {
		if (m_data[i]) {
			return false;
		}
	}
	for(s = other.m_data.size(); i < s; ++i) {
		if (other.m_data[i]) {
			return false;
		}
	}
	return true;
}

bool DynamicBitSet::operator!=(const DynamicBitSet & other) const {
	return !(*this == other);
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


DynamicBitSet & DynamicBitSet::operator&=(const DynamicBitSet & other) {
	m_data.resize( std::min(m_data.size(), other.m_data.size()) );
	for(UByteArrayAdapter::OffsetType i(0), s(m_data.size()); i < s; ++i) {
		m_data[i] &= other.m_data[i];
	}
	return *this;
}

DynamicBitSet & DynamicBitSet::operator|=(const DynamicBitSet & other) {
	for(UByteArrayAdapter::OffsetType i(0), s(std::min(m_data.size(), other.m_data.size())); i < s; ++i) {
		m_data[i] |= other.m_data[i];
	}
	if (m_data.size() < other.m_data.size()) {
		auto s = m_data.size();
		m_data.resize(other.m_data.size());
		m_data.putData(s, UByteArrayAdapter(other.m_data, s));
	}
	return *this;
}

DynamicBitSet & DynamicBitSet::operator-=(const DynamicBitSet & other) {
	for(UByteArrayAdapter::OffsetType i(0), s(std::min(m_data.size(), other.m_data.size())); i < s; ++i) {
		m_data[i] &= (~other.m_data[i]);
	}
	return *this;
}

DynamicBitSet & DynamicBitSet::operator^=(const DynamicBitSet & other) {
	for(UByteArrayAdapter::OffsetType i(0), s(std::min(m_data.size(), other.m_data.size())); i < s; ++i) {
		m_data[i] ^= other.m_data[i];
	}
	if (m_data.size() < other.m_data.size()) {
		auto s = m_data.size();
		m_data.resize(other.m_data.size());
		m_data.putData(s, UByteArrayAdapter(other.m_data, s));
	}
	return *this;
}

DynamicBitSet::const_iterator DynamicBitSet::cbegin() const {
	return const_iterator( new detail::DynamicBitSet::DynamicBitSetIdIterator(this, 0) );
}

DynamicBitSet::const_iterator DynamicBitSet::cend() const {
	return const_iterator( new detail::DynamicBitSet::DynamicBitSetIdIterator(this, m_data.size()) );
}


}//end namespace
