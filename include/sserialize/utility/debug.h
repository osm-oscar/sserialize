#ifndef SSERIALIZE_DEBUG_H
#define SSERIALIZE_DEBUG_H

#if defined(DEBUG_CHECK_ALL)
	#define DEBUG_CHECK_KVSTORE_SERIALIZE 1
	#define DEBUG_CHECK_ARRAY_OFFSET_INDEX 1
	#define DEBUG_CHECK_SERIALIZED_INDEX 1
	#define DEBUG_CHECK_HASH_BASED_FLAT_TRIE 1
	#define NO_OPTIMIZE_ON_DEBUG __attribute__((optimize(0)))
	#define NO_INLINE_ON_DEBUG __attribute__ ((noinline))
#else
	#define NO_OPTIMIZE_ON_DEBUG
#endif

#define NO_OPTIMIZE __attribute__((optimize(0)))
#define NO_INLINE __attribute__ ((noinline))

#endif