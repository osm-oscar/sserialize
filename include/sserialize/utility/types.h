#ifndef SSERIALIZE_TYPES_H
#define SSERIALIZE_TYPES_H
#include <stdint.h>
#include <limits>
#include <assert.h>

namespace sserialize {

#ifndef SSERIALIZE_TEMP_FILE_PREFIX
	#define SSERIALIZE_TEMP_FILE_PREFIX "/tmp/sserializetmp"
#endif

#ifndef SSERIALIZE_SHM_FILE_PREFIX
	#define SSERIALIZE_SHM_FILE_PREFIX "sserialize"
#endif

#ifndef SSERIALIZE_PERSISTENT_CACHE_PATH
	#define SSERIALIZE_PERSISTENT_CACHE_PATH "/tmp/sserializecache"
#endif

#ifndef SSERIALIZE_MAX_IN_MEMORY_CACHE
	#ifdef __ANDROID__
		#define SSERIALIZE_MAX_IN_MEMORY_CACHE (10*1024*1024)
	#else
		#define SSERIALIZE_MAX_IN_MEMORY_CACHE (200*1024*1024)
	#endif
#endif

#define SSERIALIZE_OFFSET_BYTE_COUNT 5
#define SSERIALIZE_NEGATIVE_OFFSET_BYTE_COUNT 5
#define SSERIALIZE_EPSILON 0.0000001l
#define SSERIALIZE_SYSTEM_PAGE_SIZE 4096

///OffsetType >= SizeType >= IdType!
//TODO:OffsetType should be signed so that off_t and OffsetType are the same, SizeType should be unsigned

typedef uint64_t OffsetType;
typedef int64_t SignedOffsetType;
typedef uint64_t SizeType;
typedef int64_t SignedSizeType;
typedef int64_t DifferenceType;
typedef uint64_t IdType;
// static_assert(sizeof(std::size_t) == sizeof(OffsetType), "sizeof(std::size_t) MUST EQUAL sizeof(OffsetType)");
static_assert(sizeof(double) == sizeof(uint64_t), "sizeof(uint64_t) MUST EQUAL sizeof(double)");
static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(uint32_t) MUST EQUAL sizeof(float)");


#ifdef __LP64__
	#define SSERIALIZE_MAX_SIZE_FOR_FULL_MMAP (std::numeric_limits<uint64_t>::max()/4)
	#define SSERIALIZE_CHUNKED_MMAP_EXPONENT 23
#else
	#define SSERIALIZE_MAX_SIZE_FOR_FULL_MMAP (std::numeric_limits<uint32_t>::max()/4)
	#define SSERIALIZE_CHUNKED_MMAP_EXPONENT 23
#endif

}//end namespace

#endif
