#ifndef SSERIALIZE_STATIC_STRING_COMPLETER_PRIVATE_GEO_HIERARCHY_UNCLUSTERED_H
#define SSERIALIZE_STATIC_STRING_COMPLETER_PRIVATE_GEO_HIERARCHY_UNCLUSTERED_H
#include <sserialize/completers/StringCompleterPrivate.h>
#include <sserialize/completers/StringCompleter.h>
#include <sserialize/Static/GeoHierarchy.h>

namespace sserialize {
namespace Static {
namespace detail {
namespace StringCompleter {

class GeoHierarchyUnclustered: public sserialize::StringCompleterPrivate {
public:
	typedef sserialize::RCPtrWrapper<sserialize::StringCompleterPrivate> MyStringCompleter;
	typedef sserialize::Static::spatial::GeoHierarchy MyGeoHierarchy;
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
	sserialize::Static::ItemIndexStore m_store;
	sserialize::RCPtrWrapper<sserialize::StringCompleterPrivate> m_ghCompleter;
	sserialize::RCPtrWrapper<sserialize::StringCompleterPrivate> m_directCompleter;
	
public:
	GeoHierarchyUnclustered();
	GeoHierarchyUnclustered(const MyGeoHierarchy & gh, const sserialize::Static::ItemIndexStore & store, const MyStringCompleter & ghCompleter, const MyStringCompleter & itemsCompleter);
	virtual ~GeoHierarchyUnclustered();
	
	virtual ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	virtual sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;

	virtual ItemIndex indexFromId(uint32_t idxId) const;

	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const;
	
};

}}}}

#endif