#ifndef SSERIALIZE_DEBUGGER_FUNCTIONS_H
#define SSERIALIZE_DEBUGGER_FUNCTIONS_H
#include <fstream>

namespace sserialize {
	inline bool breakHere(bool stop) {
		if (stop)
			return true;
		else
			return false;
	}
	
	
	template<typename T_CONTAINER>
	bool dumpToFile(const char * fileName, const T_CONTAINER & c) {
		typename T_CONTAINER::const_iterator it( c.begin() );
		typename T_CONTAINER::const_iterator end( c.end() );
		std::ofstream out;
		out.open(fileName);
		if (!out.is_open())
			return false;
		while (it != end) {
			out << *it << std::endl;
			it++;
		}
		out.close();
		return true;
	}
	
	template<typename T_CONTAINER>
	bool dumpToFile(const std::string & fileName, const T_CONTAINER & c) {
		return dumpToFile(fileName.c_str(), c);
	}
}

#endif