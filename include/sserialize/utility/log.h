#ifndef OSMFIND_LOG_H
#define OSMFIND_LOG_H
#include <sstream>
#include <vector>
#include <sserialize/utility/printers.h>
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

}//end namespace

sserialize::Log& operator<<(sserialize::Log& log, const std::string & msg);
sserialize::Log& operator<<(sserialize::Log& log, int num);
sserialize::Log& operator<<(sserialize::Log& log, unsigned int num);
sserialize::Log& operator<<(sserialize::Log& log, double num);
sserialize::Log& operator<<(sserialize::Log& log, bool value);
sserialize::Log& operator<<(sserialize::Log& log, sserialize::Log::CmdTypes cmdt);

#endif