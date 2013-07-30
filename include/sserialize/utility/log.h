#ifndef OSMFIND_LOG_H
#define OSMFIND_LOG_H
#include <sstream>
#include <vector>
#ifdef __ANDROID__
#include <android/log.h>
#else
#include <iostream>
#endif

namespace osmfindlog {

class Log {
public:
	enum CmdTypes {
		endl, flush
	};
	enum LogLevel {
		INFO, DEBUG, ERROR
	};
private:
	LogLevel m_defLogLevel;
	std::stringstream m_sbuf;
private:
#ifdef __ANDROID__
	unsigned int m_androidLogLevel;
#endif
public:
	Log();
	Log(LogLevel defLevel);
	~Log();
	Log& operator()(const std::string & logTag, const std::string & msg); 
	void sbufCmd(CmdTypes t);
	std::stringstream& sbuf() { return m_sbuf;}
};

extern Log info;
extern Log debug;
extern Log err;

template<typename T>
std::string nameOfType();

#define __NAME_OF_TYPE_SPECIALICATION(__TYPE) template<> inline std::string nameOfType<__TYPE>() { return std::string("__TYPE"); }

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

inline void toString(std::stringstream & ss) {}

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
void putInto(std::ostream & out, const std::vector<T> & vec) {
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

}//end namespace

osmfindlog::Log& operator<<(osmfindlog::Log& log, const std::string & msg);
osmfindlog::Log& operator<<(osmfindlog::Log& log, int num);
osmfindlog::Log& operator<<(osmfindlog::Log& log, unsigned int num);
osmfindlog::Log& operator<<(osmfindlog::Log& log, double num);
osmfindlog::Log& operator<<(osmfindlog::Log& log, bool value);
osmfindlog::Log& operator<<(osmfindlog::Log& log, osmfindlog::Log::CmdTypes cmdt);

#endif