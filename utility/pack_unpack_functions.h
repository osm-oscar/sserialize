#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>


namespace sserialize {

typedef void(*PackFunctionsFuncPtr)(uint32_t s, uint8_t * d);
typedef int(*VlPackUint32FunctionsFuncPtr)(uint32_t s, uint8_t * d);
typedef int(*VlPackInt32FunctionsFuncPtr)(int32_t s, uint8_t * d);

typedef uint32_t(*VlUnPackUint32FunctionsFuncPtr)(uint8_t * s, int * len);
typedef int32_t(*VlUnPackInt32FunctionsFuncPtr)(int8_t * s, int * len);

inline uint32_t unPack_uint32_t(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) {
	return (
		((static_cast<uint32_t>(a) << 24) |
		(static_cast<uint32_t>(b) << 16)) |
		((static_cast<uint32_t>(c) << 8) |
		(static_cast<uint32_t>(d) << 0))
	);
}

inline uint32_t unPack_uint32_t(const uint8_t * src) {
	return unPack_uint32_t(src[0], src[1], src[2], src[3]);
}

inline uint64_t unPack_uint64_t(const uint8_t * src) {
	return (static_cast<uint64_t>( unPack_uint32_t(src) )  << 32) | static_cast<uint64_t>( unPack_uint32_t(&src[4]) );
}

inline uint64_t unPack_uint64_t(const uint8_t i0, const uint8_t i1, const uint8_t i2, const uint8_t i3, const uint8_t i4, const uint8_t i5, const uint8_t i6, const uint8_t i7) {
	return (static_cast<uint64_t>( unPack_uint32_t(i0, i1, i2, i3) )  << 32) | static_cast<uint64_t>( unPack_uint32_t(i4, i5, i6, i7) );
}

inline uint64_t unPack_uint40_t(const uint8_t i0, const uint8_t i1, const uint8_t i2, const uint8_t i3, const uint8_t i4) {
	return (
		((static_cast<uint64_t>(i0) << 32) |
		(static_cast<uint64_t>(i1) << 24)) |
		((static_cast<uint64_t>(i2) << 16) |
		(static_cast<uint64_t>(i3) << 8))
		| static_cast<uint64_t>(i4)
	);
}

inline uint64_t unPack_uint40_t(const uint8_t * src) {
	return unPack_uint40_t(src[0], src[1], src[2], src[3], src[4]);
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

inline uint32_t unPack_uint24_t(const uint8_t b, const uint8_t c, const uint8_t d) {
	return (
		(static_cast<uint32_t>(b) << 16) |
		(static_cast<uint32_t>(c) << 8) |
		(static_cast<uint32_t>(d) << 0)
	);
}

inline uint16_t unPack_uint16_t(const uint8_t c, const uint8_t d) {
	return (
		(static_cast<uint32_t>(c) << 8) |
		(static_cast<uint32_t>(d) << 0)
	);
}

inline uint8_t unPack_uint8_t(const uint8_t a) {
	return a;
}

inline void pack_uint64_t(uint64_t s, uint8_t * d) {
	d[0] = static_cast<uint8_t>((s >> 56) & 0xFF);
	d[1] = static_cast<uint8_t>((s >> 48) & 0xFF);
	d[2] = static_cast<uint8_t>((s >> 40) & 0xFF);
	d[3] = static_cast<uint8_t>((s >> 32) & 0xFF);
	d[4] = static_cast<uint8_t>((s >> 24) & 0xFF);
	d[5] = static_cast<uint8_t>((s >> 16) & 0xFF);
	d[6] = static_cast<uint8_t>((s >> 8) & 0xFF);
	d[7] = static_cast<uint8_t>((s >> 0) & 0xFF);
}

inline void pack_uint40_t(uint64_t s, uint8_t * d) {
	d[0] = static_cast<uint8_t>((s >> 32) & 0xFF);
	d[1] = static_cast<uint8_t>((s >> 24) & 0xFF);
	d[2] = static_cast<uint8_t>((s >> 16) & 0xFF);
	d[3] = static_cast<uint8_t>((s >> 8) & 0xFF);
	d[4] = static_cast<uint8_t>((s >> 0) & 0xFF);
}

inline void pack_uint32_t(uint32_t s, uint8_t * d) {
	d[0] = static_cast<uint8_t>((s & 0xFF000000) >> 24);
	d[1] = static_cast<uint8_t>((s & 0x00FF0000) >> 16);
	d[2] = static_cast<uint8_t>((s & 0x0000FF00) >> 8);
	d[3] = static_cast<uint8_t>((s & 0x000000FF) >> 0); 
}

inline void pack_uint24_t(uint32_t s, uint8_t * d) {
	d[0] = static_cast<uint8_t>((s & 0x00FF0000) >> 16);
	d[1] = static_cast<uint8_t>((s & 0x0000FF00) >> 8);
	d[2] = static_cast<uint8_t>((s & 0x000000FF) >> 0); 
}

inline void pack_uint16_t(uint32_t s, uint8_t * d) {
	d[0] = static_cast<uint8_t>((s & 0x0000FF00) >> 8);
	d[1] = static_cast<uint8_t>((s & 0x000000FF) >> 0); 
}

inline void pack_uint8_t(uint32_t s, uint8_t * d) {
	d[0] = s;
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
	pack_uint64_t(tmp, d);
}

inline void pack_int32_t(int32_t s, uint8_t * d) {
	uint32_t tmp;
	if (s < 0)
		tmp = (-s << 1) | 0x1;
	else
		tmp = (s << 1);
	pack_uint32_t(tmp, d);
}

/** @param len
 * len = length of bytes of the value
 */
uint32_t vl_unpack_uint32_t(uint8_t * s, int * len);

int32_t vl_unpack_int32_t(uint8_t * s, int * len);

/** @param d needs at most 5 bytes
 *  @return returns the element count (-1 if failed)
 */
int vl_pack_uint32_t(uint32_t s, uint8_t * d);

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
