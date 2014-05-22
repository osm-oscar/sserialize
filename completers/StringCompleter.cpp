#include <sserialize/completers/StringCompleter.h>
#include <sserialize/completers/StringCompleterPrivate.h>
#ifdef SSERIALIZE_WITH_THREADS
#include <sserialize/utility/MutexLocker.h>
#endif



namespace sserialize {

namespace detail {
namespace types {
namespace StringCompleterPrivate {

EmptyForwardIterator::~EmptyForwardIterator() {}

std::set<uint32_t> EmptyForwardIterator::EmptyForwardIterator::getNext() const {
	return std::set<uint32_t>();
}

bool EmptyForwardIterator::hasNext(uint32_t /*codepoint*/) const {
	return false;
}

bool EmptyForwardIterator::next(uint32_t /*codepoint*/) {
	return false;
}

ForwardIterator * EmptyForwardIterator::copy() const {
	return new EmptyForwardIterator();
}

}}}
	
StringCompleter::ForwardIterator::ForwardIterator() {}

StringCompleter::ForwardIterator::ForwardIterator(detail::types::StringCompleterPrivate::ForwardIterator * data) : MyBaseClass(data) {}

StringCompleter::ForwardIterator::ForwardIterator(const ForwardIterator & other) : MyBaseClass(other) {}

StringCompleter::ForwardIterator::~ForwardIterator() {}

StringCompleter::ForwardIterator & StringCompleter::ForwardIterator::operator=(const ForwardIterator & other) {
	MyBaseClass::operator=(other);
	return *this;
}

std::set<uint32_t> StringCompleter::ForwardIterator::getNext() const {
	return priv()->getNext();
}

bool StringCompleter::ForwardIterator::hasNext(uint32_t codepoint) const {
	return priv()->hasNext(codepoint);
}

bool StringCompleter::ForwardIterator::next(uint32_t codepoint) {
	return priv()->next(codepoint);
}

StringCompleter::StringCompleter() :
RCWrapper<StringCompleterPrivate>(new StringCompleterPrivate())
{
#ifdef SSERIALIZE_STRING_COMPLETER_WITH_CACHE
	m_cache.setSize(SSERIALIZE_STRING_COMPLETER_DEFAULT_CACHE_SIZE);
#endif
}

StringCompleter::StringCompleter(const StringCompleter& other) :
RCWrapper<StringCompleterPrivate>(other)
{
#ifdef SSERIALIZE_STRING_COMPLETER_WITH_CACHE
	m_cache.setSize(SSERIALIZE_STRING_COMPLETER_DEFAULT_CACHE_SIZE);
#endif
}


StringCompleter::StringCompleter(StringCompleterPrivate * priv) :
RCWrapper< sserialize::StringCompleterPrivate >(priv)
{
#ifdef SSERIALIZE_STRING_COMPLETER_WITH_CACHE
	m_cache.setSize(SSERIALIZE_STRING_COMPLETER_DEFAULT_CACHE_SIZE);
#endif
}

StringCompleter& StringCompleter::operator=(const StringCompleter& strc) {
	RCWrapper< sserialize::StringCompleterPrivate >::operator=(strc);
#ifdef SSERIALIZE_STRING_COMPLETER_WITH_CACHE
	m_cache = strc.m_cache;
#endif
	return *this;
}

StringCompleter::SupportedQuerries StringCompleter::getSupportedQuerries() {
	return priv()->getSupportedQuerries();
}

bool StringCompleter::supportsQuerry(StringCompleter::QuerryType qt) {
	StringCompleter::SupportedQuerries sq = getSupportedQuerries();
	if (sq & SQ_CASE_SENSITIVE || qt & QT_CASE_INSENSITIVE ) {
		return (sq & qt) == qt;
	}
	else {
		return false;
	}
}

void StringCompleter::clearCache() {
#ifdef SSERIALIZE_STRING_COMPLETER_WITH_CACHE
#ifdef SSERIALIZE_WITH_THREADS
	MutexLocker locker(m_cacheLock);
#endif
	m_cache.clear();
#endif
}

#ifdef SSERIALIZE_STRING_COMPLETER_WITH_CACHE
void StringCompleter::setCacheSize(uint32_t s) {
#ifdef SSERIALIZE_WITH_THREADS
	MutexLocker locker(m_cacheLock);
#endif
	m_cache.clear();
	m_cache.setSize(s);
}
#else
void StringCompleter::setCacheSize(uint32_t) {}
#endif

ItemIndex StringCompleter::complete(const std::string & str, StringCompleter::QuerryType qtype) {
	std::pair<std::string, sserialize::StringCompleter::QuerryType> q(str, qtype);
#ifdef SSERIALIZE_STRING_COMPLETER_WITH_CACHE
#ifdef SSERIALIZE_WITH_THREADS
	MutexLocker locker(m_cacheLock);
#endif
	if (m_cache.contains(q)) {
		return m_cache.find(q);
	}
	else {
#ifdef SSERIALIZE_WITH_THREADS
		m_cacheLock.unlock();
#endif
		ItemIndex idx(priv()->complete(str, qtype));
#ifdef SSERIALIZE_WITH_THREADS
		m_cacheLock.lock();
#endif
		m_cache.insert(q, idx);
		return idx;
	}
#else
	return priv()->complete(str, qtype);
#endif
}

ItemIndexIterator StringCompleter::partialComplete(const std::string& str, StringCompleter::QuerryType qtype) {
	return priv()->partialComplete(str, qtype);
}

StringCompleter::ForwardIterator StringCompleter::forwardIterator() const {
	return ForwardIterator( priv()->forwardIterator() );
}

std::map< uint16_t, ItemIndex > StringCompleter::getNextCharacters(const std::string& str, StringCompleter::QuerryType qtype, bool withIndex) const {
	return priv()->getNextCharacters(str, qtype, withIndex);
}

ItemIndex StringCompleter::indexFromId(uint32_t idxId) const {
	return priv()->indexFromId(idxId);
}

StringCompleterPrivate* StringCompleter::getPrivate() const {
	return priv();
}

std::ostream& StringCompleter::printStats(std::ostream& out) const {
	return priv()->printStats(out);
}

std::string StringCompleter::getName() const {
	return priv()->getName();
}

StringCompleter::QuerryType StringCompleter::normalize(std::string & q) {
	uint32_t qt = sserialize::StringCompleter::QT_NONE;
	if (!q.size()) {
		return sserialize::StringCompleter::QT_NONE;
	}
	if (q.front() == '*') {
		if (q.size() > 2 && q[1] == '*') {
			q = std::string(q.cbegin()+1, q.cend()-1);
			qt = sserialize::StringCompleter::QT_SUBSTRING;
		}
		else {
			q = std::string(q.cbegin()+1, q.cend());
			qt = sserialize::StringCompleter::QT_PREFIX;
		}
	}
	else if (q.back() == '*') {
		q.pop_back();
		qt = sserialize::StringCompleter::QT_SUFFIX;
	}
	else if (q.size() == 2 && q.back() == '"' && q.front() == '"') {
		q = std::string(q.cbegin()+1, q.cend()-1);
		qt = sserialize::StringCompleter::QT_EXACT;
	}
	else {
		qt = sserialize::StringCompleter::QT_SUBSTRING;
	}
	return (StringCompleter::QuerryType)qt;
}


}//end namespace
