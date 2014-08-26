#ifndef SSERIALIZE_UTIL_PRINTERS_H
#define SSERIALIZE_UTIL_PRINTERS_H
#include <ostream>
#include <vector>
#include <set>
#include <deque>
#include <sstream>

template<typename T1, typename T2>
std::ostream & operator<<(std::ostream & out, const std::pair<T1, T2> & s) {
	return out << "(" << s.first << ", " << s.second << ")";
}

template<typename T>
std::ostream & operator<<(std::ostream & out, const std::vector<T> & s) {
	if (!s.size())
		return out << "std::vector[]";
	typename std::vector<T>::const_iterator end( s.cend());
	typename std::vector<T>::const_iterator it = s.cbegin();
	
	out << "std::vector[";
	while (true) {
		out << *it;
		++it;
		if (it != end)
			out << ", ";
		else
			return out << "]";
	}
}

template<typename T>
std::ostream & operator<<(std::ostream & out, const std::set<T> & s) {
	if (!s.size())
		return out << "std::set<0>[]";
	typename std::set<T>::const_iterator end( s.cend());
	typename std::set<T>::const_iterator it = s.cbegin();
	
	out << "std::set<" << s.size() << ">[";
	while (true) {
		out << *it;
		++it;
		if (it != end)
			out << ", ";
		else
			return out << "]";
	}
}

template<typename T>
std::ostream & operator<<(std::ostream & out, const std::deque<T> & s) {
	if (!s.size())
		return out << "std::deque[]";
	typename std::deque<T>::const_iterator end( s.cend());
	typename std::deque<T>::const_iterator it = s.cbegin();
	
	out << "std::deque[";
	while (true) {
		out << *it;
		++it;
		if (it != end)
			out << ", ";
		else
			return out << "]";
	}
}

namespace sserialize {

template<char SEPARATOR, typename T_IT>
std::ostream & print(std::ostream & out, T_IT begin, T_IT end) {
	if (begin != end) {
		out << *begin;
		++begin;
		for(; begin != end; ++begin) {
			out << SEPARATOR << *begin;
		}
	}
	return out;
}

template<typename T>
std::string nameOfType();

#define __NAME_OF_TYPE_SPECIALICATION(__TYPE) template<> inline std::string nameOfType<__TYPE>() { return std::string(#__TYPE); }

__NAME_OF_TYPE_SPECIALICATION(uint8_t);
__NAME_OF_TYPE_SPECIALICATION(uint16_t);
__NAME_OF_TYPE_SPECIALICATION(uint32_t);
__NAME_OF_TYPE_SPECIALICATION(uint64_t);
__NAME_OF_TYPE_SPECIALICATION(int8_t);
__NAME_OF_TYPE_SPECIALICATION(int16_t);
__NAME_OF_TYPE_SPECIALICATION(int32_t);
__NAME_OF_TYPE_SPECIALICATION(int64_t);
__NAME_OF_TYPE_SPECIALICATION(float);
__NAME_OF_TYPE_SPECIALICATION(double);
__NAME_OF_TYPE_SPECIALICATION(std::string);
#undef __NAME_OF_TYPE_SPECIALICATION

inline void toString(std::stringstream & /*ss*/) {}

std::string toString(bool value);

template<typename PrintType>
void toString(std::stringstream & ss, PrintType t) {
	ss << t;
}

template<typename PrintType, typename ... PrintTypeList>
void toString(std::stringstream & ss, PrintType t, PrintTypeList ... args) {
	ss << t;
	toString(ss, args...);
}

template<typename ... PrintTypeList>
std::string toString(PrintTypeList ... args) {
	std::stringstream ss;
	toString(ss, args...);
	return ss.str();
}

template<typename T>
void print(std::ostream & out, const std::vector<T> & vec) {
	out << "std::vector<" << nameOfType<T>() << ">[";
	typename std::vector<T>::const_iterator it(vec.begin());
	typename std::vector<T>::const_iterator end(vec.end());
	if (vec.size()) {
		--end;
		for(; it < end; ++it) {
			out << *it << ", ";
		}
		out << *it;
	}
	out << "]";
}

inline std::string prettyFormatSize(uint64_t bytes) {
	char prefixes[] = {'N', 'K', 'M', 'G'};
	std::stringstream ss;
	for(int i = 3; i >= 0; --i) {
		uint8_t shift = 10*i;
		uint64_t tmp = bytes >> shift;
		if (tmp) {
			ss << tmp << prefixes[i] << "iB ";
			bytes = bytes & ~(tmp << shift);
		}
	}
	return ss.str();
}


}

#endif