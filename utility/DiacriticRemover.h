#ifndef SSERIALIZE_DIACRITIC_REMOVER_H
#define SSERIALIZE_DIACRITIC_REMOVER_H
#include <unicode/translit.h>

namespace sserialize {

class DiacriticRemover {
	icu::Transliterator * m_transLiterator;
public:
	DiacriticRemover() : m_transLiterator(0) {}
	virtual ~DiacriticRemover() { delete m_transLiterator; }
	UErrorCode init();

	void transliterate(std::string & str);

};

}//end namespace

#endif