#ifndef OSMFIND_LOG_H
#define OSMFIND_LOG_H
#include <sstream>
#include <vector>
#ifdef __ANDROID__
#include <android/log.h>
#else
#include <iostream>
#endif

namespace sserialize {

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

inline std::string prettyFormatSize(uint64_t bytes) {
	if ( bytes >> 30 ) {
		return toString(bytes >> 30, " Gibibytes");
	}
	if ( bytes >> 20 ) {
		return toString(bytes >> 20, " Mebibytes");
	}
	if ( bytes >> 10 ) {
		return toString(bytes >> 10, " Kibibytes");
	}
	return toString(bytes, " Bytes");
}

}//end namespace

sserialize::Log& operator<<(sserialize::Log& log, const std::string & msg);
sserialize::Log& operator<<(sserialize::Log& log, int num);
sserialize::Log& operator<<(sserialize::Log& log, unsigned int num);
sserialize::Log& operator<<(sserialize::Log& log, double num);
sserialize::Log& operator<<(sserialize::Log& log, bool value);
sserialize::Log& operator<<(sserialize::Log& log, sserialize::Log::CmdTypes cmdt);

#endif