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
}

StringCompleter::StringCompleter(const StringCompleter& other) :
MyBaseClass(other)
{
}


StringCompleter::StringCompleter(StringCompleterPrivate * priv) :
MyBaseClass(priv)
{
}

StringCompleter::StringCompleter(const sserialize::RCPtrWrapper<StringCompleterPrivate> & priv) :
MyBaseClass(priv)
{
}

StringCompleter& StringCompleter::operator=(const StringCompleter& strc) {
	RCWrapper< sserialize::StringCompleterPrivate >::operator=(strc);
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

ItemIndex StringCompleter::complete(const std::string & str, StringCompleter::QuerryType qtype) {
	return priv()->complete(str, qtype);
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


//check if explicit match operators were set, and remove them
//@return StringCompleter::QuerryType
StringCompleter::QuerryType myNormalize(std::string & q) {
	uint32_t qt = sserialize::StringCompleter::QT_NONE;
	if (!q.size()) {
		return sserialize::StringCompleter::QT_NONE;
	}
	if (q.front() == '?') {
		if (q.size() >= 2 && q.back() == '?') {
			q = std::string(q.cbegin()+1, q.cend()-1);
			qt = sserialize::StringCompleter::QT_SUBSTRING;
		}
		else {
			q = std::string(q.cbegin()+1, q.cend());
			qt = sserialize::StringCompleter::QT_SUFFIX;
		}
	}
	else if (q.back() == '?') {
		q.pop_back();
		qt = sserialize::StringCompleter::QT_PREFIX;
	}
	return (StringCompleter::QuerryType) qt;
}

StringCompleter::QuerryType StringCompleter::normalize(std::string & q) {
	uint32_t qt = myNormalize(q);
	if (qt == sserialize::StringCompleter::QT_NONE && q.size() >= 2 && q.back() == '"' && q.front() == '"') {
		q = std::string(q.cbegin()+1, q.cend()-1);
		qt = myNormalize(q);
		if (qt == sserialize::StringCompleter::QT_NONE) {
			qt = sserialize::StringCompleter::QT_EXACT;
		}
	}
	else {
		qt = sserialize::StringCompleter::QT_SUBSTRING;
	}
	return (StringCompleter::QuerryType)qt;
}


}//end namespace
