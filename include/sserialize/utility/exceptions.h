#ifndef SSERIALIZE_EXCPETIONS_H
#define SSERIALIZE_EXCPETIONS_H
#include <exception>
#include <string>

namespace sserialize {

class Exception: public std::exception {
private:
	std::string m_msg;
protected:
	void setMsg(const std::string &  msg);
public:
	Exception();
	Exception(const std::string & msg);
	virtual ~Exception() throw();
	virtual const char* what() const throw();
	void appendMessage(const std::string & str);
};

class BugException: public Exception {
public:
	BugException(const std::string & what);
};

class InvalidAlgorithmStateException: public Exception {
public:
	InvalidAlgorithmStateException(const std::string & what);
};

class VersionMissMatchException: public Exception {
	uint32_t m_wantVersion;
	uint32_t m_haveVersion;
public:
	VersionMissMatchException(const std::string & what, uint32_t want, uint32_t have);
	uint32_t wantVersion() const throw() ;
	uint32_t haveVersion() const throw() ;
};

class OutOfBoundsException: public Exception {
public:
	OutOfBoundsException();
	OutOfBoundsException(const std::string & what);
};

class IOException: public Exception {
public:
	IOException();
	IOException(const std::string & what);
};

class TypeMissMatchException: public Exception {
public:
	TypeMissMatchException(const std::string & what);
};

class CorruptDataException: public Exception {
public:
	CorruptDataException(const std::string & what);
};

class MissingDataException: public Exception {
public:
	MissingDataException(const std::string & what);
};

class InvalidReferenceException: public Exception {
public:
	InvalidReferenceException(const std::string & what);
};

class CreationException: public Exception {
public:
	CreationException(const std::string & what);
};

class UnimplementedFunctionException: public Exception {
public:
	UnimplementedFunctionException(const std::string & what);
};

class UnsupportedFeatureException: public Exception {
public:
	UnsupportedFeatureException(const std::string & what);
};

class ConfigurationException: public Exception {
public:
	ConfigurationException(const std::string & where, const std::string & what);
};

class TypeOverflowException: public Exception {
public:
	TypeOverflowException(const std::string & what);
};

class InvalidEnumValueException: public Exception {
public:
	InvalidEnumValueException(const std::string & what);
};

class ConversionException: public Exception {
public:
	ConversionException(const std::string & what);
};

class MathException: public Exception {
public:
	MathException(const std::string & what);
};

class PreconditionViolationException: public Exception {
public:
	PreconditionViolationException(const std::string & what);
};

class AllocationException: public Exception {
public:
	AllocationException(std::string const & what);
};

}//end namespace

#define SSERIALIZE_VERSION_MISSMATCH_CHECK(__WANTVERSION, __HAVEVERSION, __MSG) do { if (__WANTVERSION != __HAVEVERSION) throw sserialize::VersionMissMatchException(__MSG, __WANTVERSION, __HAVEVERSION); } while(0);
#define SSERIALIZE_LENGTH_CHECK(__NEEDLENGTH, __HAVELENGTH, __MSG) do { if (__NEEDLENGTH > __HAVELENGTH) throw sserialize::CorruptDataException(__MSG); } while(0);
#define SSERIALIZE_EQUAL_LENGTH_CHECK(__NEEDLENGTH, __HAVELENGTH, __MSG) do { if (__NEEDLENGTH != __HAVELENGTH) throw sserialize::CorruptDataException("Length missmatch. Need= " + std::to_string(__NEEDLENGTH) + ", have=" + std::to_string(__HAVELENGTH) + ": " + __MSG); } while(0);
#define SSERIALIZE_ASSERT_EQUAL_CREATION(__NEEDLENGTH, __HAVELENGTH, __MSG) do { if (__NEEDLENGTH != __HAVELENGTH) throw sserialize::CreationException(__MSG); } while(0);


#endif
