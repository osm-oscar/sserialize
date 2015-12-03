#include <sserialize/strings/DiacriticRemover.h>

namespace sserialize {

UErrorCode DiacriticRemover::init() {
	UErrorCode status = U_ZERO_ERROR;
	m_transLiterator = icu::Transliterator::createInstance("NFD; [:Nonspacing Mark:] Remove; NFC;", UTRANS_FORWARD, status);
	return status;
}

void DiacriticRemover::transliterate(std::string& str) const {
	UnicodeString ustr( UnicodeString::fromUTF8(str));
	m_transLiterator->transliterate(ustr);
	str.clear();
	StringByteSink<std::string> retStr(&str);
	ustr.toUTF8(retStr);
}

std::string DiacriticRemover::operator()(const std::string& str) const {
	UnicodeString ustr( UnicodeString::fromUTF8(str) );
	m_transLiterator->transliterate(ustr);
	std::string ret;
	StringByteSink<std::string> retStr(&ret);
	ustr.toUTF8(retStr);
	return ret;
}


}//end namespace