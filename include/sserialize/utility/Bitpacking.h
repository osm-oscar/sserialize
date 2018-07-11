#ifndef SSERIALIZE_BIT_PACKING_H
#define SSERIALIZE_BIT_PACKING_H
#include <sserialize/utility/constants.h>
#include <sserialize/utility/assert.h>
#include <sserialize/algorithm/utilmath.h>
#include <sserialize/algorithm/utilfunctional.h>

#include <memory>
#include <limits>
#include <numeric>
#include <utility>
#include <string.h>
#include <x86intrin.h>

namespace sserialize {
namespace detail {
namespace bitpacking {

inline uint64_t htobe(uint64_t v) { return htobe64(v); }
inline uint32_t htobe(uint32_t v) { return htobe32(v); }
inline uint16_t htobe(uint16_t v) { return htobe16(v); }
inline uint8_t htobe(uint8_t v) { return v; }

inline uint64_t betoh(uint64_t v) { return be64toh(v); }
inline uint32_t betoh(uint32_t v) { return be32toh(v); }
inline uint16_t betoh(uint16_t v) { return be16toh(v); }
inline uint8_t betoh(uint8_t v) { return v; }

template<uint32_t bpn>
struct BitpackingBufferTypeSelector {
	using type = uint64_t;
};

template<> struct BitpackingBufferTypeSelector<1> { using type = uint8_t; };
template<> struct BitpackingBufferTypeSelector<2> { using type = uint8_t; };
template<> struct BitpackingBufferTypeSelector<4> { using type = uint8_t; };
template<> struct BitpackingBufferTypeSelector<8> { using type = uint8_t; };
template<> struct BitpackingBufferTypeSelector<16> { using type = uint16_t; };
template<> struct BitpackingBufferTypeSelector<32> { using type = uint32_t; };

template<> struct BitpackingBufferTypeSelector<3> { using type = uint16_t; };
template<> struct BitpackingBufferTypeSelector<5> { using type = uint16_t; };
template<> struct BitpackingBufferTypeSelector<6> { using type = uint16_t; };
template<> struct BitpackingBufferTypeSelector<7> { using type = uint16_t; };
template<> struct BitpackingBufferTypeSelector<9> { using type = uint32_t; };
template<> struct BitpackingBufferTypeSelector<10> { using type = uint32_t; };
template<> struct BitpackingBufferTypeSelector<11> { using type = uint32_t; };
template<> struct BitpackingBufferTypeSelector<12> { using type = uint32_t; };
template<> struct BitpackingBufferTypeSelector<13> { using type = uint32_t; };
template<> struct BitpackingBufferTypeSelector<14> { using type = uint32_t; };
template<> struct BitpackingBufferTypeSelector<15> { using type = uint32_t; };

template<typename BufferType, std::size_t BlockSize>
struct BitpackingIntrinsics {
	
	inline void bswap_(std::array<BufferType, BlockSize> & buffers) {
		for(uint32_t i(0); i < BlockSize; ++i) {
			buffers[i] = betoh(buffers[i]);
		}
	}
};

template<uint32_t bpn>
class BitpackingImp {
private:
	using BufferType = typename BitpackingBufferTypeSelector<bpn>::type;
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
private: //packing stuff
	
	static constexpr uint16_t calc_up_cpb(std::size_t i) {
		return (i*bpn+BufferBits > BlockBits ? BlockBytes-BufferBytes : i*bpn/8);
	}
	static constexpr uint8_t calc_up_cps(std::size_t i) {
		return calc_up_cpb(i+1) - calc_up_cpb(i);
	}
	static constexpr uint8_t calc_up_ls(std::size_t i) {
		return std::min<uint32_t>(BlockBytes - (i*bpn)/8, BufferBytes)*8 - bpn - ((i*bpn)%8);
	}
	
	template<std::size_t... I>
	static constexpr std::array<uint16_t, BlockSize> calc_up_cpb_real(std::index_sequence<I...>) {
		return std::array<uint16_t, BlockSize>{{ calc_up_cpb(I)... }};
	}
	template<std::size_t... I>
	static constexpr std::array<uint8_t, BlockSize> calc_up_cps_real(std::index_sequence<I...>) {
		return std::array<uint8_t, BlockSize>{{ calc_up_cps(I)... }};
	}
	template<std::size_t... I>
	static constexpr std::array<uint8_t, BlockSize> calc_up_ls_real(std::index_sequence<I...>) {
		return std::array<uint8_t, BlockSize>{{ calc_up_ls(I)... }};
	}
	
	static constexpr std::array<uint16_t, BlockSize> calc_up_cpb() {
		using seq = std::make_integer_sequence<std::size_t, BlockSize>;
		return calc_up_cpb_real(seq{});
	}
	static constexpr std::array<uint8_t, BlockSize> calc_up_cps() {
		using seq = std::make_integer_sequence<std::size_t, BlockSize>;
		return calc_up_cps_real(seq{});
	}
	static constexpr std::array<uint8_t, BlockSize> calc_up_ls() {
		using seq = std::make_integer_sequence<std::size_t, BlockSize>;
		return calc_up_ls_real(seq{});
	}
public:
	BitpackingImp() {}
public:
	///src and dest should be random access iterators
	///value_type(source) == uint8_t and memmove(&BufferType, src, BufferSize) is available
	template<typename T_SOURCE_ITERATOR, typename T_DESTINATION_ITERATOR>
	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	inline void unpack(T_SOURCE_ITERATOR src, T_DESTINATION_ITERATOR dest) const {
		struct Executer {
			T_SOURCE_ITERATOR src;
			T_DESTINATION_ITERATOR dest;
			Executer(T_SOURCE_ITERATOR src, T_DESTINATION_ITERATOR dest) : src(src), dest(dest) {}
			Executer(const Executer & other) = delete;
			inline void operator()(const uint32_t i) {
				BufferType buffer;
				::memmove(&buffer, src+m_eb[i], BufferSize);
				buffer = betoh(buffer);
				buffer >>= m_rs[i];
				dest[i] = buffer & mask;
			}
		} e(src, dest);
		sserialize::static_range_for<uint32_t, uint32_t(0), uint32_t(BlockSize)>(e);
// 		for(uint32_t i(0); i < BlockSize; ++i) {
// 			e(i);
// 		}
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
	
	//Pack BlockSize elements into BlockBits bits into output
	template<typename T_SOURCE_ITERATOR, typename T_DESTINATION_ITERATOR>
// 	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	inline void pack(T_SOURCE_ITERATOR input, T_DESTINATION_ITERATOR output) const {
		struct Executer {
			BufferType flushBuffer{0};
			T_SOURCE_ITERATOR input;
			T_DESTINATION_ITERATOR output;
			Executer(T_SOURCE_ITERATOR input, T_DESTINATION_ITERATOR output) : input(input), output(output) {}
			Executer(const Executer & other) = delete;
			inline void operator()(const uint32_t i) {
				BufferType buffer = input[i] & mask;
				buffer <<= m_up_ls[i];
				buffer = htobe(buffer);
				flushBuffer |= buffer;
				
				::memmove(output+m_up_cpb[i], &flushBuffer, BufferBytes);

				//move out the bits already copied
				//do this in two steps to avoid undefined behavior in case 2*shiftamount == BufferBits
				int shiftamount = 4*m_up_cps[i];
				#if __BYTE_ORDER == __LITTLE_ENDIAN
				flushBuffer >>= shiftamount;
				flushBuffer >>= shiftamount;
				#else
				flushBuffer <<= shiftamount;
				flushBuffer <<= shiftamount;
				#endif
			}
		} e(input, output);
		sserialize::static_range_for<uint32_t, uint32_t(0), uint32_t(BlockSize)>(e);
// 		for(uint32_t i(0); i < BlockSize; ++i) {
// 			e(i);
// 		}
	}
	
	template<typename T_SOURCE_ITERATOR, typename T_DESTINATION_ITERATOR>
	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	T_DESTINATION_ITERATOR pack(T_SOURCE_ITERATOR input, T_DESTINATION_ITERATOR output, std::size_t count) const {
		SSERIALIZE_CHEAP_ASSERT(count%BlockSize == 0);
		for(uint32_t i(0); i < count; i += BlockSize, input += BlockSize, output += BlockBytes) {
			pack(input, output);
		}
		return output;
	}
	
private: //unpacking
	static constexpr std::array<uint16_t, BlockSize> m_eb = calc_eb();
	static constexpr std::array<uint8_t, BlockSize> m_rs = calc_rs(); //(8-len)*8 + ie with len = ee-eb+uint32_t(ie>0) and ie = 8-(bitsEnd%8);
private: //packing
	static constexpr std::array<uint16_t, BlockSize> m_up_cpb = calc_up_cpb(); //copy begin
	static constexpr std::array<uint8_t, BlockSize> m_up_cps = calc_up_cps(); //number of bytes to copy from the flush buffer
	static constexpr std::array<uint8_t, BlockSize> m_up_ls = calc_up_ls(); //left shift to align to destination
};

template<uint32_t bpn>
constexpr std::array<uint16_t, BitpackingImp<bpn>::BlockSize> BitpackingImp<bpn>::m_eb;

template<uint32_t bpn>
constexpr std::array<uint8_t, BitpackingImp<bpn>::BlockSize> BitpackingImp<bpn>::m_rs;

template<uint32_t bpn>
constexpr std::array<uint16_t, BitpackingImp<bpn>::BlockSize> BitpackingImp<bpn>::m_up_cpb;

template<uint32_t bpn>
constexpr std::array<uint8_t, BitpackingImp<bpn>::BlockSize> BitpackingImp<bpn>::m_up_cps;

template<uint32_t bpn>
constexpr std::array<uint8_t, BitpackingImp<bpn>::BlockSize> BitpackingImp<bpn>::m_up_ls;

	
}} //end namespace detail::bitpacking


class BitpackingInterface {
public:
	BitpackingInterface() {}
	virtual ~BitpackingInterface() {}
public:
	virtual void unpack_blocks(const uint8_t* & src, uint32_t* & dest, uint32_t & count) const = 0;
	virtual void unpack_blocks(const uint8_t* & src, uint64_t* & dest, uint32_t & count) const = 0;
public:
	virtual void pack_blocks(const uint32_t* & src, uint8_t* & dest, uint32_t & count) const = 0;
	virtual void pack_blocks(const uint64_t* & src, uint8_t* & dest, uint32_t & count) const = 0;
public:
	static std::unique_ptr<BitpackingInterface> instance(uint32_t bpn);
};

template<uint32_t bpn>
class Bitpacking: public BitpackingInterface {
public:
	using BitpackingImp = detail::bitpacking:: BitpackingImp<bpn>;
public:
	Bitpacking() {}
	virtual ~Bitpacking() {}
public:
	virtual void unpack_blocks(const uint8_t* & src, uint32_t* & dest, uint32_t & count) const override {
		uint32_t myCount = (count/BitpackingImp::BlockSize)*BitpackingImp::BlockSize;
		src = m_p.unpack(src, dest, myCount);
		dest += myCount;
		count -= myCount;
	}
	virtual void unpack_blocks(const uint8_t* & src, uint64_t* & dest, uint32_t & count) const override {
		uint32_t myCount = (count/BitpackingImp::BlockSize)*BitpackingImp::BlockSize;
		src = m_p.unpack(src, dest, myCount);
		dest += myCount;
		count -= myCount;
	}
public:
	virtual void pack_blocks(const uint32_t* & src, uint8_t* & dest, uint32_t & count) const override {
		uint32_t myCount = (count/BitpackingImp::BlockSize)*BitpackingImp::BlockSize;
		dest = m_p.pack(src, dest, myCount);
		src += myCount;
		count -= myCount;
	}
	virtual void pack_blocks(const uint64_t* & src, uint8_t* & dest, uint32_t & count) const override {
		uint32_t myCount = (count/BitpackingImp::BlockSize)*BitpackingImp::BlockSize;
		dest = m_p.pack(src, dest, myCount);
		src += myCount;
		count -= myCount;
	}
private:
	BitpackingImp m_p;
};

}//end namespace sserialize

#endif
