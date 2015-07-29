#ifndef STRING_COMPLETER_PRIVATE_STATIC_DB_H
#define STRING_COMPLETER_PRIVATE_STATIC_DB_H
#include <sserialize/completers/StringCompleterPrivate.h>
#include <sserialize/completers/StringCompleter.h>
#include <sserialize/containers/ItemIndexIteratorDB.h>
#include <unordered_set>

namespace sserialize {

template<class DataBaseType>
class StringCompleterPrivateStaticDB: public StringCompleterPrivate {
private:
	DataBaseType m_sdb;
public:
	StringCompleterPrivateStaticDB() : StringCompleterPrivate() {}
	StringCompleterPrivateStaticDB(const DataBaseType & sdb) : 
	StringCompleterPrivate(), m_sdb(sdb) {}
	virtual ~StringCompleterPrivateStaticDB() {}

	virtual StringCompleter::SupportedQuerries getSupportedQuerries() const {
		return m_sdb.getSupportedQuerries();
	}
	
	virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const {
		return m_sdb.complete(str, qtype);
	}
	
	virtual ItemIndexIterator partialComplete(const std::string & str, StringCompleter::QuerryType qtype) const {
		return m_sdb.partialComplete(str, qtype);
	}

	virtual ItemIndex select(const std::unordered_set<uint32_t> & strIds) const {
		return m_sdb.select(strIds);
	}
	
	virtual std::ostream& printStats(std::ostream& out) const {
		return out;
	}
	
	virtual std::string getName() const {
		return std::string("Static::StringsItemDB");
	}
	
	DataBaseType & getDB() {
		return m_sdb;
	}
};


}//end namespace
#endif