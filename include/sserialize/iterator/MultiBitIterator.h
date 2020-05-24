#ifndef SSERIALIZE_MULTI_BIT_ITERATOR_H
#define SSERIALIZE_MULTI_BIT_ITERATOR_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <limits>
#include <type_traits>

namespace sserialize {
namespace detail {


template<uint32_t bytes>
struct UnsignedFromBytes;

#define UNSIGNED_FROM_BYTES_SPEC(__BYTES, __TYPE) template<> struct UnsignedFromBytes<__BYTES> { using type = __TYPE; };

UNSIGNED_FROM_BYTES_SPEC(1, uint8_t)
UNSIGNED_FROM_BYTES_SPEC(2, uint16_t)
UNSIGNED_FROM_BYTES_SPEC(3, uint32_t)
UNSIGNED_FROM_BYTES_SPEC(4, uint32_t)
UNSIGNED_FROM_BYTES_SPEC(5, uint64_t)
UNSIGNED_FROM_BYTES_SPEC(6, uint64_t)
UNSIGNED_FROM_BYTES_SPEC(7, uint64_t)
UNSIGNED_FROM_BYTES_SPEC(8, uint64_t)

#undef UNSIGNED_FROM_BYTES_SPEC

template<uint32_t bits>
struct UnsignedFromBits {
	using type = typename UnsignedFromBytes<bits/8 + uint32_t(bits%8 != 0)>::type;
};

}

class MultiBitIterator {
	UByteArrayAdapter m_data;
	uint8_t m_bitOffset;
private:
	///get get<uint8_t> will NOT! work
	template<typename T_UINT_TYPE>
	typename std::enable_if< std::is_unsigned<T_UINT_TYPE>::value, T_UINT_TYPE>::type getN() const {
		UByteArrayAdapter::OffsetType getPtr = m_data.tellGetPtr();
		T_UINT_TYPE res = (T_UINT_TYPE) (static_cast<T_UINT_TYPE>(m_data.getUint8(getPtr)) << 8);
		++getPtr;
		uint8_t headerLength = 8-m_bitOffset;
		int remainingBits = std::numeric_limits<T_UINT_TYPE>::digits - headerLength;
		do {
			uint8_t srcRShift = (uint8_t)(remainingBits > 8 ? 0 : 8-remainingBits);
			uint8_t byte = 0;
			try { //UBA does not return 0 anymore on out of bounds access
				byte = m_data.getUint8(getPtr);
			}
			catch (OutOfBoundsException const &) {}
			++getPtr;
			byte >>= srcRShift;
			res |= byte;
			remainingBits -= 8;
			if (remainingBits > 0) {
				res <<= (remainingBits > 8 ? 8 : remainingBits);
			}
			else {
				break;
			}
		} while(remainingBits > 0);
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
	///get the first @len bits
	template<typename T>
	inline typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value && !std::is_same<T, uint8_t>::value, T>::type get(uint8_t len) {
		return getN<T>(len);
	}
	template<typename T>
	inline typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value && std::is_same<T, uint8_t>::value, T>::type get(uint8_t len) {
		return uint8_t( getN<uint16_t>(len) );
	}
	template<uint32_t bits>
	inline auto get() {
		return get<typename detail::UnsignedFromBits<bits>::type>(bits);
	}
	///@param bitCount this should not be larger than INT_MAX
	MultiBitIterator & operator+=(uint32_t bitCount);
	void reset();
	bool hasNext() const;
	UByteArrayAdapter::OffsetType dataSize() const { return m_data.size(); }
};

}//end namespace


#endif
