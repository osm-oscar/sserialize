#ifndef SSERIALIZE_DEBUG_H
#define SSERIALIZE_DEBUG_H

/*
DEBUG_CHECK_KVSTORE_SERIALIZE
DEBUG_CHECK_ALL
DEBUG_CHECK_ARRAY_OFFSET_INDEX
DEBUG_CHECK_SERIALIZED_INDEX
__attribute__((optimize(0)))
*/

#if defined(DEBUG_CHECK_ALL)
#define NO_OPTIMIZE_ON_DEBUG __attribute__((optimize(0)))
#else
#define NO_OPTIMIZE_ON_DEBUG
#endif

#endif