#ifndef SSERIALIZE_STRING_COMPLETER_PRIVATE_MULTI_H
#define SSERIALIZE_STRING_COMPLETER_PRIVATE_MULTI_H
#include <sserialize/search/StringCompleterPrivate.h>

namespace sserialize {

class StringCompleterPrivateMulti: public StringCompleterPrivate {
private:
	StringCompleter::SupportedQuerries m_sq;
	std::vector< RCPtrWrapper<sserialize::StringCompleterPrivate> > m_completers;
public:
	StringCompleterPrivateMulti();
	virtual ~StringCompleterPrivateMulti();
	
	void addCompleter(const RCPtrWrapper<sserialize::StringCompleterPrivate> & completer);

	virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const;
	
	virtual ItemIndexIterator partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	virtual StringCompleter::SupportedQuerries getSupportedQuerries() const;
	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const;


};

}//end namespace

#endif