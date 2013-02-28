#include <sserialize/utility/pack_unpack_functions.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

using namespace sserialize;

bool test_packFunctions2() {
	uint32_t should;
	uint32_t is;
	int len;
	uint8_t array[5];

	should = 0xFEFEFEFE; //32 bits
	vl_pack_uint32_t(should, array);
	is = vl_unpack_uint32_t(array, &len);
	if (is != should) std::cout << "5 byte: vl_pack/unpack wrong" << std::endl;
	if (vl_pack_uint32_t_size(should) != 5) std::cout << "5 byte: vl_pack_uint32_t_size wrong" << std::endl;
	
	
	should = 0x0FFFFFFF; //28 bits
	vl_pack_uint32_t(should, array);
	is = vl_unpack_uint32_t(array, &len);
	if (is != should) std::cout << "4 byte: vl_pack/unpack wrong" << std::endl;
	if (vl_pack_uint32_t_size(should) != 4) std::cout << "4 byte: vl_pack_uint32_t_size wrong" << std::endl;
	
	should = 0x000FFFFF;
	vl_pack_uint32_t(should, array);
	is = vl_unpack_uint32_t(array, &len);
	if (is != should) std::cout << "3 byte: vl_pack/unpack wrong" << std::endl;
	if (vl_pack_uint32_t_size(should) != 3) std::cout << "4 byte: vl_pack_uint32_t_size wrong" << std::endl;
	
	should = 0x00000FFF;
	vl_pack_uint32_t(should, array);
	is = vl_unpack_uint32_t(array, &len);
	if (is != should) std::cout << "2 byte: vl_pack/unpack wrong" << std::endl;
	if (vl_pack_uint32_t_size(should) != 2) std::cout << "2 byte: vl_pack_uint32_t_size wrong" << std::endl;
	
	should = 0x0000005F;
	vl_pack_uint32_t(should, array);
	is = vl_unpack_uint32_t(array, &len);
	if (is != should) std::cout << "1 byte: vl_pack/unpack wrong" << std::endl;
	if (vl_pack_uint32_t_size(should) != 1) std::cout << "1 byte: vl_pack_uint32_t_size wrong" << std::endl;
	
	should = 0x0;
	vl_pack_uint32_t(should, array);
	is = vl_unpack_uint32_t(array, &len);
	if (is != should || len != 1)
		std::cout << "1 byte (0x0): vl_pack/unpack wrong" << std::endl;


	
	int32_t shouldn;
	int32_t isn;

	shouldn = -0x7FEFEFEF; //32 bits
	vl_pack_int32_t(shouldn, array);
	isn = vl_unpack_int32_t(array, &len);
	if (isn != shouldn) std::cout << "5 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (vl_pack_int32_t_size(should) != 5) std::cout << "5 byte: vl_pack_int32_t_size wrong" << std::endl;

	
	shouldn = -0x07FFFFFF; //28 bits
	vl_pack_int32_t(shouldn, array);
	isn = vl_unpack_int32_t(array, &len);
	if (isn != shouldn) std::cout << "4 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (vl_pack_int32_t_size(should) != 4) std::cout << "4 byte: vl_pack_int32_t_size wrong" << std::endl;
	
	shouldn = -0x0007FFFF;
	vl_pack_int32_t(shouldn, array);
	isn = vl_unpack_int32_t(array, &len);
	if (isn != shouldn) std::cout << "3 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (vl_pack_int32_t_size(should) != 3) std::cout << "3 byte: vl_pack_int32_t_size wrong" << std::endl;
	
	shouldn = -0x000007FF;
	vl_pack_int32_t(shouldn, array);
	isn = vl_unpack_int32_t(array, &len);
	if (isn != shouldn) std::cout << "2 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (vl_pack_int32_t_size(should) != 2) std::cout << "2 byte: vl_pack_int32_t_size wrong" << std::endl;
	
	shouldn = -0x0000003F;
	vl_pack_int32_t(shouldn, array);
	isn = vl_unpack_int32_t(array, &len);
	if (isn != shouldn) std::cout << "1 byte: vl_pack/unpack_int32_t wrong" << std::endl;
	if (vl_pack_int32_t_size(should) != 1) std::cout << "1 byte: vl_pack_int32_t_size wrong" << std::endl;

	shouldn = 0x0;
	vl_pack_int32_t(shouldn, array);
	isn = vl_unpack_int32_t(array, &len);
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
			int orgLen = vl_pack_uint32_t(should, array);
			is = vl_unpack_uint32_t(array, &len);
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
			int orgLen = vl_pack_uint32_t_pad4(should, array);
			is = vl_unpack_uint32_t(array, &len);
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
			int orgLen = vl_pack_int32_t(shouldn, array);
			isn = vl_unpack_int32_t(array, &len);
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
			int orgLen = vl_pack_int32_t_pad4(shouldn, array);
			isn = vl_unpack_int32_t(array, &len);
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