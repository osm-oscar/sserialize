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

inline void assert_true(bool value, const char * msg) {
	if (UNLIKELY_BRANCH(!value)) {
		std::stringstream ss;
		ss << "ASSERTION FAILED!" << msg;
		__assert_function(ss.str());
	}
}

inline void assert_true_message(bool value, const std::string & msg) {
	return assert_true(value, msg.c_str());
}

template<typename T1, typename T2>
void assert_equal(const T1 & v1, const T2 & v2, const char * msg) {
	if (UNLIKELY_BRANCH(!(v1 == v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED!" << msg << " with LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_notequal(const T1 & v1, const T2 & v2, const char * msg) {
	if (UNLIKELY_BRANCH(!(v1 != v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED!" << msg << " with LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_larger(const T1 & v1, const T2 & v2, const char * msg) {
	if (UNLIKELY_BRANCH(!(v1 > v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED!" << msg << " with LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_larger_or_equal(const T1 & v1, const T2 & v2, const char * msg) {
	if (UNLIKELY_BRANCH(!(v1 >= v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED!" << msg << " with LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_smaller(const T1 & v1, const T2 & v2, const char * msg) {
	if (UNLIKELY_BRANCH(!(v1 < v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED!" << msg << " with LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_smaller_or_equal(const T1 & v1, const T2 & v2, const char * msg) {
	if (UNLIKELY_BRANCH(!(v1 <= v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED!" << msg << " with LEFT=" << v1 << ", RIGHT=" << v2;
		__assert_function(ss.str());
	}
}

}//end namspace

#if defined(WITH_SSERIALIZE_EXPENSIVE_ASSERT)
	#ifndef WITH_SSERIALIZE_NORMAL_ASSERT
		#define WITH_SSERIALIZE_NORMAL_ASSERT
	#endif
	#ifndef WITH_SSERIALIZE_CHEAP_ASSERT
		#define WITH_SSERIALIZE_CHEAP_ASSERT
	#endif
#endif

#if defined(WITH_SSERIALIZE_NORMAL_ASSERT)
	#ifndef WITH_SSERIALIZE_CHEAP_ASSERT
		#define WITH_SSERIALIZE_CHEAP_ASSERT
	#endif
#endif

#define SSERIALIZE_ASSERT_WITH_LINE(__BOOL, __LN) sserialize::assert_true(__BOOL, "In File " __FILE__ " in Line " __LN ":" #__BOOL);
#define SSERIALIZE_ASSERT_MESSAGE_WITH_LINE(__BOOL, __MSG, __LN) sserialize::assert_true_message(__BOOL, __MSG);
#define SSERIALIZE_ASSERT_EQUAL_WITH_LINE(__V1, __V2, __LN) sserialize::assert_equal(__V1, __V2, "In File " __FILE__ " in Line " __LN ":" #__V1 " == " #__V2);
#define SSERIALIZE_ASSERT_NOT_EQUAL_WITH_LINE(__V1, __V2, __LN) sserialize::assert_notequal(__V1, __V2, "In File " __FILE__ " in Line " __LN ":" #__V1 " != " #__V2);
#define SSERIALIZE_ASSERT_LARGER_WITH_LINE(__V1, __V2, __LN) sserialize::assert_larger(__V1, __V2, "In File " __FILE__ " in Line " __LN ":" #__V1 " > " #__V2);
#define SSERIALIZE_ASSERT_LARGER_OR_EQUAL_WITH_LINE(__V1, __V2, __LN) sserialize::assert_larger_or_equal(__V1, __V2, "In File " __FILE__ " in Line " __LN ":" #__V1 " >= " #__V2);
#define SSERIALIZE_ASSERT_SMALLER_WITH_LINE(__V1, __V2, __LN) sserialize::assert_smaller(__V1, __V2, "In File " __FILE__ " in Line " __LN ":" #__V1 " < " #__V2);
#define SSERIALIZE_ASSERT_SMALLER_OR_EQUAL_WITH_LINE(__V1, __V2, __LN) sserialize::assert_smaller_or_equal(__V1, __V2, "In File " __FILE__ " in Line " __LN ":" #__V1 " <= " #__V2);

#define SSERIALIZE_ASSERT(__BOOL) SSERIALIZE_ASSERT_WITH_LINE(__BOOL, SSA_MY_STR(__LINE__))
#define SSERIALIZE_ASSERT_MESSAGE(__BOOL, __MSG) SSERIALIZE_ASSERT_MESSAGE_WITH_LINE(__BOOL, __MSG, SSA_MY_STR(__LINE__))
#define SSERIALIZE_ASSERT_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_EQUAL_WITH_LINE(__V1, __V2, SSA_MY_STR(__LINE__))
#define SSERIALIZE_ASSERT_NOT_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_NOT_EQUAL_WITH_LINE(__V1, __V2, SSA_MY_STR(__LINE__))
#define SSERIALIZE_ASSERT_LARGER(__V1, __V2) SSERIALIZE_ASSERT_LARGER_WITH_LINE(__V1, __V2, SSA_MY_STR(__LINE__))
#define SSERIALIZE_ASSERT_LARGER_OR_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_LARGER_OR_EQUAL_WITH_LINE(__V1, __V2, SSA_MY_STR(__LINE__))
#define SSERIALIZE_ASSERT_SMALLER(__V1, __V2) SSERIALIZE_ASSERT_SMALLER_WITH_LINE(__V1, __V2, SSA_MY_STR(__LINE__))
#define SSERIALIZE_ASSERT_SMALLER_OR_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_SMALLER_OR_EQUAL_WITH_LINE(__V1, __V2, SSA_MY_STR(__LINE__))

#define SSA_MY_STRH(x) #x
#define SSA_MY_STR(x) SSA_MY_STRH(x)
#define SSA_MY_STR_LINE SSA_MY_STR(__LINE__)

#ifdef WITH_SSERIALIZE_CHEAP_ASSERT
	#define SSERIALIZE_CHEAP_ASSERT_ENABLED
	#define SSERIALIZE_CHEAP_ASSERT(__BOOL) SSERIALIZE_ASSERT(__BOOL)
	#define SSERIALIZE_CHEAP_ASSERT_MESSAGE(__BOOL, __MSG) SSERIALIZE_ASSERT_MESSAGE(__BOOL, __MSG)
	#define SSERIALIZE_CHEAP_ASSERT_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_EQUAL(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_NOT_EQUAL(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_LARGER(__V1, __V2) SSERIALIZE_ASSERT_LARGER(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_LARGER_OR_EQUAL(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_SMALLER(__V1, __V2) SSERIALIZE_ASSERT_SMALLER(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_SMALLER_OR_EQUAL(__V1, __V2)
#else
	#define SSERIALIZE_CHEAP_ASSERT(__BOOL)
	#define SSERIALIZE_CHEAP_ASSERT_MESSAGE(__BOOL, __MSG)
	#define SSERIALIZE_CHEAP_ASSERT_EQUAL(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_LARGER(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_SMALLER(__V1, __V2)
	#define SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(__V1, __V2)
#endif
	
//normal asserts
#ifdef WITH_SSERIALIZE_NORMAL_ASSERT
	#define SSERIALIZE_NORMAL_ASSERT_ENABLED
	#define SSERIALIZE_NORMAL_ASSERT(__BOOL) SSERIALIZE_ASSERT(__BOOL)
	#define SSERIALIZE_NORMAL_ASSERT_MESSAGE(__BOOL, __MSG) SSERIALIZE_ASSERT_MESSAGE(__BOOL, __MSG)
	#define SSERIALIZE_NORMAL_ASSERT_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_EQUAL(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_NOT_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_NOT_EQUAL(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_LARGER(__V1, __V2) SSERIALIZE_ASSERT_LARGER(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_LARGER_OR_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_LARGER_OR_EQUAL(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_SMALLER(__V1, __V2) SSERIALIZE_ASSERT_SMALLER(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_SMALLER_OR_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_SMALLER_OR_EQUAL(__V1, __V2)
#else
	#define SSERIALIZE_NORMAL_ASSERT(__BOOL)
	#define SSERIALIZE_NORMAL_ASSERT_MESSAGE(__BOOL, __MSG)
	#define SSERIALIZE_NORMAL_ASSERT_EQUAL(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_NOT_EQUAL(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_LARGER(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_LARGER_OR_EQUAL(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_SMALLER(__V1, __V2)
	#define SSERIALIZE_NORMAL_ASSERT_SMALLER_OR_EQUAL(__V1, __V2)
#endif

//expensive asserts
#ifdef WITH_SSERIALIZE_EXPENSIVE_ASSERT
	#define SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	#define SSERIALIZE_EXPENSIVE_ASSERT(__BOOL) SSERIALIZE_ASSERT(__BOOL)
	#define SSERIALIZE_EXPENSIVE_ASSERT_MESSAGE(__BOOL, __MSG) SSERIALIZE_ASSERT_MESSAGE(__BOOL, __MSG)
	#define SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_EQUAL(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_NOT_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_NOT_EQUAL(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_LARGER(__V1, __V2) SSERIALIZE_ASSERT_LARGER(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_LARGER_OR_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_LARGER_OR_EQUAL(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_SMALLER(__V1, __V2) SSERIALIZE_ASSERT_SMALLER(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_SMALLER_OR_EQUAL(__V1, __V2) SSERIALIZE_ASSERT_SMALLER_OR_EQUAL(__V1, __V2)
#else
	#define SSERIALIZE_EXPENSIVE_ASSERT(__BOOL)
	#define SSERIALIZE_EXPENSIVE_ASSERT_MESSAGE(__BOOL, __MSG)
	#define SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_NOT_EQUAL(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_LARGER(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_LARGER_OR_EQUAL(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_SMALLER(__V1, __V2)
	#define SSERIALIZE_EXPENSIVE_ASSERT_SMALLER_OR_EQUAL(__V1, __V2)
#endif

#endif