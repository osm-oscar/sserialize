#include <sserialize/utility/exceptions.h>
#include <sstream>

namespace sserialize {


void Exception::setMsg(const std::string &  msg) {
	m_msg = msg;
}

Exception::Exception() {}

Exception::Exception(const std::string & msg) :
m_msg(msg)
{}

Exception::~Exception() throw() 
{}

const char* Exception::what() const throw() {
	return m_msg.c_str();
}

void Exception::appendMessage(const std::string & str) {
	m_msg.reserve(m_msg.size()+1+str.size());
	m_msg += "\n";
	m_msg += str;
}

InvalidAlgorithmStateException::InvalidAlgorithmStateException(const std::string& what) :
Exception("InvalidAlgorithmStateException: " + what)
{}

BugException::BugException(const std::string & what) :
Exception("BugException: " + what)
{}

VersionMissMatchException::VersionMissMatchException(const std::string & what, uint32_t want, uint32_t have) :
Exception(what),
m_wantVersion(want),
m_haveVersion(have)
{
	std::stringstream ss;
	ss << "VersionMissMatchException (want=" << want << ", have=" << have << "): " << what;
	setMsg(ss.str());
}
uint32_t VersionMissMatchException::wantVersion() const throw() {
	return m_wantVersion;
}

uint32_t VersionMissMatchException::haveVersion() const throw() {
	return m_haveVersion;
}

OutOfBoundsException::OutOfBoundsException() :
Exception()
{
	setMsg("OutOfBoundsException");
}

OutOfBoundsException::OutOfBoundsException(const std::string & what) :
Exception()
{
	setMsg("OutOfBoundsException: " + what);
}

IOException::IOException() :
Exception()
{
	setMsg("IOException");
}

IOException::IOException(const std::string & what) :
Exception()
{
	setMsg("IOException: " + what);
}

TypeMissMatchException::TypeMissMatchException(const std::string & what) :
Exception()
{
	setMsg("TypeMissMatchException: " + what);
}

CorruptDataException::CorruptDataException(const std::string & what) :
Exception()
{
	setMsg("CorruptDataException: " + what);
}

MissingDataException::MissingDataException(const std::string & what) :
Exception()
{
	setMsg("MissingDataException: " + what);
}

InvalidReferenceException::InvalidReferenceException(const std::string & what) :
Exception()
{
	setMsg("InvalidReferenceException: " + what);
}

CreationException::CreationException(const std::string & what) :
Exception()
{
	setMsg("CreationException: " + what);
}

UnimplementedFunctionException::UnimplementedFunctionException(const std::string & what) :
Exception()
{
	setMsg("UnimplementedFunctionException: " + what);
}

UnsupportedFeatureException::UnsupportedFeatureException(const std::string & what) :
Exception()
{
	setMsg("UnsupportedFeatureException: " + what);
}

ConfigurationException::ConfigurationException(const std::string & where, const std::string & what) :
Exception("ConfigurationException in " + where + ": " + what)
{}

TypeOverflowException::TypeOverflowException(const std::string & what) :
Exception("TypeOverflowException:" + what)
{}

InvalidEnumValueException::InvalidEnumValueException(const std::string & what) :
Exception()
{
	setMsg("InvalidEnumValueException: " + what);
}

ConversionException::ConversionException(const std::string & what) :
Exception()
{
	setMsg("ConversionException: " + what);
}

MathException::MathException(const std::string & what) :
Exception()
{
	setMsg("MathException: " + what);
}

PreconditionViolationException::PreconditionViolationException(const std::string & what) :
Exception()
{
	setMsg("PreconditionViolationException: " + what);
}


} //end namespace sserialize
