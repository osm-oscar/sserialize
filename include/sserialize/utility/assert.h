#ifndef SSERIALIZE_ASSERT_H
#define SSERIALIZE_ASSERT_H
#include <sserialize/utility/constants.h>
#include <iostream>
#include <sstream>

namespace sserialize {

void __assert_function(const std::string & message) {
	while (true) {
		std::cout << message << std::endl;
	}
}

void assert_true(bool value) {
	if (UNLIKELY_BRANCH(!value)) {
		__assert_function("ASSERTION FAILED");
	}
}

template<typename T1, typename T2>
void assert_equal(const T1 & v1, const T2 & v2) {
	if (UNLIKELY_BRANCH(v1 != v2)) {
		std::stringstream ss;
		ss << "ASSERTION FAILED! SHOULD=" << v1 << ", IS=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_equal_message(const T1 & v1, const T2 & v2) {
	if (UNLIKELY_BRANCH(!(v1 == v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED! SHOULD=" << v1 << ", IS=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_larger(const T1 & v1, const T2 & v2) {
	if (UNLIKELY_BRANCH(!(v1 > v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED! SHOULD=" << v1 << ", IS=" << v2;
		__assert_function(ss.str());
	}
}

template<typename T1, typename T2>
void assert_larger_or_equal(const T1 & v1, const T2 & v2) {
	if (UNLIKELY_BRANCH(!(v1 >= v2))) {
		std::stringstream ss;
		ss << "ASSERTION FAILED! SHOULD=" << v1 << ", IS=" << v2;
		__assert_function(ss.str());
	}
}

}//end namspace

#define SSERIALIZE_CHEAP_ASSERT(__BOOL) sserialize::assert_true(__BOOL);
#define SSERIALIZE_CHEAP_ASSERT_EQUAL(__V1, __V2) sserialize::assert_equal(__V1, __V2);
#define SSERIALIZE_CHEAP_ASSERT_LARGER(__V1, __V2) sserialize::assert_larger(__V1, __V2);
#define SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(__V1, __V2) sserialize::assert_larger_or_equal(__V1, __V2);
#define SSERIALIZE_CHEAP_ASSERT_EQUAL_MESSAGE(__V1, __V2) sserialize::assert_equal(__V1, __V2);

#endif