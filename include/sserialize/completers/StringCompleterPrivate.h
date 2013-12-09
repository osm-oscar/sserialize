#ifndef SSERIALIZE_STRING_COMPLETER_PRIVATE_H
#define SSERIALIZE_STRING_COMPLETER_PRIVATE_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/types.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/completers/StringCompleter.h>

namespace sserialize {

class StringCompleterPrivate: public RefCountObject {
public:
	typedef detail::types::StringCompleterPrivate::ForwardIterator ForwardIterator;
public:
	StringCompleterPrivate();
	virtual ~StringCompleterPrivate();
	
	virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const;
	
	virtual ItemIndexIterator partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	/** @return returns pairs of char->ItemIndex **/
	virtual std::map<uint16_t, ItemIndex> getNextCharacters(const std::string& str, sserialize::StringCompleter::QuerryType qtype, bool withIndex) const;

	virtual StringCompleter::SupportedQuerries getSupportedQuerries() const;

	virtual ItemIndex indexFromId(uint32_t idxId) const;

	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const;
	
	virtual ForwardIterator * forwardIterator() const;
};

}//end namespace

#endif