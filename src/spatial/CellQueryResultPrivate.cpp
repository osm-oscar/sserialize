#include <sserialize/spatial/CellQueryResultPrivate.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {
namespace detail {

CellQueryResult::CellQueryResult() :
m_flags(sserialize::CellQueryResult::FF_NONE),
m_idx(0)
{}

CellQueryResult::CellQueryResult(const sserialize::ItemIndex & fmIdx,
	const sserialize::ItemIndex & pmIdx,
	std::vector<sserialize::ItemIndex>::const_iterator pmItemsIt,
	const GeoHierarchy & gh,
	const ItemIndexStore & idxStore,
	int flags)
:
m_gh(gh),
m_idxStore(idxStore),
m_flags(flags)
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend()), pmIt(pmIdx.cbegin()), pmEnd(pmIdx.cend());

	uint32_t totalSize = fmIdx.size() + pmIdx.size();
	m_desc.reserve(totalSize);
	m_idx = (IndexDesc*) malloc(totalSize * sizeof(IndexDesc));
	uint32_t pos = 0;
	for(; fmIt != fmEnd && pmIt != pmEnd; ++pos) {
		uint32_t fCellId = *fmIt;
		uint32_t pCellId = *pmIt;
		if(fCellId < pCellId) {
			m_desc.emplace_back(1, 0, fCellId);
			++fmIt;
		}
		else if (fCellId == pCellId) {
			m_desc.emplace_back(1, 0, fCellId);
			++fmIt;
			++pmIt;
			++pmItemsIt;
		}
		else {
			m_desc.emplace_back(0, 1, pCellId);
			this->uncheckedSet(pos,*pmItemsIt);
			++pmIt;
			++pmItemsIt;
		}
	}
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.emplace_back(1, 0, *fmIt);
	}
	
	for(; pmIt != pmEnd; ++pos, ++pmIt, ++pmItemsIt) {
		m_desc.emplace_back(0, 1, *pmIt);
		this->uncheckedSet(pos, *pmItemsIt);
	}
	
	m_desc.shrink_to_fit();
	//return should stay the same since gthis is just a shrink
	m_idx = (IndexDesc*) ::realloc(m_idx, m_desc.size()*sizeof(IndexDesc));

	SSERIALIZE_NORMAL_ASSERT(selfCheck());
}

CellQueryResult::CellQueryResult(
	const ItemIndex & fmIdx,
	const GeoHierarchy & gh,
	const ItemIndexStore & idxStore,
	int flags)
:
m_gh(gh),
m_idxStore(idxStore),
m_flags(flags)
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend());

	uint32_t totalSize = fmIdx.size();
	m_desc.reserve(totalSize);
	m_idx = (IndexDesc*) malloc(totalSize * sizeof(IndexDesc));
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.push_back( CellDesc(1, 0, *fmIt) );
	}
	SSERIALIZE_NORMAL_ASSERT(selfCheck());
}

CellQueryResult::CellQueryResult(
	bool fullMatch,
	uint32_t cellId,
	const GeoHierarchy & gh,
	const ItemIndexStore & idxStore,
	uint32_t cellIdxId,
	int flags)
:
m_gh(gh),
m_idxStore(idxStore),
m_flags(flags)
{

	uint32_t totalSize = 1;
	m_desc.reserve(totalSize);
	m_idx = (IndexDesc*) malloc(totalSize * sizeof(IndexDesc));
	IndexDesc * idxPtr = m_idx;
	m_desc.push_back( CellDesc((fullMatch ? 1 : 0), 0, cellId) );
	if (!fullMatch) {
		idxPtr[0].idxPtr = cellIdxId;
	}
	SSERIALIZE_NORMAL_ASSERT(selfCheck());
}

CellQueryResult::CellQueryResult(
	const GeoHierarchy & gh,
	const ItemIndexStore & idxStore,
	int flags)
:
m_gh(gh),
m_idxStore(idxStore),
m_flags(flags)
{}

CellQueryResult::~CellQueryResult() {
	std::vector<CellDesc>::const_iterator dIt(m_desc.cbegin());
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

bool CellQueryResult::flagCheck(int first, int second) { 
	return (first | second) == first;
}

uint32_t CellQueryResult::idxSize(uint32_t pos) const {
	const CellDesc & cd = m_desc[pos];
	if (cd.fetched) {
		return (m_idx+pos)->idx.size();
	}
	if (cd.fullMatch) {
		return m_gh.cellItemsCount(cd.cellId);
	}
	else {
		return m_idxStore.idxSize( (m_idx+pos)->idxPtr );
	}
}

const sserialize::ItemIndex & CellQueryResult::idx(uint32_t pos) const {
	auto that = const_cast<CellQueryResult*>(this);
	CellDesc & cd = that->m_desc[pos];
	if (cd.fetched) {
		return (m_idx+pos)->idx;
	}
	if (cd.fullMatch) {
		if (flags() & sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS) {
			uint32_t cellSize = m_gh.cellItemsCount(cd.cellId);
			auto idxType = m_idxStore.indexType();
			that->uncheckedSet(pos, sserialize::ItemIndexFactory::range(0, cellSize, 1, idxType));
		}
		else {
			uint32_t idxId = m_gh.cellItemsPtr(cd.cellId);
			that->uncheckedSet(pos, m_idxStore.at(idxId));
		}
	}
	else {
		uint32_t idxId = (m_idx+pos)->idxPtr;
		that->uncheckedSet(pos, m_idxStore.at(idxId));
	}
	cd.fetched = 1;
	return (m_idx+pos)->idx;
}

uint32_t CellQueryResult::idxId(uint32_t pos) const {
	const CellDesc & cd = m_desc[pos];
	if (cd.fullMatch) {
		SSERIALIZE_CHEAP_ASSERT((flags()&sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS) == 0);
		return m_gh.cellItemsPtr(cd.cellId);
	}
	else {
		return (m_idx+pos)->idxPtr;
	}
}

uint32_t CellQueryResult::maxItems() const {
	uint32_t tmp = 0;
	for(uint32_t i(0), s(cellCount()); i < s; ++i) {
		tmp += idxSize(i);
	}
	return tmp;
}

CellQueryResult * CellQueryResult::intersect(const CellQueryResult * oPtr) const {
	SSERIALIZE_CHEAP_ASSERT(flagCheck(flags(), oPtr->flags()));
	const CellQueryResult & o = *oPtr;
	CellQueryResult * rPtr = new CellQueryResult(m_gh, m_idxStore, m_flags);
	CellQueryResult & r = *rPtr;
	r.m_idx = (IndexDesc*) malloc(sizeof(IndexDesc) * std::min<std::size_t>(m_desc.size(), o.m_desc.size()));
	

	for(uint32_t myI(0), myEnd((uint32_t)m_desc.size()), oI(0), oEnd((uint32_t)o.m_desc.size()); myI < myEnd && oI < oEnd;) {
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
		int ct = (myCD.fullMatch << 1) | oCD.fullMatch;
		switch(ct) {
		case 0x0: //both partial
			{
				const sserialize::ItemIndex & myPIdx = idx(myI);
				const sserialize::ItemIndex & oPIdx = o.idx(oI);
				sserialize::ItemIndex res(myPIdx / oPIdx);
				if (res.size()) {
					r.uncheckedSet((uint32_t)r.m_desc.size(), res);
					r.m_desc.push_back(CellDesc(0, 1, myCellId));
				}
			}
			break;
		case 0x1: //o full
			if (myCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), m_idx[myI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
			}
			r.m_desc.push_back(myCD);
			break;
		case 0x2: //my full
			if (oCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), o.m_idx[oI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = o.m_idx[oI].idxPtr;
			}
			r.m_desc.push_back(oCD);
			break;
		case 0x3:
			if (oCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), o.idx(oI));
			}
			else if (myCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), idx(myI));
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = o.m_idx[oI].idxPtr;
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
	SSERIALIZE_CHEAP_ASSERT(flagCheck(flags(), other->flags()));
	const CellQueryResult & o = *other;
	CellQueryResult * rPtr = new CellQueryResult(m_gh, m_idxStore, m_flags);
	CellQueryResult & r = *rPtr;
	r.m_idx = (IndexDesc*) malloc(sizeof(IndexDesc) * (m_desc.size() + o.m_desc.size()));
	
	uint32_t myI(0), myEnd((uint32_t)m_desc.size()), oI(0), oEnd((uint32_t)o.m_desc.size());
	for(; myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			//do copy
			if (myCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), m_idx[myI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
			}
			r.m_desc.push_back(myCD);
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			//do copy
			if (oCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), o.m_idx[oI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = o.m_idx[oI].idxPtr;
			}
			r.m_desc.push_back(oCD);
			++oI;
			continue;
		}
		int ct = (myCD.fullMatch << 1) | oCD.fullMatch;
		switch(ct) {
		case 0x0: //both partial
			{
				const sserialize::ItemIndex & myPIdx = idx(myI);
				const sserialize::ItemIndex & oPIdx = o.idx(oI);
				sserialize::ItemIndex res(myPIdx + oPIdx);
				if (res.size() == m_gh.cellItemsCount(myCellId)) {
					r.m_idx[r.m_desc.size()].idxPtr = m_gh.cellItemsPtr(myCellId);
					r.m_desc.push_back(CellDesc(1, 0, myCellId));
				}
				else {
					r.uncheckedSet((uint32_t)r.m_desc.size(), res);
					r.m_desc.push_back(CellDesc(0, 1, myCellId));
				}
			}
			break;
		case 0x2: //my full
			if (myCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), m_idx[myI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
			}
			r.m_desc.push_back(myCD);
			break;
		case 0x1: //o full
			if (oCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), o.m_idx[oI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = o.m_idx[oI].idxPtr;
			}
			r.m_desc.push_back(oCD);
			break;
		case 0x3: //both full
			if (oCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), o.idx(oI));
			}
			else if (myCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), idx(myI));
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
			}
			r.m_desc.push_back(CellDesc(1, oCD.fetched | myCD.fetched, myCellId));
			break;
		default:
			break;
		};
		++myI;
		++oI;
	}
	//unite the rest
	for(; myI < myEnd;) {
		const CellDesc & myCD = m_desc[myI];
		if (myCD.fetched) {
			r.uncheckedSet((uint32_t)r.m_desc.size(), m_idx[myI].idx);
		}
		else {
			r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
		}
		r.m_desc.push_back(myCD);
		++myI;
	}

	for(; oI < oEnd;) {
		const CellDesc & oCD = o.m_desc[oI];
		if (oCD.fetched) {
			r.uncheckedSet((uint32_t)r.m_desc.size(), o.m_idx[oI].idx);
		}
		else {
			r.m_idx[r.m_desc.size()].idxPtr = o.m_idx[oI].idxPtr;
		}
		r.m_desc.push_back(oCD);
		++oI;
	}
	r.m_idx = (IndexDesc*) realloc(r.m_idx, r.m_desc.size()*sizeof(IndexDesc));
	SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(r.m_desc.size(), std::max<std::size_t>(m_desc.size(), o.m_desc.size()));
	return rPtr;
}

CellQueryResult * CellQueryResult::diff(const CellQueryResult * other) const {
	SSERIALIZE_CHEAP_ASSERT(flagCheck(flags(), other->flags()));
	const CellQueryResult & o = *other;
	CellQueryResult * rPtr = new CellQueryResult(m_gh, m_idxStore, m_flags);
	CellQueryResult & r = *rPtr;
	r.m_idx = (IndexDesc*) malloc(sizeof(IndexDesc) * m_desc.size());
	
	uint32_t myI(0), myEnd((uint32_t) m_desc.size());
	for(uint32_t oI(0), oEnd((uint32_t)o.m_desc.size()); myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			if (myCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), m_idx[myI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
			}
			r.m_desc.push_back(myCD);
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			++oI;
			continue;
		}
		if (!oCD.fullMatch) {
			const sserialize::ItemIndex & myPIdx = idx(myI);
			const sserialize::ItemIndex & oPIdx = o.idx(oI);
			sserialize::ItemIndex res(myPIdx - oPIdx);
			if (res.size()) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), res);
				r.m_desc.push_back(CellDesc(0, 1, myCellId));
			}
		}
		++myI;
		++oI;
	}

	for(; myI < myEnd;) {
		const CellDesc & myCD = m_desc[myI];
		if (myCD.fetched) {
			r.uncheckedSet((uint32_t)r.m_desc.size(), m_idx[myI].idx);
		}
		else {
			r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
		}
		r.m_desc.push_back(myCD);
		++myI;
		continue;
	}
	r.m_idx = (IndexDesc*) realloc(r.m_idx, r.m_desc.size()*sizeof(IndexDesc));
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(r.m_desc.size(), m_desc.size());
	return rPtr;
}

CellQueryResult * CellQueryResult::symDiff(const CellQueryResult * other) const {
	SSERIALIZE_CHEAP_ASSERT(flagCheck(flags(), other->flags()));
	const CellQueryResult & o = *other;
	CellQueryResult * rPtr = new CellQueryResult(m_gh, m_idxStore, m_flags);
	CellQueryResult & r = *rPtr;
	r.m_idx = (IndexDesc*) malloc(sizeof(IndexDesc) * (m_desc.size() + o.m_desc.size()));
	
	uint32_t myI(0), myEnd((uint32_t)m_desc.size()), oI(0), oEnd((uint32_t)o.m_desc.size());
	for(; myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			//do copy
			if (myCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), m_idx[myI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
			}
			r.m_desc.push_back(myCD);
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			//do copy
			if (oCD.fetched) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), o.m_idx[oI].idx);
			}
			else {
				r.m_idx[r.m_desc.size()].idxPtr = o.m_idx[oI].idxPtr;
			}
			r.m_desc.push_back(oCD);
			++oI;
			continue;
		}
		int ct = (myCD.fullMatch << 1) | oCD.fullMatch;
		if (ct != 0x3) {
			const sserialize::ItemIndex & myPIdx = idx(myI);
			const sserialize::ItemIndex & oPIdx = o.idx(oI);
			sserialize::ItemIndex res(myPIdx ^ oPIdx);
			if (res.size()) {
				r.uncheckedSet((uint32_t)r.m_desc.size(), res);
				r.m_desc.push_back(CellDesc(0, 1, myCellId));
			}
		}
		++myI;
		++oI;
	}
	//unite the rest
	for(; myI < myEnd;) {
		const CellDesc & myCD = m_desc[myI];
		if (myCD.fetched) {
			r.uncheckedSet((uint32_t)r.m_desc.size(), m_idx[myI].idx);
		}
		else {
			r.m_idx[r.m_desc.size()].idxPtr = m_idx[myI].idxPtr;
		}
		r.m_desc.push_back(myCD);
		++myI;
		continue;
	}

	for(; oI < oEnd;) {
		const CellDesc & oCD = o.m_desc[oI];
		if (oCD.fetched) {
			r.uncheckedSet((uint32_t)r.m_desc.size(), o.m_idx[oI].idx);
		}
		else {
			r.m_idx[r.m_desc.size()].idxPtr = o.m_idx[oI].idxPtr;
		}
		r.m_desc.push_back(oCD);
		++oI;
		continue;
	}
	r.m_idx = (IndexDesc*) realloc(r.m_idx, r.m_desc.size()*sizeof(IndexDesc));
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(r.m_desc.size(), (m_desc.size() + o.m_desc.size()));
	return rPtr;
}

CellQueryResult * CellQueryResult::allToFull() const {
	CellQueryResult * rPtr = new CellQueryResult(m_gh, m_idxStore, m_flags);
	
	uint32_t totalSize = cellCount();
	rPtr->m_desc.reserve(totalSize);
	rPtr->m_desc = m_desc;
	rPtr->m_idx = (IndexDesc*) ::malloc(totalSize * sizeof(IndexDesc));
	for(CellDesc & cd : rPtr->m_desc) {
		cd.fetched = 0;
		cd.fullMatch = 1;
	}
	return rPtr;
}

CellQueryResult * CellQueryResult::removeEmpty(uint32_t emptyCellCount) const {
	detail::CellQueryResult * rPtr = new detail::CellQueryResult(m_gh, m_idxStore, m_flags);
	detail::CellQueryResult & r = *rPtr;
	
	if (emptyCellCount > cellCount()) {
		emptyCellCount = 0;
	}
	
	if (!emptyCellCount) {
		for(uint32_t i(0), s(cellCount()); i < s; ++i) {
			const CellDesc & cd = m_desc[i];
			if ((!cd.fullMatch && !cd.fetched && idxId(i) == 0) || (cd.fetched && idxSize(i) == 0)) {
				++emptyCellCount;
			}
		}
	}
	
	uint32_t realCellCount = cellCount() - emptyCellCount;
	r.m_desc.reserve(realCellCount);
	r.m_idx = (detail::CellQueryResult::IndexDesc*) ::malloc(sizeof(sserialize::detail::CellQueryResult::IndexDesc) * realCellCount);
	
	for(uint32_t i(0), s(cellCount()); i < s; ++i) {
		const detail::CellQueryResult::CellDesc & cd = m_desc[i];
		if (cd.fullMatch) {
			r.m_desc.emplace_back(cd);
		}
		else if (!cd.fetched && m_idx[i].idxPtr != 0) { //remove empty indices
			r.m_idx[r.m_desc.size()].idxPtr = m_idx[i].idxPtr;//this has to come first due to the usage of m_desc.size()
			r.m_desc.emplace_back(cd);
		}
		else if (cd.fetched) {
			r.uncheckedSet((uint32_t)r.m_desc.size(), m_idx[i].idx);
			r.m_desc.emplace_back(cd);//this has to come first due to the usage of m_desc.size()
		}
	}
	return rPtr;
}

bool CellQueryResult::selfCheck() {
	uint32_t cellCount = m_gh.cellSize();
	for(const CellDesc & d : m_desc) {
		if (d.cellId >= cellCount) {
			return false;
		}
	}
	return true;
}

ItemIndex CellQueryResult::cells() const {
	std::vector<uint32_t> tmp(m_desc.size(), 0);
	for(const CellDesc & cd : m_desc) {
		tmp.emplace_back(cd.cellId);
	}
	return sserialize::ItemIndex(tmp);
}

}}//end namespace