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

template<uint32_t bpn>
class BitunpackerImp {
private:
	using BufferType = typename UnpackerBufferTypeSelector<bpn>::type;
	static constexpr std::size_t BufferSize = sizeof(BufferType);
	static constexpr std::size_t BufferBits = std::numeric_limits<BufferType>::digits;
	static constexpr BufferType mask = sserialize::createMask64(bpn);
	static constexpr uint32_t lcm = bpn * BufferBits; //= std::lcm(bpn, BufferBits);
public:
	static constexpr uint32_t blocksize = lcm / bpn;
	static constexpr uint32_t bytesPerBlock = lcm/8;
private:
	//first byte to read
	static constexpr uint16_t calc_begin(std::size_t i) {
		return i*bpn/8;
	}
	//one past the end
	static constexpr uint16_t calc_end(std::size_t i) {
		return (i+1)*bpn/8;
	}
	//intra byte offset
	static constexpr uint8_t calc_ie(std::size_t i) {
		return 8-((i+1)*bpn%8);
	}
	//returns the number of bytes to read
	static constexpr uint16_t calc_len(std::size_t i) {
		return calc_end(i) - calc_begin(i)+uint16_t(calc_ie(i)>0);
	}
	//rshift of our buffer
	static constexpr uint8_t calc_rs(std::size_t i) {
		return (BufferSize-calc_len(i))*8 + calc_ie(i);
	}
	static constexpr std::array<uint16_t, blocksize> calc_eb() {
		using seq = std::make_integer_sequence<std::size_t, blocksize>;
		return calc_eb_real(seq{});
	}
	template<std::size_t... I>
	static constexpr std::array<uint16_t, blocksize> calc_eb_real(std::index_sequence<I...>) {
		return std::array<uint16_t, blocksize>{{ calc_begin(I)... }};
	}
	static constexpr std::array<uint8_t, blocksize> calc_rs() {
		using seq = std::make_integer_sequence<std::size_t, blocksize>;
		return calc_rs_real(seq{});
	}
	template<std::size_t... I>
	static constexpr std::array<uint8_t, blocksize> calc_rs_real(std::index_sequence<I...>) {
		return std::array<uint8_t, blocksize>{{ calc_rs(I)... }};
	}
public:
	BitunpackerImp() {}
public:
	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	inline void unpack(const uint8_t * src, uint32_t * dest) const {
		for(uint32_t i(0); i < blocksize; ++i) {
			BufferType buffer;
			::memmove(&buffer, src+m_eb[i], BufferSize);
			buffer = betoh(buffer);
			buffer >>= m_rs[i];
			dest[i] = buffer & mask;
		}
	}
	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	const uint8_t * unpack(const uint8_t * src, uint32_t * dest, uint32_t count) const {
		SSERIALIZE_CHEAP_ASSERT(count%blocksize == 0);
		for(uint32_t i(0); i < count; i += blocksize, src += bytesPerBlock, dest += blocksize) {
			unpack(src, dest);
		}
		return src;
	}
public:
	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	inline void unpack(const uint8_t * src, uint64_t * dest) const {
		for(uint32_t i(0); i < blocksize; ++i) {
			BufferType buffer;
			::memmove(&buffer, src+m_eb[i], BufferSize);
			buffer = betoh(buffer);
			buffer >>= m_rs[i];
			dest[i] = buffer & mask;
		}
	}
	__attribute__((optimize("unroll-loops"))) __attribute__((optimize("tree-vectorize")))
	const uint8_t * unpack(const uint8_t * src, uint64_t * dest, uint32_t count) const {
		SSERIALIZE_CHEAP_ASSERT(count%blocksize == 0);
		for(uint32_t i(0); i < count; i += blocksize, src += bytesPerBlock, dest += blocksize) {
			unpack(src, dest);
		}
		return src;
	}
private:
	static constexpr std::array<uint16_t, blocksize> m_eb = calc_eb();
	static constexpr std::array<uint8_t, blocksize> m_rs = calc_rs(); //(8-len)*8 + ie with len = ee-eb+uint32_t(ie>0) and ie = 8-(bitsEnd%8);
};

template<uint32_t bpn>
constexpr std::array<uint16_t, BitunpackerImp<bpn>::blocksize> BitunpackerImp<bpn>::m_eb;

template<uint32_t bpn>
constexpr std::array<uint8_t, BitunpackerImp<bpn>::blocksize> BitunpackerImp<bpn>::m_rs;

	
}} //end namespace detail::bitpacking


class BitunpackerInterface {
public:
	BitunpackerInterface() {}
	virtual ~BitunpackerInterface() {}
	virtual void unpack(const uint8_t* & src, uint32_t* & dest, uint32_t & count) const = 0;
	virtual void unpack(const uint8_t* & src, uint64_t* & dest, uint32_t & count) const = 0;
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
	virtual void unpack(const uint8_t* & src, uint32_t* & dest, uint32_t & count) const override {
		uint32_t myCount = (count/UnpackerImp::blocksize)*UnpackerImp::blocksize;
		src = m_unpacker.unpack(src, dest, myCount);
		dest += myCount;
		count -= myCount;
	}
	virtual void unpack(const uint8_t* & src, uint64_t* & dest, uint32_t & count) const override {
		uint32_t myCount = (count/UnpackerImp::blocksize)*UnpackerImp::blocksize;
		src = m_unpacker.unpack(src, dest, myCount);
		dest += myCount;
		count -= myCount;
	}
private:
	UnpackerImp m_unpacker;
};

}//end namespace sserialize

#endif
