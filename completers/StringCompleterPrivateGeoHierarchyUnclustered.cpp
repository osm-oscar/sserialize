#include <sserialize/completers/StringCompleterPrivateGeoHierarchyUnclustered.h>


namespace sserialize {
namespace Static {
namespace detail {
namespace StringCompleter {

GeoHierarchyUnclustered::GeoHierarchyUnclustered() {}

GeoHierarchyUnclustered::GeoHierarchyUnclustered(const sserialize::Static::spatial::GeoHierarchy& gh, const sserialize::Static::ItemIndexStore & store, const MyStringCompleter & ghCompleter, const MyStringCompleter & itemsCompleter) :
m_gh(gh),
m_store(store),
m_ghCompleter(ghCompleter),
m_directCompleter(itemsCompleter)
{}

GeoHierarchyUnclustered::~GeoHierarchyUnclustered() {}

ItemIndex GeoHierarchyUnclustered::complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {
	ItemIndex ghIdx = m_ghCompleter->complete(str, qtype);
	ItemIndex itemIdx = m_directCompleter->complete(str, qtype);
	if (ghIdx.size()) {
		std::vector<ItemIndex> mergeIdcs(ghIdx.size()+1);
		uint32_t i = 0;
		for(uint32_t s = ghIdx.size(); i < s; ++i) {
			mergeIdcs[i] = m_store.at(m_gh.regionItemsPtr(ghIdx.at(i)));
		}
		mergeIdcs[i] = itemIdx;
		return ItemIndex::unite(mergeIdcs);
	}
	return itemIdx;
}

sserialize::StringCompleter::SupportedQuerries GeoHierarchyUnclustered::getSupportedQuerries() const {
	return (sserialize::StringCompleter::SupportedQuerries) (m_directCompleter->getSupportedQuerries() & m_ghCompleter->getSupportedQuerries());
}

ItemIndex GeoHierarchyUnclustered::indexFromId(uint32_t idxId) const {
	return m_directCompleter->indexFromId(idxId);
}

std::ostream& GeoHierarchyUnclustered::printStats(std::ostream& out) const {
	out << "GeoHierarchyUnclustered::stats--BEGIN" << std::endl;
	m_directCompleter->printStats(out);
	m_ghCompleter->printStats(out);
	m_gh.printStats(out);
	out << "GeoHierarchyUnclustered::stats" << std::endl;
	return out;
}
	
std::string GeoHierarchyUnclustered::getName() const {
	return std::string("GeoHierarchyUnclustered");
}
	
};

}}}