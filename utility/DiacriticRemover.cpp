#include <sserialize/utility/DiacriticRemover.h>

namespace sserialize {

UErrorCode DiacriticRemover::init() {
	UErrorCode status = U_ZERO_ERROR;
	m_transLiterator = icu::Transliterator::createInstance("NFD; [:Nonspacing Mark:] Remove; NFC;", UTRANS_FORWARD, status);
	return status;
}

void DiacriticRemover::transliterate(std::string & str) {
	UnicodeString ustr( UnicodeString::fromUTF8(str));
	m_transLiterator->transliterate(ustr);
	str = std::string();
	StringByteSink<std::string> retStr(&str);
	ustr.toUTF8(retStr);
}

}//end namespace