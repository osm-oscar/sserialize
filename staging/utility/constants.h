#ifndef SSERIALIZE_CONSTANTS_H
#define SSERIALIZE_CONSTANTS_H

#define SSERIALIZE_NAMESPACE sserialize

//from https://stackoverflow.com/questions/295120/c-mark-as-deprecated
#ifdef __GNUC__
#define MARK_DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define MARK_DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

#endif