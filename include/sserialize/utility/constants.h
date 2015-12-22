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

#endif