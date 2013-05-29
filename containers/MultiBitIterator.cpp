#include <sserialize/containers/MultiBitIterator.h>
#include <sserialize/utility/utilfuncs.h>

namespace sserialize {


MultiBitIterator::MultiBitIterator() :
m_bitOffset(0)
{}

MultiBitIterator::MultiBitIterator(const UByteArrayAdapter & data) :
m_data(data),
m_bitOffset(0)
{
	m_data.resetGetPtr();
}

MultiBitIterator::~MultiBitIterator() {}

uint32_t MultiBitIterator::get32() const {
	UByteArrayAdapter::OffsetType getPtr = m_data.tellGetPtr();
	uint32_t res = static_cast<uint32_t>(m_data.getUint8(getPtr) << 8);
	++getPtr;
	uint8_t headerLength = 8-m_bitOffset;
	int bitsRead = headerLength;
	for(; bitsRead < 32 && getPtr < m_data.size(); bitsRead += 8) { 
		uint8_t destLshift = (32-bitsRead > 8 ? 8 : 32-bitsRead);
		uint8_t srcRShift = (destLshift < 8 ? 8-destLshift : 0);
		uint8_t byte = m_data.getUint8(getPtr);
		++getPtr;
		byte >>= srcRShift;
		res |= byte;
		res <<= destLshift;
	}
	if (bitsRead < 32) {
		res <<= (32-bitsRead);
	}
	
	return res;
}

uint64_t MultiBitIterator::get64() const {
	UByteArrayAdapter::OffsetType getPtr = m_data.tellGetPtr();
	uint64_t res = m_data.getUint8(getPtr);
	++getPtr;
	uint8_t headerLength = 8-m_bitOffset;
	int bitsRead = headerLength;
	uint8_t destLshift = 8;
	uint8_t srcRShift = 0;
	for(; bitsRead < 64 && getPtr < m_data.size(); bitsRead += 8) {
		destLshift = (64-bitsRead > 8 ? 8 : 64-bitsRead);
		res <<= destLshift;
		srcRShift = 8-destLshift;
		uint8_t byte = m_data.getUint8(getPtr);
		++getPtr;
		byte >>= srcRShift;
		res |= byte;
	}
	if (bitsRead < 64) {
		res <<= (64-bitsRead);
	}
	return res;
}

uint32_t MultiBitIterator::get32(uint8_t len) const {
	return get32() >> (32-len);
}

uint64_t MultiBitIterator::get64(uint8_t len) const {
	return get64() >> (64-len);
}

MultiBitIterator & MultiBitIterator::operator+=(uint32_t bitCount) {
	bitCount += m_bitOffset;
	m_data.incGetPtr(bitCount/8);
	m_bitOffset = bitCount % 8;
	return *this;
}

void MultiBitIterator::reset() {
	m_data.resetGetPtr();
	m_bitOffset = 0;
}

bool MultiBitIterator::hasNext() const {
	return m_bitOffset || m_data.getPtrHasNext();
}



}//end namespace