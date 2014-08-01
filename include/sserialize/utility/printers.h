#ifndef SSERIALIZE_UTIL_PRINTERS_H
#define SSERIALIZE_UTIL_PRINTERS_H
#include <ostream>
#include <vector>

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

}

#endif