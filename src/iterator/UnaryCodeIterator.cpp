#include <sserialize/iterator/UnaryCodeIterator.h>

namespace sserialize {

UnaryCodeIterator::UnaryCodeIterator() :
m_d(),
m_pos(0),
m_last(0),
m_lastChunk(0),
m_chunkBitPtr(0)
{}

UnaryCodeIterator::UnaryCodeIterator(const UByteArrayAdapter& d) :
m_d(d),
m_pos(0),
m_last(0),
m_lastChunk(0),
m_chunkBitPtr(0)
{
	operator++();
}

UnaryCodeIterator::~UnaryCodeIterator() {}

UnaryCodeIterator::value_type
UnaryCodeIterator::operator*() const {
	return m_last;
}

UnaryCodeIterator&
UnaryCodeIterator::operator++() {
	if (m_pos >= m_d.size() && !m_chunkBitPtr) {
		return *this;
	}
	m_last = 0;
	
	if (m_chunkBitPtr != chunk_max_bit) { //there are still bits in our current byte
		while (m_chunkBitPtr) {
			bool isStopBit = (m_lastChunk & m_chunkBitPtr);
			m_chunkBitPtr >>= 1;
			
			if (isStopBit) {
				return *this;
			}
			else {
				m_last += 1;
			}
		}
	}
	
	SSERIALIZE_CHEAP_ASSERT(!m_chunkBitPtr);
	
	//from here on we only have full chunks, not loaded yet
	
	while (m_pos < m_d.size()) {
		loadNextChunk();
		if (m_lastChunk) { //our stop bit is inside
			break;
		}
		else {
			m_last += chunk_bits;
		}
	}
	
	//we still have to find our stop bit
	while (m_chunkBitPtr) {
		bool isStopBit = (m_lastChunk & m_chunkBitPtr);
		m_chunkBitPtr >>= 1;
		
		if (isStopBit) {
			break;
		}
		else {
			m_last += 1;
		}
	}
	
	return *this;
}

bool UnaryCodeIterator::operator!=(const UnaryCodeIterator& other) const {
	SSERIALIZE_CHEAP_ASSERT(m_d == other.m_d);
	return m_pos != other.m_pos || m_chunkBitPtr != other.m_chunkBitPtr || m_last != other.m_last;
}

bool UnaryCodeIterator::operator==(const UnaryCodeIterator& other) const {
	SSERIALIZE_CHEAP_ASSERT(m_d == other.m_d);
	return m_pos == other.m_pos && m_chunkBitPtr == other.m_chunkBitPtr && m_last == other.m_last;
}

void UnaryCodeIterator::loadNextChunk() {
	m_lastChunk = m_d.get<chunk_type>(m_pos);
	m_chunkBitPtr = chunk_max_bit;
	m_pos += SerializationInfo<chunk_type>::length;
}

// creator

UnaryCodeCreator::UnaryCodeCreator(UByteArrayAdapter& d) :
m_d(d),
m_lastChunk(0),
m_inChunkBits(chunk_bits)
{}

UnaryCodeCreator::~UnaryCodeCreator() {
	flush();
}

void UnaryCodeCreator::put(uint32_t v) {
	if (v >= uint32_t(m_inChunkBits) ) {
		m_d.put(m_lastChunk);
		v -= m_inChunkBits;
		
		m_inChunkBits = chunk_bits;
		m_lastChunk = 0;
		
		for(;v >= chunk_bits; v -= chunk_bits) {
			m_d.put(m_lastChunk);
		}
	}
	
	SSERIALIZE_CHEAP_ASSERT_SMALLER(v, uint32_t(m_inChunkBits));
	
	m_lastChunk |= chunk_type(1) << (m_inChunkBits-(v+1));
	m_inChunkBits -= (v+1);
}

void UnaryCodeCreator::flush() {
	if (m_lastChunk) {
		m_d.put(m_lastChunk);
	}
}

}//end namespace sserialize
