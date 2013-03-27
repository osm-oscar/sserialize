#ifndef COMPLETIONTREE_COMMON_H
#define COMPLETIONTREE_COMMON_H
#include <stdint.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include "UByteArrayAdapter.h"

namespace sserialize {

template<int TSTRIDE>
inline int16_t findKeyInArray(const UByteArrayAdapter & arrayStart, uint8_t len, uint8_t key) {
	for(int i = 0; i < len; i++) {
		if (arrayStart.at(TSTRIDE*i) == key) return i;
	}
	return -1;
}

template<int TSTRIDE>
inline int16_t findKeyInArray_uint16(const UByteArrayAdapter & arrayStart, uint16_t len, uint16_t key) {
	uint16_t srcKey;
	for(int i = 0; i < len; i++) {
		srcKey = arrayStart.getUint16(TSTRIDE*i);
		if (srcKey == key) return i;
	}
	return -1;
}

template<int TSTRIDE>
inline int32_t findKeyInArray_uint24(const UByteArrayAdapter & arrayStart, uint32_t len, uint32_t key) {
	uint32_t srcKey;
	for(int i = 0; i < len; i++) {
		srcKey = arrayStart.getUint24(TSTRIDE*i);
		if (srcKey == key) return i;
	}
	return -1;
}

template<int TSTRIDE>
inline int32_t findKeyInArray_uint32(const UByteArrayAdapter & arrayStart, uint32_t len, uint32_t key) {
	uint32_t srcKey;
	for(uint32_t i = 0; i < len; i++) {
		srcKey = arrayStart.getUint32(TSTRIDE*i);
		if (srcKey == key)
			return i;
	}
	return -1;
}

template<int TSTRIDE>
inline int16_t binarySearchKeyInArray(const UByteArrayAdapter & arrayStart, uint8_t len, uint8_t key) {
	if (len == 0) return -1;
	int16_t left = 0;
	int16_t right = len-1;
	int16_t mid = (right-left)/2+left;
// 		while (left != right) {
	while( left < right ) {
		if (arrayStart.at(TSTRIDE*mid) == key) return mid;
		if (arrayStart.at(TSTRIDE*mid) < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	return ((arrayStart.at(TSTRIDE*mid) == key) ? mid : -1);
}

template<int TSTRIDE>
inline int16_t binarySearchKeyInArray_uint16(const UByteArrayAdapter & arrayStart, int16_t len, uint16_t key) {
	if (len == 0) return -1;
	int16_t left = 0;
	int16_t right = len-1;
	int16_t mid = (right-left)/2+left;
	uint16_t srcKey;
	while( left < right ) {
		srcKey = arrayStart.getUint16(TSTRIDE*mid);
		if (srcKey == key) return mid;
		if (srcKey < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	srcKey = arrayStart.getUint16(TSTRIDE*mid);
	return ((srcKey == key) ? mid : -1);
}

template<int TSTRIDE>
inline int32_t binarySearchKeyInArray_uint24(const UByteArrayAdapter & arrayStart, int32_t len, uint32_t key) {
	if (len == 0) return -1;
	int32_t left = 0;
	int32_t right = len-1;
	int32_t mid = (right-left)/2+left;
	uint32_t srcKey;
	while( left < right ) {
		srcKey = arrayStart.getUint24(TSTRIDE*mid);
		if (srcKey == key) return mid;
		if (srcKey < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	srcKey = arrayStart.getUint24(TSTRIDE*mid);
	return ((srcKey == key) ? mid : -1);
}

template<int TSTRIDE>
inline int32_t binarySearchKeyInArray_uint32(const UByteArrayAdapter & arrayStart, int32_t len, uint32_t key) {
	if (len == 0) return -1;
	int32_t left = 0;
	int32_t right = len-1;
	int32_t mid = (right-left)/2+left;
	uint32_t srcKey;
	while( left < right ) {
		srcKey = arrayStart.getUint32(TSTRIDE*mid);
		if (srcKey == key) return mid;
		if (srcKey < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	srcKey = arrayStart.getUint32(TSTRIDE*mid);
	return ((srcKey == key) ? mid : -1);
}




//------------------------------------------------------------------------------------
template<int TSTRIDE>
inline int16_t findKeyInArray(uint8_t * arrayStart, uint8_t len, uint8_t key) {
	for(int i = 0; i < len; i++) {
		if (arrayStart[TSTRIDE*i] == key) return i;
	}
	return -1;
}

template<int TSTRIDE>
inline int16_t findKeyInArray_uint16(uint8_t * arrayStart, uint16_t len, uint16_t key) {
	uint16_t srcKey;
	for(int i = 0; i < len; i++) {
		srcKey = unPack_uint16_t(arrayStart[TSTRIDE*i], arrayStart[TSTRIDE*i+1]);
		if (srcKey == key) return i;
	}
	return -1;
}

template<int TSTRIDE>
inline int32_t findKeyInArray_uint24(uint8_t * arrayStart, uint32_t len, uint32_t key) {
	uint32_t srcKey;
	for(int i = 0; i < len; i++) {
		srcKey = unPack_uint24_t(arrayStart[TSTRIDE*i], arrayStart[TSTRIDE*i+1], arrayStart[TSTRIDE*i+2]);
		if (srcKey == key) return i;
	}
	return -1;
}

template<int TSTRIDE>
inline int32_t findKeyInArray_uint32(uint8_t * arrayStart, uint32_t len, uint32_t key) {
	uint32_t srcKey;
	for(int i = 0; i < len; i++) {
		srcKey = unPack_uint32_t(arrayStart[TSTRIDE*i], arrayStart[TSTRIDE*i+1], arrayStart[TSTRIDE*i+2], arrayStart[TSTRIDE*i+3]);
		if (srcKey == key) return i;
	}
	return -1;
}

template<int TSTRIDE>
inline int16_t binarySearchKeyInArray(uint8_t * arrayStart, uint8_t len, uint8_t key) {
	if (len == 0) return -1;
	int16_t left = 0;
	int16_t right = len-1;
	int16_t mid = (right-left)/2+left;
// 		while (left != right) {
	while( left < right ) {
		if (arrayStart[TSTRIDE*mid] == key) return mid;
		if (arrayStart[TSTRIDE*mid] < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	return ((arrayStart[TSTRIDE*mid] == key) ? mid : -1);
}

template<int TSTRIDE>
inline int16_t binarySearchKeyInArray_uint16(uint8_t * arrayStart, int16_t len, uint16_t key) {
	if (len == 0) return -1;
	int16_t left = 0;
	int16_t right = len-1;
	int16_t mid = (right-left)/2+left;
	uint16_t srcKey;
	while( left < right ) {
		srcKey = unPack_uint16_t(arrayStart[TSTRIDE*mid], arrayStart[TSTRIDE*mid+1]);
		if (srcKey == key) return mid;
		if (srcKey < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	srcKey = unPack_uint16_t(arrayStart[TSTRIDE*mid], arrayStart[TSTRIDE*mid+1]);
	return ((srcKey == key) ? mid : -1);
}

template<int TSTRIDE>
inline int32_t binarySearchKeyInArray_uint24(uint8_t * arrayStart, int32_t len, uint32_t key) {
	if (len == 0) return -1;
	int32_t left = 0;
	int32_t right = len-1;
	int32_t mid = (right-left)/2+left;
	uint32_t srcKey;
	while( left < right ) {
		srcKey = unPack_uint24_t(arrayStart[TSTRIDE*mid], arrayStart[TSTRIDE*mid+1], arrayStart[TSTRIDE*mid+2]);
		if (srcKey == key) return mid;
		if (srcKey < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	srcKey = unPack_uint24_t(arrayStart[TSTRIDE*mid], arrayStart[TSTRIDE*mid+1], arrayStart[TSTRIDE*mid+2]);
	return ((srcKey == key) ? mid : -1);
}

template<int TSTRIDE>
inline int32_t binarySearchKeyInArray_uint32(uint8_t * arrayStart, int32_t len, uint32_t key) {
	if (len == 0) return -1;
	int32_t left = 0;
	int32_t right = len-1;
	int32_t mid = (right-left)/2+left;
	uint32_t srcKey;
	while( left < right ) {
		srcKey = unPack_uint32_t(arrayStart[TSTRIDE*mid], arrayStart[TSTRIDE*mid+1], arrayStart[TSTRIDE*mid+2], arrayStart[TSTRIDE*mid+3]);
		if (srcKey == key) return mid;
		if (srcKey < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	srcKey = unPack_uint32_t(arrayStart[TSTRIDE*mid], arrayStart[TSTRIDE*mid+1], arrayStart[TSTRIDE*mid+2], arrayStart[TSTRIDE*mid+3]);
	return ((srcKey == key) ? mid : -1);
}

}//end namespace

#endif