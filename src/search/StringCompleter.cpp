#include <sserialize/search/StringCompleter.h>
#include <sserialize/search/StringCompleterPrivate.h>
#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/algorithm/utilmath.h>

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
		return (int(sq) & int(qt)) == qt;
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
	if (q.size() >= 2 && q.back() == '"' && q.front() == '"') {
		q = std::string(q.cbegin()+1, q.cend()-1);
		if (qt == sserialize::StringCompleter::QT_NONE) {
			qt = sserialize::StringCompleter::QT_EXACT;
		}
	}
	else if (qt == sserialize::StringCompleter::QT_NONE) {
		qt = sserialize::StringCompleter::QT_SUBSTRING;
	}
	return (StringCompleter::QuerryType)qt;
}

StringCompleter::QuerryType
StringCompleter::toAvailable(int requested, int available) {
	uint32_t qt = requested & sserialize::StringCompleter::QT_EPSS;

	if (! (qt & available )) {
		if (qt & sserialize::StringCompleter::QT_PREFIX) {
			qt = available & sserialize::StringCompleter::QT_EXACT;
		}
		else if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
			qt = available & sserialize::StringCompleter::QT_SUFFIX;
		}
		else {
			qt = sserialize::StringCompleter::QT_NONE;
		}
	}
	if (sserialize::popCount<uint32_t>(qt) > 1) {
		qt = 1 << sserialize::fastLog2(qt);
	}
	return QuerryType(qt);
}

bool StringCompleter::matches(const std::string & str, const std::string & qstr, StringCompleter::QuerryType qt) {
	const std::string * myStr;
	const std::string * myQStr;

	std::string lStr, lqStr;
	
	if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
		lStr = sserialize::unicode_to_lower(str);
		lqStr = sserialize::unicode_to_lower(qstr);
		myStr = &lStr;
		myQStr = &lqStr;
	}
	else {
		myStr = &str;
		myQStr = &qstr;
	}
	
	if (qt & sserialize::StringCompleter::QT_EXACT || qt & sserialize::StringCompleter::QT_PREFIX) {
		std::size_t firstPos = myStr->find(*myQStr);
		if (firstPos == 0) {
			if (qt & sserialize::StringCompleter::QT_EXACT && myStr->size() == myQStr->size()) {
				return true;
			}
			if (qt & sserialize::StringCompleter::QT_PREFIX) {
				return true;
			}
		}
	}
	else {
		std::size_t lastPos = myStr->rfind(*myQStr);
		if (lastPos < std::string::npos) {
			if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
				return true;
			}
			if (qt & sserialize::StringCompleter::QT_SUFFIX && myQStr->size() + lastPos == myStr->size()) {
				return true;
			}
		}
	}
	return false;
}


}//end namespace
