#ifndef SSERIALIZE_MULTI_BIT_BACK_INSERTER_H
#define SSERIALIZE_MULTI_BIT_BACK_INSERTER_H
#include <array>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/algorithm/utilfuncs.h>

namespace sserialize {

class MultiBitBackInserter final {
	UByteArrayAdapter * m_data;
	bool m_delete;
	uint8_t m_curInByteOffset;
	std::array<uint8_t, 9> m_buffer;
public:
	MultiBitBackInserter(const MultiBitBackInserter &) = delete;
	MultiBitBackInserter & operator=(const MultiBitBackInserter &) = delete;
public:
	MultiBitBackInserter();
	MultiBitBackInserter(UByteArrayAdapter & data);
	~MultiBitBackInserter();
	void push_back(uint64_t value, uint8_t length);
	
	template<typename T_ITERATOR>
	void push_back(T_ITERATOR begin, T_ITERATOR end, uint8_t length);
	
	template<typename T_ITERATOR>
	void push_back32(T_ITERATOR begin, T_ITERATOR end, uint8_t length);
	
	void flush();
	UByteArrayAdapter & data();
};

}//end namespace

namespace sserialize {


template<typename T_ITERATOR>
void MultiBitBackInserter::push_back(T_ITERATOR begin, T_ITERATOR end, uint8_t length) {
	uint64_t mask = createMask64(length);
	for(; begin != end; ++begin) {
		uint64_t value = *begin & mask;
		if (8-m_curInByteOffset >= length) {
			m_buffer[0] |= ( value << ((8-m_curInByteOffset)-length) );
			if (8-m_curInByteOffset == length) {
				data().putUint8(m_buffer[0]);
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
			data().putData(m_buffer.begin(), pushLen);
			m_buffer.fill(0);
			m_buffer[0] = overlap;
		}
	}
}

}

#endif
