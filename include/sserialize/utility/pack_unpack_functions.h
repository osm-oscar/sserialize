#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H
/* uint*_t */
#include <stdint.h>
/* memcpy */
#include <string.h>

/* make sure be32toh and be64toh are present */
#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#else
/* htons/htonl */
#include <arpa/inet.h>
#  define htobe16(x) htons(x)
#  define be16toh(x) htons(x)
#  define htobe32(x) htonl(x)
#  define be32toh(x) htonl(x)
#  define htobe64(x) my_htobe64(x)
#  define be64toh(x) htobe64(x)
static uint64_t my_htobe64(uint64_t x) {
	if (htobe32(1) == 1) return x;
	return ((uint64_t)htobe32(x)) << 32 | htobe32(x >> 32);
}

#endif

namespace sserialize {


inline uint8_t unPack_uint8_t(const uint8_t a) {
	return a;
}

inline uint16_t unPack_uint16_t(const uint8_t c, const uint8_t d) {
	return (
		(static_cast<uint16_t>(c) << 8) |
		(static_cast<uint16_t>(d) << 0)
	);
}

inline uint32_t unPack_uint24_t(const uint8_t b, const uint8_t c, const uint8_t d) {
	return (
		(static_cast<uint32_t>(b) << 16) |
		(static_cast<uint32_t>(c) << 8) |
		(static_cast<uint32_t>(d) << 0)
	);
}

inline uint32_t unPack_uint32_t(const unsigned char * src ) {
	uint32_t i;
	memcpy(&i, src, sizeof(i));
	return be32toh(i);
}

inline uint64_t unPack_uint64_t(const unsigned char * src) {
	uint64_t i;
	memcpy(&i, src, sizeof(i));
	return be64toh(i);
}

inline uint64_t unPack_uint40_t(const unsigned char * src) {
	unsigned char tmp[8] = {0};
	memcpy(tmp + 3, src, 5);
	return unPack_uint64_t(tmp);
}

inline void pack_uint8_t(uint32_t s, uint8_t * d) {
	d[0] = s;
}

inline void pack_uint16_t(uint32_t s, uint8_t * d) {
	d[0] = static_cast<uint8_t>((s & 0x0000FF00) >> 8);
	d[1] = static_cast<uint8_t>((s & 0x000000FF) >> 0); 
}

inline void pack_uint24_t(uint32_t s, uint8_t * d) {
	d[0] = static_cast<uint8_t>((s & 0x00FF0000) >> 16);
	d[1] = static_cast<uint8_t>((s & 0x0000FF00) >> 8);
	d[2] = static_cast<uint8_t>((s & 0x000000FF) >> 0); 
}

inline void pack_uint32_t(uint32_t src, unsigned char * dest) {
	src = htobe32(src);
	memcpy(dest, &src, sizeof(src));
}

inline void pack_uint40_t(uint64_t src, unsigned char * dest) {
	src = htobe64(src);
	memcpy(dest, 3 + (unsigned char*) &src, 5);
}

inline void pack_uint64_t(uint64_t src, unsigned char * dest) {
	src = htobe64(src);
	memcpy(dest, &src, sizeof(src));
}



typedef void(*PackFunctionsFuncPtr)(uint32_t s, uint8_t * d);
typedef int(*VlPackUint32FunctionsFuncPtr)(uint32_t s, uint8_t * d);
typedef int(*VlPackInt32FunctionsFuncPtr)(int32_t s, uint8_t * d);

typedef uint32_t(*VlUnPackUint32FunctionsFuncPtr)(uint8_t * s, int * len);
typedef int32_t(*VlUnPackInt32FunctionsFuncPtr)(int8_t * s, int * len);


inline uint32_t unPack_uint32_t(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) {
	uint8_t tmp[4] = {a, b, c , d};
	return unPack_uint32_t(tmp);
}

inline uint64_t unPack_uint64_t(const uint8_t i0, const uint8_t i1, const uint8_t i2, const uint8_t i3, const uint8_t i4, const uint8_t i5, const uint8_t i6, const uint8_t i7) {
	uint8_t tmp[8] = {i0, i1, i2, i3, i4, i5, i6, i7};
	return unPack_uint64_t(tmp);
}

inline uint64_t unPack_uint40_t(const uint8_t i0, const uint8_t i1, const uint8_t i2, const uint8_t i3, const uint8_t i4) {
	uint8_t tmp[5] = {i0, i1, i2, i3, i4};
	return unPack_uint40_t(tmp);
}

inline int64_t unPack_int40_t(const uint8_t * src) {
	uint64_t tmp = unPack_uint40_t(src);
	if (tmp & 0x1)
		return - (tmp >> 1);
	else
		return (tmp >> 1);
}

inline int64_t unPack_int40_t(const uint8_t i0, const uint8_t i1, const uint8_t i2, const uint8_t i3, const uint8_t i4) {
	uint64_t tmp = unPack_uint40_t(i0, i1, i2, i3, i4);
	if (tmp & 0x1)
		return - (tmp >> 1);
	else
		return (tmp >> 1);
}

inline int64_t unPack_int64_t(const uint8_t * src) {
	uint64_t tmp = unPack_uint64_t(src);
	if (tmp & 0x1)
		return - (tmp >> 1);
	else
		return (tmp >> 1);
}

inline int64_t unPack_int64_t(const uint8_t i0, const uint8_t i1, const uint8_t i2, const uint8_t i3, const uint8_t i4, const uint8_t i5, const uint8_t i6, const uint8_t i7) {
	uint64_t tmp = unPack_uint64_t(i0, i1, i2, i3, i4, i5, i6, i7);
	if (tmp & 0x1)
		return - (tmp >> 1);
	else
		return (tmp >> 1);
}

inline int32_t unPack_int32_t(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) {
	uint32_t tmp = unPack_uint32_t(a,b,c,d);
	if (tmp & 0x1)
		return - (tmp >> 1);
	else
		return (tmp >> 1);
}

inline void pack_int64_t(int64_t s, uint8_t * d) {
	uint64_t tmp;
	if (s < 0)
		tmp = (-s << 1) | 0x1;
	else
		tmp = (s << 1);
	pack_uint64_t(tmp, d);
}

inline void pack_int40_t(int64_t s, uint8_t * d) {
	uint64_t tmp;
	if (s < 0)
		tmp = (-s << 1) | 0x1;
	else
		tmp = (s << 1);
	pack_uint40_t(tmp, d);
}

inline void pack_int32_t(int32_t s, uint8_t * d) {
	uint32_t tmp;
	if (s < 0)
		tmp = (-s << 1) | 0x1;
	else
		tmp = (s << 1);
	pack_uint32_t(tmp, d);
}

inline uint32_t vl_unpack_uint32_t(uint8_t * s, int * len) {
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

inline int vl_pack_uint32_t(uint32_t s, uint8_t * d) {
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

/** @param len
 * len = length of bytes of the value
 */

int32_t vl_unpack_int32_t(uint8_t * s, int * len);

/** @param d needs at most 5 bytes
 *  @return returns the element count (-1 if failed)
 */

int vl_pack_uint32_t_size(uint32_t s);

/** @param d needs 4 bytes
 *  @return returns the element count (-1 if failed)
 */
int vl_pack_uint32_t_pad4(uint32_t s, uint8_t * d);


/** @param d needs at most 4 bytes
 *  @return returns the element count (-1 if failed)
 */
int vl_pack_int32_t(int32_t s, uint8_t * d);

int vl_pack_int32_t_size(int32_t s);


int vl_pack_int32_t_pad4(int32_t s, uint8_t * d);


bool vl_pack_uint32_t_valid(uint32_t number);
bool vl_pack_int32_t_valid(int32_t number);

uint64_t vl_unpack_uint64_t(uint8_t* s, int* len);
int vl_pack_uint64_t(uint64_t s, uint8_t * d);

int64_t vl_unpack_int64_t(uint8_t* s, int* len);
int vl_pack_int64_t(int64_t s, uint8_t * d);

int vl_pack_uint64_t_size(uint64_t s);
int vl_pack_int64_t_size(int64_t s);

}//end namespace

#endif
