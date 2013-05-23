#include <sserialize/completers/StringCompleter.h>
#include <sserialize/completers/StringCompleterPrivate.h>
#ifdef SSERIALIZE_WITH_THREADS
#include <sserialize/utility/MutexLocker.h>
#endif

#define INDEX_CACHE_SIZE 8


namespace sserialize {

StringCompleter::StringCompleter() :
RCWrapper<StringCompleterPrivate>(new StringCompleterPrivate())
{
	m_cache.setSize(INDEX_CACHE_SIZE);
}

StringCompleter::StringCompleter(const StringCompleter& other) :
RCWrapper<StringCompleterPrivate>(other)
{
	m_cache.setSize(INDEX_CACHE_SIZE);
}


StringCompleter::StringCompleter(StringCompleterPrivate * priv) :
RCWrapper< sserialize::StringCompleterPrivate >(priv)
{
	m_cache.setSize(INDEX_CACHE_SIZE);
}

StringCompleter& StringCompleter::operator=(const StringCompleter& strc)
{
	RCWrapper< sserialize::StringCompleterPrivate >::operator=(strc);
	m_cache = strc.m_cache;
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
	m_cache.clear();
}

ItemIndex StringCompleter::complete(const std::string & str, StringCompleter::QuerryType qtype) {
	std::pair<std::string, sserialize::StringCompleter::QuerryType> q(str, qtype);
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
		//TODO:write index to file
#ifdef SSERIALIZE_WITH_THREADS
		m_cacheLock.lock();
#endif
		m_cache.insert(q, idx);
		return idx;
	}
}

ItemIndexIterator StringCompleter::partialComplete(const std::string& str, StringCompleter::QuerryType qtype) {
	return priv()->partialComplete(str, qtype);
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



}//end namespace
