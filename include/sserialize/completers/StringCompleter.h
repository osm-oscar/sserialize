#ifndef SSERIALIZE_STRING_COMPLETER_H
#define SSERIALIZE_STRING_COMPLETER_H
#include <map>
#include <sserialize/utility/refcounting.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexIterator.h>
#include <sserialize/templated/LFUCache.h>
#ifdef SSERIALIZE_WITH_THREADS
#include <mutex>
#endif

namespace sserialize {


class StringCompleterPrivate;

class StringCompleter: public RCWrapper<StringCompleterPrivate> {
public:
	enum QuerryType {
		QT_NONE=0, QT_EXACT=1, QT_PREFIX=2, QT_SUFFIX=4, QT_SUFFIX_PREFIX=8, QT_CASE_INSENSITIVE=16, QT_CASE_SENSTIVE=32
	};
	
	enum SupportedQuerries {
		SQ_NONE=0, SQ_EXACT=1, SQ_PREFIX=2, SQ_EP=3, SQ_SUFFIX=4, SQ_SUFFIX_PREFIX=8, SQ_SSP=12, SQ_EPSP=15, SQ_CASE_INSENSITIVE=16, SQ_CASE_SENSITIVE=32, SQ_ALL=63
	};
private:
	sserialize::LFUCache< std::pair<std::string, uint16_t>, ItemIndex> m_cache;
#ifdef SSERIALIZE_WITH_THREADS
	std::mutex m_cacheLock;
#endif
public:
	StringCompleter();
	StringCompleter(const StringCompleter & other);
	StringCompleter(StringCompleterPrivate * priv);
	StringCompleter & operator=(const StringCompleter & strc);
	SupportedQuerries getSupportedQuerries();
	bool supportsQuerry(QuerryType qt);

	void clearCache();
	
	ItemIndex complete(const std::string & str, QuerryType qtype);
	ItemIndexIterator partialComplete(const std::string & str, QuerryType qtype);
	/** @return returns pairs of char->ItemIndex **/
	std::map<uint16_t, ItemIndex> getNextCharacters(const std::string& str, QuerryType qtype, bool withIndex) const;
	ItemIndex indexFromId(uint32_t idxId) const;
	
    StringCompleterPrivate * getPrivate() const;

	std::ostream& printStats(std::ostream& out) const;
	
	std::string getName() const;
};
}//end namespace

#include <sserialize/completers/StringCompleterPrivate.h>

#endif