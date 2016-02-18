#ifndef SSERIALIZE_ASSERT_H
#define SSERIALIZE_ASSERT_H
#include <sserialize/utility/constants.h>
#include <iostream>
#include <sstream>
#include <unistd.h>

namespace sserialize {

inline void __assert_function(const std::string & message) {
	while (true) {
		std::cout << message << std::endl;
		::sleep(1);
	}
}

template<typename T_STRING_CONVERTIBLE>
void assert_true_message(bool value, T_STRING_CONVERTIBLE msg) {
	if (UNLIKELY_BRANCH(!value)) {
		__assert_function("ASSERTION FAILED WITH MESSAGE="+std::string(msg));
	}
}

inline void assert_true(bool value) {
	if (UNLIKELY_BRANCH(!value)) {
		__assert_function("ASSERTION FAILED");
	}
}

template<typename T1, typename T2>
void assert_equal(const T1 & v1, const T2 & v2) {
	if (UNLIKELY_BRANCH(v1 != v2)) {
		std::stringstream ss;
		ss << "ASSERTION FAILED(==) LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_larger(const T1 & v1, const T2 & v2) {
	if (UNLIKELY_BRANCH(!(v1 > v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED(>) LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_larger_or_equal(const T1 & v1, const T2 & v2) {
	if (UNLIKELY_BRANCH(!(v1 >= v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED(>=) LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_smaller(const T1 & v1, const T2 & v2) {
	if (UNLIKELY_BRANCH(!(v1 < v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED(<) LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_smaller_or_equal(const T1 & v1, const T2 & v2) {
	if (UNLIKELY_BRANCH(!(v1 <= v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED(<=) LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

}//end namspace

#if defined(WITH_SSERIALIZE_EXPENSIVE_ASSERT)
	#ifndef WITH_SSERIALIZE_NORMAL_ASSERT
		#define WITH_SSERIALIZE_NORMAL_ASSERT
	#endif
#endif

#if defined(WITH_SSERIALIZE_NORMAL_ASSERT)
	#ifndef WITH_SSERIALIZE_CHEAP_ASSERT
		#define WITH_SSERIALIZE_CHEAP_ASSERT
	#endif
#endif

#ifdef WITH_SSERIALIZE_CHEAP_ASSERT
	#define SSERIALIZE_CHEAP_ASSERT_ENABLED
	#define SSERIALIZE_CHEAP_ASSERT(__BOOL) sserialize::assert_true(__BOOL);
	#define SSERIALIZE_CHEAP_ASSERT_MESSAGE(__BOOL, __MSG) sserialize::assert_true_message(__BOOL, __MSG);
	#define SSERIALIZE_CHEAP_ASSERT_EQUAL(__V1, __V2) sserialize::assert_equal(__V1, __V2);
	#define SSERIALIZE_CHEAP_ASSERT_LARGER(__V1, __V2) sserialize::assert_larger(__V1, __V2);
	#define SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(__V1, __V2) sserialize::assert_larger_or_equal(__V1, __V2);
	#define SSERIALIZE_CHEAP_ASSERT_SMALLER(__V1, __V2) sserialize::assert_smaller(__V1, __V2);
	#define SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(__V1, __V2) sserialize::assert_smaller_or_equal(__V1, __V2);
#else
	#define SSERIALIZE_CHEAP_ASSERT(__BOOL)
	#define SSERIALIZE_CHEAP_ASSERT_MESSAGE(__BOOL, __MSG)
	#define SSERIALIZE_CHEAP_ASSERT_EQUAL(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_LARGER(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_SMALLER(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(__V1, __V2)
#endif
	
//normal asserts
#ifdef WITH_SSERIALIZE_NORMAL_ASSERT
	#define SSERIALIZE_NORMAL_ASSERT_ENABLED
	#define SSERIALIZE_NORMAL_ASSERT(__BOOL) sserialize::assert_true(__BOOL);
	#define SSERIALIZE_NORMAL_ASSERT_MESSAGE(__BOOL, __MSG) sserialize::assert_true_message(__BOOL, __MSG);
	#define SSERIALIZE_NORMAL_ASSERT_EQUAL(__V1, __V2) sserialize::assert_equal(__V1, __V2);
	#define SSERIALIZE_NORMAL_ASSERT_LARGER(__V1, __V2) sserialize::assert_larger(__V1, __V2);
	#define SSERIALIZE_NORMAL_ASSERT_LARGER_OR_EQUAL(__V1, __V2) sserialize::assert_larger_or_equal(__V1, __V2);
	#define SSERIALIZE_NORMAL_ASSERT_SMALLER(__V1, __V2) sserialize::assert_smaller(__V1, __V2);
	#define SSERIALIZE_NORMAL_ASSERT_SMALLER_OR_EQUAL(__V1, __V2) sserialize::assert_smaller_or_equal(__V1, __V2);
#else
	#define SSERIALIZE_NORMAL_ASSERT(__BOOL)
	#define SSERIALIZE_NORMAL_ASSERT_MESSAGE(__BOOL, __MSG)
	#define SSERIALIZE_NORMAL_ASSERT_EQUAL(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_LARGER(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_LARGER_OR_EQUAL(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_SMALLER(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_SMALLER_OR_EQUAL(__V1, __V2)
#endif

//expensive asserts
#ifdef WITH_SSERIALIZE_EXPENSIVE_ASSERT
	#define SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	#define SSERIALIZE_EXPENSIVE_ASSERT(__BOOL) sserialize::assert_true(__BOOL);
	#define SSERIALIZE_EXPENSIVE_ASSERT_MESSAGE(__BOOL, __MSG) sserialize::assert_true_message(__BOOL, __MSG);
	#define SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(__V1, __V2) sserialize::assert_equal(__V1, __V2);
	#define SSERIALIZE_EXPENSIVE_ASSERT_LARGER(__V1, __V2) sserialize::assert_larger(__V1, __V2);
	#define SSERIALIZE_EXPENSIVE_ASSERT_LARGER_OR_EQUAL(__V1, __V2) sserialize::assert_larger_or_equal(__V1, __V2);
	#define SSERIALIZE_EXPENSIVE_ASSERT_SMALLER(__V1, __V2) sserialize::assert_smaller(__V1, __V2);
	#define SSERIALIZE_EXPENSIVE_ASSERT_SMALLER_OR_EQUAL(__V1, __V2) sserialize::assert_smaller_or_equal(__V1, __V2);
#else
	#define SSERIALIZE_EXPENSIVE_ASSERT(__BOOL)
	#define SSERIALIZE_EXPENSIVE_ASSERT_MESSAGE(__BOOL, __MSG)
	#define SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_LARGER(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_LARGER_OR_EQUAL(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_SMALLER(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_SMALLER_OR_EQUAL(__V1, __V2)
#endif

#endif