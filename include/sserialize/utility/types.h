#ifndef SSERIALIZE_TYPES_H
#define SSERIALIZE_TYPES_H
#include <stdint.h>

namespace sserialize {

#ifdef __LP64__
typedef uint64_t OffsetType;
typedef int64_t NegativeOffsetType;
#define MAX_SIZE_FOR_FULL_MMAP 0xFFFFFFFF
#define CHUNKED_MMAP_EXPONENT 23
#else
typedef uint32_t OffsetType;
typedef int32_t NegativeOffsetType;
#define MAX_SIZE_FOR_FULL_MMAP 0x3FFFFFFF
#define CHUNKED_MMAP_EXPONENT 23
#endif

}//end namespace

#endif