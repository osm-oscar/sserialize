#include <sserialize/containers/CellQueryResult.h>
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {

CellQueryResult CellQueryResult::operator/(const sserialize::CellQueryResult& o) const {
	ItemIndex nFM = m_fullMatches / o.m_fullMatches;
	ItemIndex dMy = m_fullMatches - nFM;
	ItemIndex dO = o.m_fullMatches - nFM;
	
	
	ItemIndex nP1 = dMy / o.m_partialMatches;
	ItemIndex nP2 = dO / m_partialMatches;
	ItemIndex nP12 = nP1 + nP2;
	
	ItemIndex itemItemCandidates = (m_partialMatches - nP12) / (o.m_partialMatches - nP12);
	
	//we can now assemble the result
	CellQueryResult r;
	r.m_fullMatches = nFM;

	{
		std::vector<uint32_t> itemItemMatchesRaw;
		for(uint32_t i(0), s(itemItemCandidates.size()); i < s; ++i) {
			uint32_t idxId = itemItemCandidates.at(i);
			ItemIndex idx = m_partialMatchesItems.at(idxId) / o.m_partialMatchesItems.at(idxId);
			if (idx.size()) {
				itemItemMatchesRaw.push_back(i);
				r.m_partialMatchesItems[idxId] = idx; 
			}
		}
		UByteArrayAdapter itemItemMatchesData(UByteArrayAdapter::createCache(1, false));
		r.m_partialMatches = nP12 + ItemIndex::create(itemItemMatchesRaw, itemItemMatchesData, indexType());
	}
	for(uint32_t i(0), s(nP12.size()); i < s; ++i) {
		uint32_t idxId = nP12.at(i);
		r.m_partialMatchesItems[idxId] = m_partialMatchesItems.at(idxId);
	}
	return r;
}

}