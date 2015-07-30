#include <sserialize/search/StringCompleterPrivateGeoHierarchyUnclustered.h>
#include <sserialize/Static/StringCompleter.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/containers/DynamicBitSet.h>


namespace sserialize {
namespace Static {
namespace detail {
namespace StringCompleter {

GeoHierarchyUnclustered::GeoHierarchyUnclustered() {}

GeoHierarchyUnclustered::GeoHierarchyUnclustered(const sserialize::Static::spatial::GeoHierarchy& gh, const sserialize::Static::ItemIndexStore & store, const MyStringCompleter & ghCompleter, const MyStringCompleter & itemsCompleter) :
m_gh(gh),
m_store(store),
m_ghCompleter(ghCompleter),
m_itemsCompleter(itemsCompleter)
{}

GeoHierarchyUnclustered::GeoHierarchyUnclustered(const sserialize::Static::spatial::GeoHierarchy & gh, const sserialize::Static::ItemIndexStore& idxStore, const sserialize::UByteArrayAdapter& data) :
m_gh(gh),
m_store(idxStore)
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_GEOHIERARCHY_UNCLUSTERED_COMPLETER_VERSION, data.at(0), "sserialize::Static::StringCompleter::GeoHierarchyUnclustered");
	sserialize::Static::StringCompleter * ic = new sserialize::Static::StringCompleter(data+1, m_store);
	sserialize::Static::StringCompleter * gc = 0;
	try {
		gc = new sserialize::Static::StringCompleter(data+(1+ic->getSizeInBytes()), m_store);
	}
	catch(const std::exception & e) {
		delete ic;
		ic = 0;
		throw e;
	}
	m_itemsCompleter.reset(ic);
	m_ghCompleter.reset(gc);
}

GeoHierarchyUnclustered::~GeoHierarchyUnclustered() {}

ItemIndex GeoHierarchyUnclustered::complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {
	ItemIndex ghIdx = m_ghCompleter->complete(str, qtype);
	ItemIndex itemIdx = m_itemsCompleter->complete(str, qtype);
	if (ghIdx.size() < 1000) {
		std::vector<ItemIndex> mergeIdcs(ghIdx.size()+1);
		uint32_t i = 0;
		for(uint32_t regionId : ghIdx) {
			uint32_t ghRegionId = m_gh.storeIdToGhId(regionId);
			ItemIndex & idx = mergeIdcs[i];
			idx = m_store.at(m_gh.regionItemsPtr(ghRegionId));
			++i;
		}
		mergeIdcs[i] = itemIdx;
		itemIdx = ItemIndex::unite(mergeIdcs);
	}
	else if (ghIdx.size() > 1000) {
		sserialize::DynamicBitSet bitSet;
		for(uint32_t regionId : ghIdx) {
			uint32_t ghRegionId = m_gh.storeIdToGhId(regionId);
			m_store.at(m_gh.regionItemsPtr(ghRegionId)).putInto(bitSet);
		}
		itemIdx = sserialize::ItemIndex::fromBitSet(bitSet, m_store.indexType()) + itemIdx;
	}
	return itemIdx;
}

sserialize::StringCompleter::SupportedQuerries GeoHierarchyUnclustered::getSupportedQuerries() const {
	return (sserialize::StringCompleter::SupportedQuerries) (m_itemsCompleter->getSupportedQuerries() & m_ghCompleter->getSupportedQuerries());
}

ItemIndex GeoHierarchyUnclustered::indexFromId(uint32_t idxId) const {
	return m_itemsCompleter->indexFromId(idxId);
}

std::ostream& GeoHierarchyUnclustered::printStats(std::ostream& out) const {
	out << "GeoHierarchyUnclustered::stats--BEGIN" << std::endl;
	out << "ItemsCompleter\n";
	m_itemsCompleter->printStats(out);
	out << "GeoHierarchyCompleter\n";
	m_ghCompleter->printStats(out);
	out << "GeoHierarchy\n";
	m_gh.printStats(out, m_store);
	out << "GeoHierarchyUnclustered::stats--END" << std::endl;
	return out;
}
	
std::string GeoHierarchyUnclustered::getName() const {
	return std::string("GeoHierarchyUnclustered");
}
	
};

}}}