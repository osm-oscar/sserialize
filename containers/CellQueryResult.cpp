#include <sserialize/containers/CellQueryResult.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {

CellQueryResult CellQueryResult::operator/(const sserialize::CellQueryResult& o) const {
	ItemIndex nFM = m_fullMatches / o.m_fullMatches;
	
	ItemIndex oPM = (m_fullMatches - nFM) / o.m_partialMatches;
	ItemIndex myPM = (o.m_fullMatches - nFM)  / m_partialMatches;

	ItemIndex allFmPM = oPM + myPM;
	
	ItemIndex itemItemCandidates = (m_partialMatches / o.m_partialMatches) - allFmPM;
	
	//we can now assemble the result
	CellQueryResult r;
	r.m_fullMatches = nFM;

	{
		std::vector<uint32_t> itemItemMatchesRaw;
		for(ItemIndex::const_iterator it(itemItemCandidates.cbegin()), end(itemItemCandidates.cend()); it != end; ++it) {
			uint32_t idxId = *it;
			ItemIndex idx = m_partialMatchesItems.at(idxId) / o.m_partialMatchesItems.at(idxId);
			if (idx.size()) {
				itemItemMatchesRaw.push_back(idxId);
				r.m_partialMatchesItems[idxId] = idx; 
			}
		}
		r.m_partialMatches = allFmPM + ItemIndexFactory::create(itemItemMatchesRaw, indexType());
	}
	
	for(ItemIndex::const_iterator it(myPM.cbegin()), end(myPM.cend()); it != end; ++it) {
		uint32_t idxId = *it;
		r.m_partialMatchesItems[idxId] = m_partialMatchesItems.at(idxId);
	}
	
	for(ItemIndex::const_iterator it(oPM.cbegin()), end(oPM.cend()); it != end; ++it) {
		uint32_t idxId = *it;
		r.m_partialMatchesItems[idxId] = o.m_partialMatchesItems.at(idxId);
	}
	return r;
}

CellQueryResult CellQueryResult::operator+(const sserialize::CellQueryResult & o) const {
	CellQueryResult r;
	r.fullMatches() = fullMatches() + o.fullMatches();
	r.partialMatches() = (partialMatches() + o.partialMatches()) - r.fullMatches();
	for(uint32_t i (0), s(r.partialMatches().size()); i < s; ++i) {
		uint32_t idxId = r.partialMatches().at(i);
		if (partialMatchesItems().count(idxId)) {
			if (o.partialMatchesItems().count(idxId))
				r.partialMatchesItems()[idxId] = partialMatchesItems().at(idxId) + o.partialMatchesItems().at(idxId);
			else
				r.partialMatchesItems()[idxId] = partialMatchesItems().at(idxId);
		}
		else {
			r.partialMatchesItems()[idxId] = o.partialMatchesItems().at(idxId);
		}
	}
	return r;
}

}