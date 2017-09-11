#ifndef SSERIALIZE_TYPES_H
#define SSERIALIZE_TYPES_H
#include <stdint.h>
#include <limits>
#include <assert.h>

namespace sserialize {

#ifndef TEMP_FILE_PREFIX
#define TEMP_FILE_PREFIX "/tmp/sserializetmp"
#endif

#ifndef SHM_FILE_PREFIX
#define SHM_FILE_PREFIX "sserialize"
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
#define EPSILON 0.0000001l
#define SSERIALIZE_SYSTEM_PAGE_SIZE 4096

///OffsetType >= SizeType >= IdType!
//TODO:OffsetType should be signed so that off_t and OffsetType are the same, SizeType should be unsigned

typedef uint64_t OffsetType;
typedef int64_t SignedOffsetType;
typedef uint64_t SizeType;
typedef int64_t SignedSizeType;
typedef int64_t DifferenceType;
typedef uint64_t IdType;
static_assert(sizeof(std::size_t) == sizeof(OffsetType), "sizeof(std::size_t) MUST EQUAL sizeof(OffsetType)");
static_assert(sizeof(double) == sizeof(uint64_t), "sizeof(uint64_t) MUST EQUAL sizeof(double)");
static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(uint32_t) MUST EQUAL sizeof(float)");


#ifdef __LP64__
	#define MAX_SIZE_FOR_FULL_MMAP 0xFFFFFFFFFF
	#define CHUNKED_MMAP_EXPONENT 23
#else
	#define MAX_SIZE_FOR_FULL_MMAP 0x3FFFFFFF
	#define CHUNKED_MMAP_EXPONENT 23
#endif

}//end namespace

#endif