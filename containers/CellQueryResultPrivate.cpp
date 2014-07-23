#include <sserialize/containers/CellQueryResultPrivate.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {
namespace detail {

CellQueryResult::CellQueryResult() :
m_idx(0)
{}

CellQueryResult::CellQueryResult(const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore)
{}

CellQueryResult::~CellQueryResult() {
	std::vector<CellDesc>::const_iterator dIt(m_desc.cbegin()), dEnd(m_desc.cend());
	IndexDesc * it(m_idx);
	IndexDesc * end(m_idx+m_desc.size());
	for(; it != end; ++it, ++dIt) {
		if (dIt->fetched) {
			it->idx.sserialize::ItemIndex::~ItemIndex();
		}
	}
	free(m_idx);
	m_idx = 0;
}
	
void CellQueryResult::uncheckedSet(uint32_t pos, const sserialize::ItemIndex & idx) {
	new(m_idx+pos) sserialize::ItemIndex(idx);
}

const sserialize::ItemIndex & CellQueryResult::idx(uint32_t pos) const {
	const CellDesc & cd = m_desc[pos];
	if (cd.fetched) {
		return (m_idx+pos)->idx;
	}
	uint32_t idxId;
	if (cd.fullMatch) {
		idxId = m_gh.cellItemsPtr(cd.cellId);
	}
	else {
		idxId = (m_idx+pos)->idxPtr;
	}
	//TODO: make this thread-safe
	const_cast<CellQueryResult*>(this)->uncheckedSet(pos, m_idxStore.at(idxId));
	const_cast<CellDesc&>(cd).fetched = 1;
	return (m_idx+pos)->idx;
}

uint32_t CellQueryResult::idxId(uint32_t pos) const {
	const CellDesc & cd = m_desc[pos];
	if (cd.fullMatch) {
		return m_gh.cellItemsPtr(cd.cellId);
	}
	else {
		return (m_idx+pos)->idxPtr;
	}
}

CellQueryResult * CellQueryResult::intersect(const CellQueryResult * oPtr) const {
	const CellQueryResult & o = *oPtr;
	CellQueryResult * rPtr = new CellQueryResult(m_gh, m_idxStore);
	CellQueryResult & r = *rPtr;
	r.m_idx = (IndexDesc*) malloc(sizeof(IndexDesc) * std::min<uint32_t>(m_desc.size(), o.m_desc.size()));
	

	for(uint32_t myI(0), myEnd(m_desc.size()), oI(0), oEnd(o.m_desc.size()); myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			++oI;
			continue;
		}
		uint32_t ct = (myCD.fullMatch << 1) | oCD.fullMatch;
		switch(ct) {
		case 0x0: //both partial
			{
				const sserialize::ItemIndex & myPIdx = idx(myI);
				const sserialize::ItemIndex & oPIdx = o.idx(oI);
				sserialize::ItemIndex res(myPIdx / oPIdx);
				if (res.size()) {
					r.uncheckedSet(r.m_desc.size(), res);
					r.m_desc.push_back(CellDesc(0, 1, myCellId));
				}
			}
			break;
		case 0x1: //o full
			if (myCD.fetched) {
				r.uncheckedSet(r.m_desc.size(), m_idx[myI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
			}
			r.m_desc.push_back(myCD);
			break;
		case 0x2: //my full
			if (oCD.fetched) {
				r.uncheckedSet(r.m_desc.size(), o.m_idx[oI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = o.m_idx[oI].idxPtr;
			}
			r.m_desc.push_back(oCD);
			break;
		case 0x3:
			if (oCD.fetched) {
				r.uncheckedSet(r.m_desc.size(), o.idx(oI));
			}
			else if (myCD.fetched) {
				r.uncheckedSet(r.m_desc.size(), idx(myI));
			}
			r.m_desc.push_back(CellDesc(1, oCD.fetched | myCD.fetched, myCellId));
			break;
		default:
			break;
		};
		++myI;
		++oI;
	}
	r.m_idx = (IndexDesc*) realloc(r.m_idx, r.m_desc.size()*sizeof(IndexDesc));
	return rPtr;
}

CellQueryResult * CellQueryResult::unite(const CellQueryResult * other) const {
	return 0;
}

CellQueryResult * CellQueryResult::diff(const CellQueryResult * other) const {
	return 0;
}

CellQueryResult * CellQueryResult::symDiff(const CellQueryResult * other) const {
	return 0;
}

}}//end namespace