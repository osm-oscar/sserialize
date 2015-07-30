#include <sserialize/containers/MultiBitBackInserter.h>
#include <sserialize/algorithm/utilfuncs.h>

namespace sserialize {

MultiBitBackInserter::MultiBitBackInserter() : 
m_curInByteOffset(0)
{}

MultiBitBackInserter::MultiBitBackInserter(UByteArrayAdapter & data) :
m_data(data),
m_curInByteOffset(0)
{
	m_buffer.fill(0);
}

MultiBitBackInserter::~MultiBitBackInserter() {}

void MultiBitBackInserter::push_back(uint64_t value, uint8_t length) {
	value = value & createMask64(length);
	
	if (8-m_curInByteOffset >= length) {
		m_buffer[0] |= ( value << ((8-m_curInByteOffset)-length) );
		if (8-m_curInByteOffset == length) {
			m_data.putUint8(m_buffer[0]);
			m_curInByteOffset = 0;
			m_buffer[0] = 0;
		}
		else {
			m_curInByteOffset += length;
		}
	}
	else { //spans multiple bytes
		uint8_t pushLen = 0;
		uint8_t fullBitSpan = length + m_curInByteOffset;
		uint8_t byteSpan = fullBitSpan / 8;
		pushLen = byteSpan;
		m_curInByteOffset = fullBitSpan % 8;
		uint8_t overlap = 0;
		if (m_curInByteOffset) {
			overlap = (value << (8-m_curInByteOffset));
			value >>= m_curInByteOffset;
		}
		for(; byteSpan > 0; --byteSpan) {
			m_buffer[byteSpan-1] |= value;
			value >>= 8;
		}
		m_data.putData(m_buffer.begin(), pushLen);
		m_buffer.fill(0);
		m_buffer[0] = overlap;
	}
}

void MultiBitBackInserter::flush() {
	if (m_curInByteOffset) {
		m_data.putUint8(m_buffer[0]);
		m_curInByteOffset = 0;
		m_buffer[0] = 0;
	}
}

UByteArrayAdapter & MultiBitBackInserter::data() {
	return m_data;
}




}//end namespace