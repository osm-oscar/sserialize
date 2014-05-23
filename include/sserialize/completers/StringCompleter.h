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
#ifndef SSERIALIZE_STRING_COMPLETER_DEFAULT_CACHE_SIZE
#define SSERIALIZE_STRING_COMPLETER_DEFAULT_CACHE_SIZE 8
#endif


//TODO:Implement getSizeInBytes by fixing dependency on single file
namespace sserialize {


class StringCompleterPrivate;

namespace detail {
namespace types {
namespace StringCompleterPrivate {
	
	class ForwardIterator: public RefCountObject {
	public:
		virtual ~ForwardIterator() {}
		virtual std::set<uint32_t> getNext() const = 0;
		virtual bool hasNext(uint32_t codepoint) const = 0;
		virtual bool next(uint32_t codepoint) = 0;
		virtual ForwardIterator * copy() const = 0;
	};
	
	class EmptyForwardIterator: public ForwardIterator {
	public:
		virtual ~EmptyForwardIterator();
		virtual std::set<uint32_t> getNext() const;
		virtual bool hasNext(uint32_t codepoint) const;
		virtual bool next(uint32_t codepoint);
		virtual ForwardIterator * copy() const;
	};
	
}
}
}

class StringCompleter: public RCWrapper<StringCompleterPrivate> {
public:
	///QuerryType, can be used in conjunction with SupportedQuerries (they are the same)
	enum QuerryType {
		QT_NONE=0, QT_EXACT=1, QT_PREFIX=2, QT_SUFFIX=4, QT_SUBSTRING=8, QT_EPSS=15, QT_CASE_INSENSITIVE=16, QT_CASE_SENSTIVE=32
	};
	
	enum SupportedQuerries {
		SQ_NONE=0,
		SQ_EXACT=1, SQ_PREFIX=2, SQ_SUFFIX=4, SQ_SUBSTRING=8,
		SQ_EP=3, SQ_SSP=12, SQ_EPSP=15,
		SQ_CASE_INSENSITIVE=16, SQ_CASE_SENSITIVE=32,
		SQ_ALL_INSENSITIVE=31, SQ_ALL_SENSITIVE=47, SQ_ALL=63
	};
	
	class ForwardIterator: public RCWrapper<detail::types::StringCompleterPrivate::ForwardIterator> {
		typedef RCWrapper<detail::types::StringCompleterPrivate::ForwardIterator> MyBaseClass;
	public:
		ForwardIterator();
		ForwardIterator(detail::types::StringCompleterPrivate::ForwardIterator * data);
		ForwardIterator(const ForwardIterator & other);
		~ForwardIterator();
		ForwardIterator & operator=(const ForwardIterator & other);
		std::set<uint32_t> getNext() const;
		bool hasNext(uint32_t codepoint) const;
		bool next(uint32_t codepoint);
	};
	
private:
#ifdef SSERIALIZE_STRING_COMPLETER_WITH_CACHE
	sserialize::LFUCache< std::pair<std::string, uint16_t>, ItemIndex> m_cache;
#endif
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
	void setCacheSize(uint32_t s);
	
	ItemIndex complete(const std::string & str, QuerryType qtype);
	ItemIndexIterator partialComplete(const std::string & str, QuerryType qtype);
	
	ForwardIterator forwardIterator() const;
	
	/** @return returns pairs of char->ItemIndex **/
	std::map<uint16_t, ItemIndex> getNextCharacters(const std::string& str, QuerryType qtype, bool withIndex) const;
	ItemIndex indexFromId(uint32_t idxId) const;
	
    StringCompleterPrivate * getPrivate() const;

	std::ostream& printStats(std::ostream& out) const;
	
	std::string getName() const;
	
	static QuerryType normalize(std::string & qstr);
};
}//end namespace

#include <sserialize/completers/StringCompleterPrivate.h>

#endif