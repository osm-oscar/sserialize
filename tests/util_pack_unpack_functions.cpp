#include <sserialize/utility/pack_unpack_functions.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

using namespace sserialize;

bool test_packFunctions2() {
	uint64_t should;
	uint64_t is;
	int len;
	uint8_t array[9];

#define VL_PACK_32_TEST(__VALUE, __LEN) \
	should = __VALUE; \
	p_vu32(should, array); \
	is = up_vu32(array, &len); \
	if (is != should) std::cout << __LEN << " byte: vl_pack/unpack wrong" << std::endl;\
	if (psize_vu32(should) != __LEN) std::cout << __LEN << " byte: psize_vu32 wrong" << std::endl; \
	
VL_PACK_32_TEST(0xFEFEFEFE, 5);
VL_PACK_32_TEST(0x0FFFFFFF, 4);
VL_PACK_32_TEST(0x000FFFFF, 3);
VL_PACK_32_TEST(0x00000FFF, 2);
VL_PACK_32_TEST(0x0000005F, 1);
VL_PACK_32_TEST(0x00000000, 1);

#undef VL_PACK_32_TEST
	
#define VL_PACK_64_TEST(__VALUE, __LEN) \
	should = __VALUE; \
	p_vu64(should, array); \
	is = up_vu64(array, &len); \
	if (is != should) std::cout << __LEN << " byte: vl_pack/unpack64 wrong" << std::endl;\
	if (psize_vu64(should) != __LEN) std::cout << __LEN << " byte: psize_vu64 wrong: " << psize_vu64(should) << std::endl; \
	
VL_PACK_64_TEST((static_cast<uint64_t>(1) << 62), 9);
VL_PACK_64_TEST((static_cast<uint64_t>(1) << 54), 8);
VL_PACK_64_TEST((static_cast<uint64_t>(1) << 47), 7);
VL_PACK_64_TEST((static_cast<uint64_t>(1) << 40), 6);
VL_PACK_64_TEST((static_cast<uint64_t>(1) << 34), 5);
VL_PACK_64_TEST((static_cast<uint64_t>(1) << 26), 4);
VL_PACK_64_TEST((static_cast<uint64_t>(1) << 19), 3);
VL_PACK_64_TEST((static_cast<uint64_t>(1) << 13), 2);
VL_PACK_64_TEST((static_cast<uint64_t>(1) << 6), 1);
VL_PACK_64_TEST(0x00000000, 1);

#undef VL_PACK_64_TEST

	
	int32_t shouldn;
	int32_t isn;

	shouldn = -0x7FEFEFEF; //32 bits
	p_vs32(shouldn, array);
	isn = up_vs32(array, &len);
	if (isn != shouldn) std::cout << "5 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (psize_vs32(shouldn) != 5) std::cout << "5 byte: psize_vs32 wrong" << std::endl;

	
	shouldn = -0x07FFFFFF; //28 bits
	p_vs32(shouldn, array);
	isn = up_vs32(array, &len);
	if (isn != shouldn) std::cout << "4 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (psize_vs32(shouldn) != 4) std::cout << "4 byte: psize_vs32 wrong" << std::endl;
	
	shouldn = -0x0007FFFF;
	p_vs32(shouldn, array);
	isn = up_vs32(array, &len);
	if (isn != shouldn) std::cout << "3 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (psize_vs32(shouldn) != 3) std::cout << "3 byte: psize_vs32 wrong" << std::endl;
	
	shouldn = -0x000007FF;
	p_vs32(shouldn, array);
	isn = up_vs32(array, &len);
	if (isn != shouldn) std::cout << "2 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (psize_vs32(shouldn) != 2) std::cout << "2 byte: psize_vs32 wrong" << std::endl;
	
	shouldn = -0x0000003F;
	p_vs32(shouldn, array);
	isn = up_vs32(array, &len);
	if (isn != shouldn) std::cout << "1 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (psize_vs32(shouldn) != 1) std::cout << "1 byte: psize_vs32 wrong" << std::endl;

	shouldn = 0x0;
	p_vs32(shouldn, array);
	isn = up_vs32(array, &len);
	if (isn != shouldn || len != 1)
		std::cout << "1 byte (0x0): vl_pack/unpack_int32_t wrong" << std::endl;
	
	return true;
}

inline bool test_packFunctions() {
	uint32_t should;
	uint32_t is;
	uint8_t array[5];
	int len;

	srand ( 0 );
	
	for(int i=0; i < 1000*1000; i++) {
		should = (double)rand()/RAND_MAX * 0x1FFFFFFF;
		if (should < 0x1FFFFFFF) {
			int orgLen = p_vu32(should, array);
			is = up_vu32(array, &len);
			if (is != should) {
				std::cout << "vl_pack/unpack_uint32_t wrong" << should << ":" << is << std::endl;
				return false;
			}
			if (len != orgLen) {
				std::cout << "vl_pack/unpack_uint32_t wrong length returned" << orgLen << ":" << len << std::endl;
				return false;

			}
		}
		else {
			std::cout << "toot high" << std::endl;
		}
	}
	
	
	for(int i=0; i < 1000*1000; i++) {
		should = (double)rand()/RAND_MAX * 0xFFFFFFF;
		if (should < 0x1FFFFFFF) {
			int orgLen = p_vu32pad4(should, array);
			is = up_vu32(array, &len);
			if (is != should) {
				std::cout << "vl_pack_/unpack_uint32_t_pad4 wrong" << should << ":" << is << std::endl;
				return false;
			}
			if (len != orgLen || orgLen != 4 || len != 4) {
				std::cout << "vl_pack/unpack_uint32_t_pad4 wrong length returned" << len << ":" << orgLen << std::endl;
				return false;

			}
		}
		else {
			std::cout << "toot high" << std::endl;
		}
	}

	int32_t shouldn;
	int32_t isn;
	
	for(int i=0; i < 1000*1000; i++) {
		shouldn = (double)rand()/RAND_MAX * 0x3FFFFFF - 0x1FFFFFF;
		if (shouldn < 0xFFFFFFF && shouldn > -0xFFFFFFF) {
			int orgLen = p_vs32(shouldn, array);
			isn = up_vs32(array, &len);
			if (isn != shouldn) {
				std::cout << "vl_pack/unpack_int32_t wrong " << shouldn << ":" << isn << std::endl;
				return false;
			}
			if (orgLen != len) {
				std::cout << "vl_pack/unpack_int32_t returned wrong length" << shouldn << ":" << isn << std::endl;
				return false;
			}
		}
		else {
			std::cout << "toot high" << std::endl;
		}
	}

	
	for(int i=0; i < 1000*1000; i++) {
		shouldn = (double)rand()/RAND_MAX * 0x3FFFFFF - 0x1FFFFFF;
		if (shouldn < 0xFFFFFFF && shouldn > -0xFFFFFFF) {
			int orgLen = p_vs32pad4(shouldn, array);
			isn = up_vs32(array, &len);
			if (isn != shouldn) {
				std::cout << "vl_pack/unpack_int32_t_pad4 wrong " << shouldn << ":" << isn << std::endl;
				return false;
			}
			if (orgLen != len || orgLen != 4) {
				std::cout << "vl_pack/unpack_int32_t_pad4 returned wrong length" << shouldn << ":" << isn << std::endl;
				return false;
			}
		}
		else {
			std::cout << "toot high" << std::endl;
		}
	}


	
	return true;
}

int main() {
	
	if (test_packFunctions2()) {
		std::cout << "Selective test passed" << std::endl;
	}
	
	if (test_packFunctions()) {
		std::cout << "Rand test passed" << std::endl;
	}
	
	return 0;
}