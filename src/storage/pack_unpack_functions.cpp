#include <sserialize/storage/pack_unpack_functions.h>

namespace sserialize {
	

template<>
INLINE_WITH_LTO int p_v<uint32_t>(uint32_t s, uint8_t * d, uint8_t * e) {
// #define COMPARE_S(__BITS) (s <= createMask(__BITS))
#define COMPARE_S(__BITS) (! (s >> (__BITS)))
	if (LIKELY_BRANCH(std::distance(d, e) >= 5)) {
		if (COMPARE_S(1*7) ) {
			d[0] = s;
			return 1;
		}
		else if (COMPARE_S(2*7)) {
			d[0] = s | 0x80;
			d[1] = s >> 7;
			return 2;
		}
		else if (COMPARE_S(3*7)) {
			d[0] = s | 0x80;
			d[1] = (s >> 7) | 0x80;
			d[2] = s >> 14;
			return 3;
		}
		else if (COMPARE_S(4*7)) {
			d[0] = s | 0x80;
			d[1] = (s >> 7) | 0x80;
			d[2] = (s >> 14) | 0x80;
			d[3] = s >> 21;
			return 4;
		}
		else {
			d[0] = s | 0x80;
			d[1] = (s >> 7) | 0x80;
			d[2] = (s >> 14) | 0x80;
			d[3] = (s >> 21) | 0x80;
			d[4] = s >> 28;
			return 5;
		}
	}
	else {
		if (COMPARE_S(1*7) ) {
			if (UNLIKELY_BRANCH(e < d+1)) {
				return -1;
			}
			d[0] = s;
			return 1;
		}
		else if (COMPARE_S(2*7)) {
			if (UNLIKELY_BRANCH(e < d+2)) {
				return -2;
			}
			d[0] = s | 0x80;
			d[1] = s >> 7;
			return 2;
		}
		else if (COMPARE_S(3*7)) {
			if (UNLIKELY_BRANCH(e < d+3)) {
				return -3;
			}
			d[0] = s | 0x80;
			d[1] = (s >> 7) | 0x80;
			d[2] = s >> 14;
			return 3;
		}
		else if (COMPARE_S(4*7)) {
			if (UNLIKELY_BRANCH(e < d+4)) {
				return -4;
			}
			d[0] = s | 0x80;
			d[1] = (s >> 7) | 0x80;
			d[2] = (s >> 14) | 0x80;
			d[3] = s >> 21;
			return 4;
		}
		else {
			if (UNLIKELY_BRANCH(e < d+5)) {
				return -5;
			}
			d[0] = s | 0x80;
			d[1] = (s >> 7) | 0x80;
			d[2] = (s >> 14) | 0x80;
			d[3] = (s >> 21) | 0x80;
			d[4] = s >> 28;
			return 5;
		}
	}
#undef COMPARE_S
}

template<>
INLINE_WITH_LTO int p_v<uint64_t>(uint64_t s, uint8_t * d, uint8_t * e) {
// #define COMPARE_S(__BITS) (s <= createMask64(__BITS))
#define COMPARE_S(__BITS) (! (s >> (__BITS)))
	if (std::distance(d, e) >= 10) {
		if (COMPARE_S(4*7)) {
			if (COMPARE_S(2*7)) {
				if (COMPARE_S(7)) {
					d[0] = s;
					return 1;
				}
				else { //s <= 2*7
					d[0] = s | 0x80;
					d[1] = (s >> 7);
					return 2;
				}
			}
			else {
				if (COMPARE_S(3*7)) {
					d[0] = s | 0x80;
					d[1] = (s >> 7) | 0x80;
					d[2] = (s >> 14);
					return 3;
				}
				else { //s <= 4*7
					d[0] = s | 0x80;
					d[1] = (s >> 7) | 0x80;
					d[2] = (s >> 14) | 0x80;
					d[3] = (s >> 21);
					return 4;
				}
			}
		}
		else {
			if (COMPARE_S(6*7)) {
				if (COMPARE_S(5*7)) {
					d[0] = s | 0x80;
					d[1] = (s >> 7) | 0x80;
					d[2] = (s >> 14) | 0x80;
					d[3] = (s >> 21) | 0x80;
					d[4] = s >> 28;
					return 5;
				}
				else { //s <= 6*7
					d[0] = s | 0x80;
					d[1] = (s >> 7) | 0x80;
					d[2] = (s >> 14) | 0x80;
					d[3] = (s >> 21) | 0x80;
					d[4] = (s >> 28) | 0x80;
					d[5] = s >> 35;
					return 6;
				}
			}
			else {
				if (COMPARE_S(8*7)) {
					if (COMPARE_S(7*7)) {
						d[0] = s | 0x80;
						d[1] = (s >> 7) | 0x80;
						d[2] = (s >> 14) | 0x80;
						d[3] = (s >> 21) | 0x80;
						d[4] = (s >> 28) | 0x80;
						d[5] = (s >> 35) | 0x80;
						d[6] = s >> 42;
						return 7;
						
					}
					else { //s <= 8*7
						d[0] = s | 0x80;
						d[1] = (s >> 7) | 0x80;
						d[2] = (s >> 14) | 0x80;
						d[3] = (s >> 21) | 0x80;
						d[4] = (s >> 28) | 0x80;
						d[5] = (s >> 35) | 0x80;
						d[6] = (s >> 42) | 0x80;
						d[7] = s >> 49;
						return 8;
					}
				}
				else {
					if (COMPARE_S(9*7)) {
						d[0] = s | 0x80;
						d[1] = (s >> 7) | 0x80;
						d[2] = (s >> 14) | 0x80;
						d[3] = (s >> 21) | 0x80;
						d[4] = (s >> 28) | 0x80;
						d[5] = (s >> 35) | 0x80;
						d[6] = (s >> 42) | 0x80;
						d[7] = (s >> 49) | 0x80;
						d[8] = s >> 56;
						return 9;
					}
					else { //s == 64
						d[0] = s | 0x80;
						d[1] = (s >> 7) | 0x80;
						d[2] = (s >> 14) | 0x80;
						d[3] = (s >> 21) | 0x80;
						d[4] = (s >> 28) | 0x80;
						d[5] = (s >> 35) | 0x80;
						d[6] = (s >> 42) | 0x80;
						d[7] = (s >> 49) | 0x80;
						d[8] = (s >> 56) | 0x80;
						d[9] = s >> 63;
						return 10;
					}
				}
			}
		}
	}
	else {
		if (COMPARE_S(4*7)) {
			if (COMPARE_S(2*7)) {
				if (COMPARE_S(7)) {
					if (UNLIKELY_BRANCH(e < d+1)) {
						return -1;
					}
					d[0] = s;
					return 1;
				}
				else { //s <= 2*7
					if (UNLIKELY_BRANCH(e < d+2)) {
						return -2;
					}
					d[0] = s | 0x80;
					d[1] = (s >> 7);
					return 2;
				}
			}
			else {
				if (COMPARE_S(3*7)) {
					if (UNLIKELY_BRANCH(e < d+3)) {
						return -3;
					}
					d[0] = s | 0x80;
					d[1] = (s >> 7) | 0x80;
					d[2] = (s >> 14);
					return 3;
				}
				else { //s <= 4*7
					if (UNLIKELY_BRANCH(e < d+4)) {
						return -4;
					}
					d[0] = s | 0x80;
					d[1] = (s >> 7) | 0x80;
					d[2] = (s >> 14) | 0x80;
					d[3] = (s >> 21);
					return 4;
				}
			}
		}
		else {
			if (COMPARE_S(6*7)) {
				if (COMPARE_S(5*7)) {
					if (UNLIKELY_BRANCH(e < d+5)) {
						return -5;
					}
					d[0] = s | 0x80;
					d[1] = (s >> 7) | 0x80;
					d[2] = (s >> 14) | 0x80;
					d[3] = (s >> 21) | 0x80;
					d[4] = s >> 28;
					return 5;
				}
				else { //s <= 6*7
					if (UNLIKELY_BRANCH(e < d+6)) {
						return -6;
					}
					d[0] = s | 0x80;
					d[1] = (s >> 7) | 0x80;
					d[2] = (s >> 14) | 0x80;
					d[3] = (s >> 21) | 0x80;
					d[4] = (s >> 28) | 0x80;
					d[5] = s >> 35;
					return 6;
				}
			}
			else {
				if (COMPARE_S(8*7)) {
					if (COMPARE_S(7*7)) {
						if (UNLIKELY_BRANCH(e < d+7)) {
							return -7;
						}
						d[0] = s | 0x80;
						d[1] = (s >> 7) | 0x80;
						d[2] = (s >> 14) | 0x80;
						d[3] = (s >> 21) | 0x80;
						d[4] = (s >> 28) | 0x80;
						d[5] = (s >> 35) | 0x80;
						d[6] = s >> 42;
						return 7;
						
					}
					else { //s <= 8*7
						if (UNLIKELY_BRANCH(e < d+8)) {
							return -8;
						}
						d[0] = s | 0x80;
						d[1] = (s >> 7) | 0x80;
						d[2] = (s >> 14) | 0x80;
						d[3] = (s >> 21) | 0x80;
						d[4] = (s >> 28) | 0x80;
						d[5] = (s >> 35) | 0x80;
						d[6] = (s >> 42) | 0x80;
						d[7] = s >> 49;
						return 8;
					}
				}
				else {
					if (COMPARE_S(9*7)) {
						if (UNLIKELY_BRANCH(e < d+9)) {
							return -9;
						}
						d[0] = s | 0x80;
						d[1] = (s >> 7) | 0x80;
						d[2] = (s >> 14) | 0x80;
						d[3] = (s >> 21) | 0x80;
						d[4] = (s >> 28) | 0x80;
						d[5] = (s >> 35) | 0x80;
						d[6] = (s >> 42) | 0x80;
						d[7] = (s >> 49) | 0x80;
						d[8] = s >> 56;
						return 9;
					}
					else { //s == 64
						if (UNLIKELY_BRANCH(e < d+10)) {
							return -10;
						}
						d[0] = s | 0x80;
						d[1] = (s >> 7) | 0x80;
						d[2] = (s >> 14) | 0x80;
						d[3] = (s >> 21) | 0x80;
						d[4] = (s >> 28) | 0x80;
						d[5] = (s >> 35) | 0x80;
						d[6] = (s >> 42) | 0x80;
						d[7] = (s >> 49) | 0x80;
						d[8] = (s >> 56) | 0x80;
						d[9] = s >> 63;
						return 10;
					}
				}
			}
		}
	}
#undef COMPARE_S
}
	
}//end namespace sserialize
