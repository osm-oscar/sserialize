#ifndef STRING_COMPLETER_PRIVATE_STATIC_GSA_STR_IDS_H
#define STRING_COMPLETER_PRIVATE_STATIC_GSA_STR_IDS_H
#include <sserialize/search/StringCompleterPrivate.h>
#include <sserialize/Static/GeneralizedSuffixArray.h>

namespace sserialize {

template<class DataBaseType>
class StringCompleterPrivateStaticGSAStrIds: public sserialize::StringCompleterPrivate {
private:
	Static::GeneralizedSuffixArray m_gsa;
	DataBaseType m_sdb;
public:
	StringCompleterPrivateStaticGSAStrIds() : StringCompleterPrivate() {}
	StringCompleterPrivateStaticGSAStrIds(const Static::GeneralizedSuffixArray & gsa, const DataBaseType & sdb) : 
	StringCompleterPrivate(), m_gsa(gsa), m_sdb(sdb) {}
	virtual ~StringCompleterPrivateStaticGSAStrIds() {}

	virtual StringCompleter::SupportedQuerries getSupportedQuerries() const {
		return m_gsa.getSupportedQuerries();
	}
	
	virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const {
		return m_sdb.select( m_gsa.match(str, qtype) );
	}
	
	virtual ItemIndex select(const std::unordered_set<uint32_t> & strIds) {
		return m_sdb.select(strIds);
	}
	
	virtual std::ostream& printStats(std::ostream& out) const {
		return out;
	}
	
	DataBaseType & getDB() {
		return m_sdb;
	}
	
	Static::GeneralizedSuffixArray & getGSA() {
		return m_gsa;
	}
};


}//end namespace
#endif