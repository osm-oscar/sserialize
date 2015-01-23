#ifndef SSERIALIZE_DIACRITIC_REMOVER_H
#define SSERIALIZE_DIACRITIC_REMOVER_H
#ifndef __ANDROID__
#include <unicode/translit.h>
#endif

namespace sserialize {

#ifndef __ANDROID__
class DiacriticRemover {
public:
	typedef UErrorCode ErrorCodeType;
private:
	icu::Transliterator * m_transLiterator;
public:
	DiacriticRemover() : m_transLiterator(0) {}
	virtual ~DiacriticRemover() { delete m_transLiterator; }
	UErrorCode init();

	void transliterate(std::string & str) const;
	static bool isFailure(ErrorCodeType error) { return U_FAILURE(error); }
	static std::string errorName(ErrorCodeType error) { return std::string(u_errorName(error)); }

};
#else
class DiacriticRemover {
public:
	typedef bool ErrorCodeType;
public:
	DiacriticRemover() {}
	~DiacriticRemover() {}
	bool init() { return false; }
	void transliterate(std::string & /*str*/) const {}
	static bool isFailure(ErrorCodeType /*error*/) { return true; }
	static std::string errorName(ErrorCodeType /*error*/) { return "Unsupported under android"; }
};
#endif

}//end namespace

#endif