#include "StringCompleterPrivate.h"
#include "StringCompleter.h"

namespace sserialize {
	
StringCompleterPrivate::StringCompleterPrivate() :
RefCountObject()
{}

StringCompleterPrivate::~StringCompleterPrivate() {}

StringCompleter::SupportedQuerries StringCompleterPrivate::getSupportedQuerries() const {
	return StringCompleter::SQ_NONE;
}

ItemIndex StringCompleterPrivate::complete(const std::string& str, sserialize::StringCompleter::QuerryType qtype) const {
	return ItemIndex();
}

ItemIndexIterator StringCompleterPrivate::partialComplete(const std::string& str, StringCompleter::QuerryType qtype) const {
	return ItemIndexIterator( complete(str, qtype) );
}


std::map< uint16_t, ItemIndex > StringCompleterPrivate::getNextCharacters(const std::string& str, sserialize::StringCompleter::QuerryType qtype, bool withIndex) const {
	return std::map< uint16_t, ItemIndex >();
}

ItemIndex StringCompleterPrivate::indexFromId(uint32_t idxId) const {
	return ItemIndex();
}

std::ostream& StringCompleterPrivate::printStats(std::ostream& out) const {
	out << "StringCompleterPrivate::printStats: empty" << out;
	return out;
}

std::string StringCompleterPrivate::getName() const {
	return std::string("DefaultStringCompleterName");
}


}