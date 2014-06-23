#ifndef SSERIALIZE_TYPES_H
#define SSERIALIZE_TYPES_H
#include <stdint.h>

namespace sserialize {



#ifndef TEMP_FILE_PREFIX
#define TEMP_FILE_PREFIX "/tmp/sserializetmp"
#endif

#ifndef PERSISTENT_CACHE_PATH
#define PERSISTENT_CACHE_PATH "/tmp/sserializecache"
#endif

#ifndef MAX_IN_MEMORY_CACHE
	#ifdef __ANDROID__
	#define MAX_IN_MEMORY_CACHE (10*1024*1024)
	#else
	#define MAX_IN_MEMORY_CACHE (200*1024*1024)
	#endif
#endif

#define SSERIALIZED_OFFSET_BYTE_COUNT 5
#define SSERIALIZED_NEGATIVE_OFFSET_BYTE_COUNT 5
#define EPSILON 0.00000001d
#define SSERIALIZE_SYSTEM_PAGE_SIZE 4096

///OffsetType >= SizeType >= IdType!

#ifdef __LP64__
typedef uint64_t OffsetType;
typedef int64_t SignedOffsetType;
typedef uint64_t SizeType;
typedef int64_t SignedSizeType;
typedef uint64_t IdType;
#define MAX_SIZE_FOR_FULL_MMAP 0xFFFFFFFFF
#define CHUNKED_MMAP_EXPONENT 23
#else
typedef uint32_t OffsetType;
typedef int32_t SignedOffsetType;
typedef uint32_t SizeType;
typedef int32_t SignedSizeType;
typedef uint32_t IdType;
#define MAX_SIZE_FOR_FULL_MMAP 0x3FFFFFFF
#define CHUNKED_MMAP_EXPONENT 23
#endif

}//end namespace

#endif