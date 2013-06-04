#ifndef SSERIALIZE_MULTI_BIT_ITERATOR_H
#define SSERIALIZE_MULTI_BIT_ITERATOR_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <limits>

namespace sserialize {

class MultiBitIterator {
	UByteArrayAdapter m_data;
	uint8_t m_bitOffset;
private:
	template<typename T_UINT_TYPE>
	typename std::enable_if< std::is_unsigned<T_UINT_TYPE>::value, T_UINT_TYPE>::type getN() const {
		UByteArrayAdapter::OffsetType getPtr = m_data.tellGetPtr();
		T_UINT_TYPE res = static_cast<T_UINT_TYPE>(m_data.getUint8(getPtr)) << 8;
		++getPtr;
		uint8_t headerLength = 8-m_bitOffset;
		int bitsRead = headerLength;
		for(; bitsRead < std::numeric_limits<T_UINT_TYPE>::digits && getPtr < m_data.size(); bitsRead += 8) { 
			uint8_t destLshift = (std::numeric_limits<T_UINT_TYPE>::digits-bitsRead > 8 ? 8 : std::numeric_limits<T_UINT_TYPE>::digits-bitsRead);
			uint8_t srcRShift = (destLshift < 8 ? 8-destLshift : 0);
			uint8_t byte = m_data.getUint8(getPtr);
			++getPtr;
			byte >>= srcRShift;
			res |= byte;
			res <<= destLshift;
		}
		if (bitsRead < std::numeric_limits<T_UINT_TYPE>::digits) {
			res <<= (std::numeric_limits<T_UINT_TYPE>::digits-bitsRead);
		}
		return res;
	}
	
	template<typename T_UINT_TYPE>
	inline T_UINT_TYPE getN(uint8_t len) const {
		return getN<T_UINT_TYPE>() >> (std::numeric_limits<T_UINT_TYPE>::digits-len);
	}

	
public:
	MultiBitIterator();
	MultiBitIterator(const UByteArrayAdapter & data);
	virtual ~MultiBitIterator();
	inline uint16_t get16() const { return getN<uint16_t>(); }
	inline uint32_t get32() const { return getN<uint32_t>(); }
	inline uint64_t get64() const { return getN<uint64_t>(); }
	///get the first @len bits with up to 16 bits
	inline uint16_t get16(uint8_t len) const { return getN<uint16_t>(len);}
	///get the first @len bits with up to 32 bits
	inline uint32_t get32(uint8_t len) const { return getN<uint32_t>(len);}
	///get the first @len bits with up to 64 bits
	inline uint64_t get64(uint8_t len) const { return getN<uint64_t>(len);}
	///@param bitCount this should not be larger than INT_MAX
	MultiBitIterator & operator+=(uint32_t bitCount);
	void reset();
	bool hasNext() const;
	UByteArrayAdapter::OffsetType dataSize() const { return m_data.size(); }
};

}//end namespace


#endif