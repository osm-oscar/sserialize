#ifndef SSERIALIZE_STRING_COMPLETER_PRIVATE_ITEM_DB_H
#define SSERIALIZE_STRING_COMPLETER_PRIVATE_ITEM_DB_H
#include <sserialize/search/StringCompleterPrivate.h>
#include "StringsItemDB.h"

namespace sserialize {

template<typename DBItemType>
class StringCompleterPrivateItemDB: public StringCompleterPrivate {
	StringsItemDB<DBItemType> * m_db;
public:
	StringCompleterPrivateItemDB() : StringCompleterPrivate() {}
	StringCompleterPrivateItemDB(StringsItemDB<DBItemType> * db) : StringCompleterPrivate(), m_db(db) {} 
	virtual ~StringCompleterPrivateItemDB() {}
	
	virtual StringCompleter::SupportedQuerries getSupportedQuerries() const {
		return StringCompleter::SQ_ALL;
	}
	virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const {
		return m_db->complete(str, qtype);
	}
};


}//end namespace


#endif