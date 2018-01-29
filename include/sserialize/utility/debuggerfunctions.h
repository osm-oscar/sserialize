#ifndef SSERIALIZE_DEBUGGER_FUNCTIONS_H
#define SSERIALIZE_DEBUGGER_FUNCTIONS_H
#include <fstream>

namespace sserialize {

	bool breakHere();
	bool breakHereIf(bool stop);
	
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