#include <sserialize/strings/DiacriticRemover.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {

DiacriticRemover::DiacriticRemover(const DiacriticRemover& other) {
	m_transLiterator = other.m_transLiterator->clone();
	if (!m_transLiterator) {
		throw sserialize::CreationException("Could not create a copy of DiacriticRemover");
	}
}

UErrorCode DiacriticRemover::init() {
	UErrorCode status = U_ZERO_ERROR;
	m_transLiterator = icu::Transliterator::createInstance("NFD; [:Nonspacing Mark:] Remove; NFC;", UTRANS_FORWARD, status);
	return status;
}

void DiacriticRemover::transliterate(std::string& str) const {
	icu::UnicodeString ustr( icu::UnicodeString::fromUTF8(str));
	m_transLiterator->transliterate(ustr);
	str.clear();
	icu::StringByteSink<std::string> retStr(&str);
	ustr.toUTF8(retStr);
}

std::string DiacriticRemover::operator()(const std::string& str) const {
	icu::UnicodeString ustr( icu::UnicodeString::fromUTF8(str) );
	m_transLiterator->transliterate(ustr);
	std::string ret;
	icu::StringByteSink<std::string> retStr(&ret);
	ustr.toUTF8(retStr);
	return ret;
}


}//end namespace
