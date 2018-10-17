#ifndef SSERIALIZE_CONSTANTS_H
#define SSERIALIZE_CONSTANTS_H

#define SSERIALIZE_NAMESPACE sserialize

#ifdef __GNUC__
	#define MARK_DEPRECATED(func) func __attribute__ ((deprecated))
	#define LIKELY_BRANCH(x)    __builtin_expect (!!(x), 1)
	#define UNLIKELY_BRANCH(x)  __builtin_expect (!!(x), 0)
#elif defined(_MSC_VER)
	#define MARK_DEPRECATED(func) __declspec(deprecated) func
	#define LIKELY_BRANCH(x) x
	#define UNLIKELY_BRANCH(x) x
#else
	#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
	#define DEPRECATED(func) func
#endif

#ifdef SSERIALIZE_WITH_INLINE_IN_LTO
	#define INLINE_WITH_LTO __attribute__((used)) inline
#else
	#define INLINE_WITH_LTO
#endif

#ifdef SSERIALIZE_WITH_NO_CAST_CHECKS
	#define SSERIALIZE_NO_CAST_CHECKS
#else
	#undef SSERIALIZE_NO_CAST_CHECKS
#endif

#ifdef SSERIALIZE_WITH_NO_NULL_CHECKS
	#define SSERIALIZE_NO_NULL_CHECKS
#else
	#undef SSERIALIZE_NO_NULL_CHECKS
#endif

#endif
