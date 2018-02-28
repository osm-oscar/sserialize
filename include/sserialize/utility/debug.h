#ifndef SSERIALIZE_DEBUG_H
#define SSERIALIZE_DEBUG_H

#if defined(__clang__)
	#define NO_OPTIMIZE __attribute__((optnone))
#elif defined(__GNUC__) || defined(__GNUG__)
	#define NO_OPTIMIZE __attribute__((optimize(0)))
#elif defined(_MSC_VER)
	//what should be here?
	#define NO_OPTIMIZE __attribute__((optimize(0)))
#else
	//bailout?
	#define NO_OPTIMIZE __attribute__((optimize(0)))
#endif

#define NO_INLINE __attribute__ ((noinline))

#endif
