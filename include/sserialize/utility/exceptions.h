#ifndef SSERIALIZE_EXCPETIONS_H
#define SSERIALIZE_EXCPETIONS_H
#include <exception>
#include <string>
#include <sstream>

namespace sserialize {

class Exception: public std::exception {
private:
	std::string m_msg;
protected:
	void setMsg(const std::string &  msg) { m_msg = msg; }
public:
	Exception() {}
	Exception(const std::string & msg) : m_msg(msg) {}
	virtual ~Exception() throw() {}
	virtual const char* what() const throw() { return m_msg.c_str(); }
};

class VersionMissMatchException: public Exception {
	uint32_t m_wantVersion;
	uint32_t m_haveVersion;
public:
	VersionMissMatchException(const std::string & what, uint32_t want, uint32_t have) :
	Exception(what), m_wantVersion(want), m_haveVersion(have) {
		std::stringstream ss;
		ss << what << "; want version " << want << " but have version " << have;
		setMsg(ss.str());
	}
	uint32_t wantVersion() const { return m_wantVersion; }
	uint32_t haveVersion() const { return m_haveVersion; }
};

class OutOfBoundsException: public Exception {
public:
	OutOfBoundsException(const std::string & what) :
	Exception()
	{
		setMsg("OutOfBoundsException: " + what);
	}
};

class TypeMissMatchException: public Exception {
public:
	TypeMissMatchException(const std::string & what) :
	Exception()
	{
		setMsg("TypeMissMatchException: " + what);
	}
};

class CorruptDataException: public Exception {
public:
	CorruptDataException(const std::string & what) :
	Exception()
	{
		setMsg("CorruptDataException: " + what);
	}
};

class InvalidReferenceException: public Exception {
	InvalidReferenceException(const std::string & what) :
	Exception()
	{
		setMsg("InvalidReferenceException: " + what);
	}
};

class CreationException: public Exception {
public:
	CreationException(const std::string & what) :
	Exception()
	{
		setMsg("CreationException: " + what);
	}
};

class UnimplementedFunctionException: public Exception {
public:
	UnimplementedFunctionException(const std::string & what) :
	Exception()
	{
		setMsg("UnimplementedFunctionException: " + what);
	}
};

}//end namespace

#define SSERIALIZE_VERSION_MISSMATCH_CHECK(__WANTVERSION, __HAVEVERSION, __MSG) do { if (__WANTVERSION != __HAVEVERSION) throw sserialize::VersionMissMatchException(__MSG, __WANTVERSION, __HAVEVERSION); } while(0);

#define SSERIALIZE_LENGTH_CHECK(__NEEDLENGTH, __HAVELENGTH, __MSG) do { if (__NEEDLENGTH > __HAVELENGTH) throw sserialize::CorruptDataException(__MSG); } while(0);
#define SSERIALIZE_EQUAL_LENGTH_CHECK(__NEEDLENGTH, __HAVELENGTH, __MSG) do { if (__NEEDLENGTH != __HAVELENGTH) throw sserialize::CorruptDataException(__MSG); } while(0);

#endif