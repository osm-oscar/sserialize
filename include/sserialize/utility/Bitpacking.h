#ifndef SSERIALIZE_BIT_PACKING_H
#define SSERIALIZE_BIT_PACKING_H
#include <sserialize/utility/constants.h>
#include <sserialize/utility/assert.h>
#include <sserialize/algorithm/utilmath.h>

#include <memory>
#include <limits>
#include <numeric>
#include <utility>
#include <string.h>
#include <x86intrin.h>

namespace sserialize {
namespace detail {
namespace bitpacking {

inline uint64_t betoh(uint64_t v) { return be64toh(v); }
inline uint32_t betoh(uint32_t v) { return be32toh(v); }
inline uint16_t betoh(uint16_t v) { return be16toh(v); }
inline uint8_t betoh(uint8_t v) { return v; }

template<uint32_t bpn>
struct UnpackerBufferTypeSelector {
	using type = uint64_t;
};

template<> struct UnpackerBufferTypeSelector<1> { using type = uint8_t; };
template<> struct UnpackerBufferTypeSelector<2> { using type = uint8_t; };
template<> struct UnpackerBufferTypeSelector<4> { using type = uint8_t; };
template<> struct UnpackerBufferTypeSelector<8> { using type = uint8_t; };
template<> struct UnpackerBufferTypeSelector<16> { using type = uint16_t; };
template<> struct UnpackerBufferTypeSelector<32> { using type = uint32_t; };

template<> struct UnpackerBufferTypeSelector<3> { using type = uint16_t; };
template<> struct UnpackerBufferTypeSelector<5> { using type = uint16_t; };
template<> struct UnpackerBufferTypeSelector<6> { using type = uint16_t; };
template<> struct UnpackerBufferTypeSelector<7> { using type = uint16_t; };
template<> struct UnpackerBufferTypeSelector<9> { using type = uint32_t; };
template<> struct UnpackerBufferTypeSelector<10> { using type = uint32_t; };
template<> struct UnpackerBufferTypeSelector<11> { using type = uint32_t; };
template<> struct UnpackerBufferTypeSelector<12> { using type = uint32_t; };
template<> struct UnpackerBufferTypeSelector<13> { using type = uint32_t; };
template<> struct UnpackerBufferTypeSelector<14> { using type = uint32_t; };
template<> struct UnpackerBufferTypeSelector<15> { using type = uint32_t; };

template<typename BufferType, std::size_t BlockSize>
struct UnpackerIntrinsics {
	
	inline void bswap_(std::array<BufferType, BlockSize> & buffers) {
		for(uint32_t i(0); i < BlockSize; ++i) {
			buffers[i] = betoh(buffers[i]);
		}
	}
};

template<uint32_t bpn>
class BitunpackerImp {
private:
	using BufferType = typename UnpackerBufferTypeSelector<bpn>::type;
	static constexpr std::size_t BitsPerNumber = bpn;
	static constexpr std::size_t BufferBytes = sizeof(BufferType);
	static constexpr std::size_t BufferSize = sizeof(BufferType);
	static constexpr std::size_t BufferBits = std::numeric_limits<BufferType>::digits;
	static constexpr BufferType mask = sserialize::createMask64(bpn);
	static constexpr bool UseSimd = false;
	static constexpr uint32_t BlockBits = bpn * (UseSimd ? 256 : BufferBits); //= std::lcm(bpn, BufferBits);
public:
	static constexpr uint32_t BlockSize = BlockBits / bpn;
	static constexpr uint32_t BlockBytes = BlockBits/8;
private:
	//first byte to read
	static constexpr uint16_t calc_begin(std::size_t i) {
		//placing the start of a number at the start of the buffer ist not possible for numbers at the end of a block
		//since these may very well start in the last byte whereas the buffer is larger than 1 Bytes
		//in order to fix this we have to place these numbers as far to the right in the buffer as possible and change the shift amount accordingly
		return (i*bpn+BufferBits > BlockBits ? BlockBytes-BufferBytes : i*bpn/8);
	}
	//We load more data into our buffer than necessary.
	//We load some bits in front of the number, these are removed by the mask
	//We also load some bits after the number, these are removed by a right-shift
	//These bits are after our number up to the end of the buffer
	static constexpr uint8_t calc_rs(std::size_t i) {
		return (calc_begin(i)+BufferSize)*8 - (i+1)*bpn;
	}
	static constexpr std::array<uint16_t, BlockSize> calc_eb() {
		using seq = std::make_integer_sequence<std::size_t, BlockSize>;
		return calc_eb_real(seq{});
	}
	template<std::size_t... I>
	static constexpr std::array<uint16_t, BlockSize> calc_eb_real(std::index_sequence<I...>) {
		return std::array<uint16_t, BlockSize>{{ calc_begin(I)... }};
	}
	static constexpr std::array<uint8_t, BlockSize> calc_rs() {
		using seq = std::make_integer_sequence<std::size_t, BlockSize>;
		return calc_rs_real(seq{});
	}
	template<std::size_t... I>
	static constexpr std::array<uint8_t, BlockSize> calc_rs_real(std::index_sequence<I...>) {
		return std::array<uint8_t, BlockSize>{{ calc_rs(I)... }};
	}
public:
	BitunpackerImp() {}
public:
	///src and dest should be random access iterators
	///value_type(source) == uint8_t and memmove(&BufferType, src, BufferSize) is available
	template<typename T_SOURCE_ITERATOR, typename T_DESTINATION_ITERATOR>
	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	inline void unpack(T_SOURCE_ITERATOR src, T_DESTINATION_ITERATOR dest) const {
		for(uint32_t i(0); i < BlockSize; ++i) {
			BufferType buffer;
			::memmove(&buffer, src+m_eb[i], BufferSize);
			buffer = betoh(buffer);
			buffer >>= m_rs[i];
			dest[i] = buffer & mask;
		}
	}
	template<typename T_SOURCE_ITERATOR, typename T_DESTINATION_ITERATOR>
	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	inline void unpack_simd(T_SOURCE_ITERATOR src, T_DESTINATION_ITERATOR dest) const {
		std::array<BufferType, BlockSize> buffers;
		for(uint32_t i(0); i < BlockSize; ++i) {
			::memmove(&(buffers[i]), src+m_eb[i], sizeof(BufferType));
			buffers[i] = betoh(buffers[i]);
		}
		for(uint32_t i(0); i < BlockSize; ++i) {
			buffers[i] >>= m_rs[i];
			buffers[i] &= mask; 
			dest[i] = buffers[i];
		}
	}
	
	///src and dest should be random access iterators
	///value_type(source) == uint8_t and memmove(&BufferType, src, BufferSize) is available
	///There are no restrictions for the ouput iterator
	template<typename T_SOURCE_ITERATOR, typename T_DESTINATION_ITERATOR>
	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	T_SOURCE_ITERATOR unpack(T_SOURCE_ITERATOR input, T_DESTINATION_ITERATOR output, std::size_t count) const {
		SSERIALIZE_CHEAP_ASSERT(count%BlockSize == 0);
		if (UseSimd) {
			for(uint32_t i(0); i < count; i += BlockSize, input += BlockBytes, output += BlockSize) {
				unpack_simd(input, output);
			}
		}
		else {
			for(uint32_t i(0); i < count; i += BlockSize, input += BlockBytes, output += BlockSize) {
				unpack(input, output);
			}
		}
		return input;
	}
private:
	static constexpr std::array<uint16_t, BlockSize> m_eb = calc_eb();
	static constexpr std::array<uint8_t, BlockSize> m_rs = calc_rs(); //(8-len)*8 + ie with len = ee-eb+uint32_t(ie>0) and ie = 8-(bitsEnd%8);
};

template<uint32_t bpn>
constexpr std::array<uint16_t, BitunpackerImp<bpn>::BlockSize> BitunpackerImp<bpn>::m_eb;

template<uint32_t bpn>
constexpr std::array<uint8_t, BitunpackerImp<bpn>::BlockSize> BitunpackerImp<bpn>::m_rs;

	
}} //end namespace detail::bitpacking


class BitunpackerInterface {
public:
	BitunpackerInterface() {}
	virtual ~BitunpackerInterface() {}
	virtual void unpack_blocks(const uint8_t* & src, uint32_t* & dest, uint32_t & count) const = 0;
	virtual void unpack_blocks(const uint8_t* & src, uint64_t* & dest, uint32_t & count) const = 0;
public:
	static std::unique_ptr<BitunpackerInterface> unpacker(uint32_t bpn);
};

template<uint32_t bpn>
class Bitunpacker: public BitunpackerInterface {
public:
	using UnpackerImp = detail::bitpacking:: BitunpackerImp<bpn>;
public:
	Bitunpacker() {}
	virtual ~Bitunpacker() {}
	virtual void unpack_blocks(const uint8_t* & src, uint32_t* & dest, uint32_t & count) const override {
		uint32_t myCount = (count/UnpackerImp::BlockSize)*UnpackerImp::BlockSize;
		src = m_unpacker.unpack(src, dest, myCount);
		dest += myCount;
		count -= myCount;
	}
	virtual void unpack_blocks(const uint8_t* & src, uint64_t* & dest, uint32_t & count) const override {
		uint32_t myCount = (count/UnpackerImp::BlockSize)*UnpackerImp::BlockSize;
		src = m_unpacker.unpack(src, dest, myCount);
		dest += myCount;
		count -= myCount;
	}
private:
	UnpackerImp m_unpacker;
};

}//end namespace sserialize

#endif
