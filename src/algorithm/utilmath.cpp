#include <sserialize/algorithm/utilmath.h>

namespace sserialize {
	
	
//https://stackoverflow.com/questions/2589096/find-most-significant-bit-left-most-that-is-set-in-a-bit-array
uint32_t msb(uint32_t v) {
	static constexpr uint8_t MultiplyDeBruijnBitPosition[32] =
	{
		0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};

	v |= v >> 1; // first round down to one less than a power of 2
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;

	return MultiplyDeBruijnBitPosition[uint32_t( v * 0x07C4ACDDU ) >> 27];
}

uint32_t msb(uint64_t v) {
	if (v >> 32) {
		return msb(uint32_t(v >> 32)) + 32;
	}
	else {
		return msb(uint32_t(v));
	}
}
	
}//end namespace sserialize
