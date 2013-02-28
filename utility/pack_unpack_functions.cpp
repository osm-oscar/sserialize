#include <stdint.h>

#include "pack_unpack_functions.h"


namespace sserialize {

uint32_t vl_unpack_uint32_t(uint8_t * s, int * len) {
	uint32_t retVal = 0;
	int myLen = 0;
	do {
		retVal |= static_cast<uint32_t>(s[myLen] & 0x7F) << 7*myLen;
		myLen++;
	}
	while (myLen < 5 && s[myLen-1] & 0x80);
	
	if (len)
		*len = myLen;
		
	return retVal;
}

int vl_pack_uint32_t(uint32_t s, uint8_t * d) {
	int8_t i = 0;
	do {
		d[i] = s & 0x7F;
		s = s >> 7;
		if (s)
			d[i] |= 0x80;
		++i;
	} while (s);
	return i;
}

int vl_pack_uint32_t_size(uint32_t s) {
	int c = 0;
	do {
		++c;
		s >>= 7;
	} while(s);
	return c;
}

int vl_pack_uint32_t_pad4(uint32_t s, uint8_t * d) {
	int8_t i = 0;
	while (i < 4) {
		d[i] = (s & 0x7F) | 0x80;
		s = s >> 7;
		++i;
	}
	d[3] = d[3] & 0x7F;
	return i;
}

int32_t vl_unpack_int32_t(uint8_t * s, int * len) {
	uint32_t tmp = vl_unpack_uint32_t(s,len);
	//check if signed or unsigned
	int32_t stmp = (tmp >> 1);
	if (tmp & 0x1) return -stmp;
	return stmp;
}

int vl_pack_int32_t(int32_t s, uint8_t * d) {
	uint32_t tmp;
	if (s < 0) {
		tmp = -s;
		tmp = (tmp << 1) | 0x1;
	}
	else {
		tmp = s;
		tmp = (tmp << 1);
	}
	return vl_pack_uint32_t(tmp, d);
}

int vl_pack_int32_t_size(int32_t s) {
	uint32_t tmp;
	if (s < 0) {
		tmp = -s;
		tmp = (tmp << 1) | 0x1;
	}
	else {
		tmp = s;
		tmp = (tmp << 1);
	}
	return vl_pack_uint32_t_size(tmp);
}

int vl_pack_int32_t_pad4(int32_t s, uint8_t * d) {
	uint32_t tmp;
	if (s < 0) {
		tmp = -s;
		tmp = (tmp << 1) | 0x1;
	}
	else {
		tmp = s;
		tmp = (tmp << 1);
	}
	return vl_pack_uint32_t_pad4(tmp, d);
}

bool vl_pack_uint32_t_valid(uint32_t number) {
	return !(number >= (1 << 29) );
}

bool vl_pack_int32_t_valid(int32_t number) {
	uint32_t absnum = (number < 0 ? -number : number);
	absnum = absnum << 1;
	return !(absnum >= (1 << 29));
}


uint64_t vl_unpack_uint64_t(uint8_t * s, int * len) {
	uint64_t retVal = 0;
	int myLen = 0;
	do {
		retVal |= static_cast<uint64_t>(s[myLen] & 0x7F) << 7*myLen;
		myLen++;
	}
	while (myLen < 9 && s[myLen-1] & 0x80);
	
	if (len)
		*len = myLen;
		
	return retVal;
}

int vl_pack_uint64_t(uint64_t s, uint8_t * d) {
	int8_t i = 0;
	do {
		d[i] = s & 0x7F;
		s = s >> 7;
		if (s)
			d[i] |= 0x80;
		++i;
	} while (s);
	return i;
}

int64_t vl_unpack_int64_t(uint8_t* s, int* len) {
	uint64_t tmp = vl_unpack_uint64_t(s, len);
	//check if signed or unsigned
	int64_t stmp = (tmp >> 1);
	if (tmp & 0x1) return -stmp;
	return stmp;
}

int vl_pack_int64_t(int64_t s, uint8_t * d) {
	uint64_t tmp;
	if (s < 0) {
		tmp = -s;
		tmp = (tmp << 1) | 0x1;
	}
	else {
		tmp = s;
		tmp = (tmp << 1);
	}
	return vl_pack_uint64_t(tmp, d);
}


int vl_pack_uint64_t_size(uint64_t s) {
	int c = 0;
	do {
		++c;
		s >>= 7;
	} while(s);
	return c;
}

int vl_pack_int64_t_size(int64_t s) {
	uint64_t tmp;
	if (s < 0) {
		tmp = -s;
		tmp = (tmp << 1) | 0x1;
	}
	else {
		tmp = s;
		tmp = (tmp << 1);
	}
	return vl_pack_uint64_t_size(tmp);
}

}//end namespace