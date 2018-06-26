#include <sserialize/spatial/CellQueryResultPrivate.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/mt/ThreadPool.h>

namespace sserialize {
namespace detail {

CellQueryResult::CellQueryResult() :
m_flags(sserialize::CellQueryResult::FF_EMPTY),
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
m_flags(flags),
m_idx(0)
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

	SSERIALIZE_EXPENSIVE_ASSERT(selfCheck());
}

CellQueryResult::CellQueryResult(
	const ItemIndex & fmIdx,
	const GeoHierarchy & gh,
	const ItemIndexStore & idxStore,
	int flags)
:
m_gh(gh),
m_idxStore(idxStore),
m_flags(flags),
m_idx(0)
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend());

	uint32_t totalSize = fmIdx.size();
	m_desc.reserve(totalSize);
	m_idx = (IndexDesc*) malloc(totalSize * sizeof(IndexDesc));
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.push_back( CellDesc(1, 0, *fmIt) );
	}
	SSERIALIZE_EXPENSIVE_ASSERT(selfCheck());
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
m_flags(flags),
m_idx(0)
{

	uint32_t totalSize = 1;
	m_desc.reserve(totalSize);
	m_idx = (IndexDesc*) malloc(totalSize * sizeof(IndexDesc));
	IndexDesc * idxPtr = m_idx;
	m_desc.push_back( CellDesc((fullMatch ? 1 : 0), 0, cellId) );
	if (!fullMatch) {
		idxPtr[0].idxPtr = cellIdxId;
	}
	SSERIALIZE_EXPENSIVE_ASSERT(selfCheck());
}

CellQueryResult::CellQueryResult(
	const GeoHierarchy & gh,
	const ItemIndexStore & idxStore,
	int flags)
:
m_gh(gh),
m_idxStore(idxStore),
m_flags(flags),
m_idx(0)
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
	return ((first | second) == first) &&
		((first & sserialize::CellQueryResult::FF_EMPTY) != sserialize::CellQueryResult::FF_EMPTY) &&
		((second & sserialize::CellQueryResult::FF_EMPTY) != sserialize::CellQueryResult::FF_EMPTY);
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
			auto idxTypes = m_idxStore.indexTypes();
			that->uncheckedSet(pos, sserialize::ItemIndexFactory::range(0, cellSize, 1, idxTypes));
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

sserialize::ItemIndex CellQueryResult::items(uint32_t pos) const {
	if (flags() & sserialize::CellQueryResult::FF_CELL_GLOBAL_ITEM_IDS) {
		return idx(pos);
	}
	else {
			SSERIALIZE_CHEAP_ASSERT(flags() & sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS);
			const CellDesc & cd = m_desc.at(pos);
			sserialize::ItemIndex cellIdx = m_idxStore.at( m_gh.cellItemsPtr(cd.cellId) );
			std::vector<uint32_t> tmpidx;
			if (cd.fetched) {
				idx(pos).putInto(tmpidx);
			}
			else {
				idxStore().at( idxId(pos) ).putInto(tmpidx);
			}
			for(uint32_t j(0), js(tmpidx.size()); j < js; ++j) {
				uint32_t localId = tmpidx[j];
				uint32_t globalId = cellIdx.at(localId);
				tmpidx[j] = globalId;
			}
			return sserialize::ItemIndexFactory::create(tmpidx, m_idxStore.indexTypes());
	}
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

CellQueryResult * CellQueryResult::toGlobalItemIds(uint32_t threadCount) const {
	if ((flags() & sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS) == 0 ||
		(flags() & sserialize::CellQueryResult::FF_CELL_GLOBAL_ITEM_IDS) != 0) {
		throw sserialize::TypeMissMatchException("sserialize::CellQueryResult::toGlobalItemIds: No cell local item ids to process");
	}
	int newFlags = m_flags - sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS + sserialize::CellQueryResult::FF_CELL_GLOBAL_ITEM_IDS;
	
	struct State {
		uint32_t totalSize;
		const CellQueryResult * that;
		CellQueryResult * rPtr;
		std::atomic<uint32_t> i{0};
	} state;
	state.totalSize = cellCount();
	state.that = this;
	state.rPtr = new CellQueryResult(m_gh, m_idxStore, newFlags);
	state.rPtr->m_desc.resize(state.totalSize);
	state.rPtr->m_idx = (IndexDesc*) ::malloc(state.totalSize * sizeof(IndexDesc));
	
	struct Worker {
		State * state;
		std::vector<uint32_t> tmpidx;
		void operator()() {
			while (true) {
				uint32_t i = state->i.fetch_add(1, std::memory_order_relaxed);
				if (i >= state->totalSize) {
					break;
				}
				const CellDesc & cd = state->that->m_desc[i];
				if (cd.fullMatch) {
					state->rPtr->m_desc[i] = cd;
				}
				else { //TODO: speed this up with caches
					if (cd.fetched) {
						state->that->idx(i).putInto(tmpidx);
					}
					else {
						state->that->m_idxStore.at( state->that->idxId(i) ).putInto(tmpidx);
					}
					//TODO: speed up mapping
					sserialize::ItemIndex cellIdx = state->that->idxStore().at( state->that->geoHierarchy().cellItemsPtr(cd.cellId) );
					SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmpidx.size(), cellIdx.size());
					for(uint32_t j(0), js(tmpidx.size()); j < js; ++j) {
						uint32_t localId = tmpidx[j];
						uint32_t globalId = cellIdx.at(localId);
						tmpidx[j] = globalId;
					}
					SSERIALIZE_EXPENSIVE_ASSERT(std::is_sorted(tmpidx.begin(), tmpidx.end()));
					state->rPtr->m_desc[i] = CellDesc(0x0, 0x1, cd.cellId);
					state->rPtr->uncheckedSet(i, sserialize::ItemIndexFactory::create(tmpidx, state->that->m_idxStore.indexTypes()));
					tmpidx.clear();
				}
			}
		}
		Worker(State * state) : state(state) {}
		Worker(const Worker & other) : state(other.state) {}
	};
	
	if (threadCount == 0) {
		threadCount = std::thread::hardware_concurrency();
	}
	
	if (threadCount > 1) {
		sserialize::ThreadPool::execute(Worker(&state), threadCount, sserialize::ThreadPool::CopyTaskTag());
	}
	else {
		Worker w(&state);
		w();
	}
	SSERIALIZE_CHEAP_ASSERT_EQUAL(cellCount(), state.rPtr->cellCount());
	return state.rPtr;
}


CellQueryResult * CellQueryResult::toCellLocalItemIds(uint32_t threadCount) const {
	if ((flags() & sserialize::CellQueryResult::FF_CELL_GLOBAL_ITEM_IDS) == 0 ||
		(flags() & sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS) != 0) {
		throw sserialize::TypeMissMatchException("sserialize::CellQueryResult::toCellLocalItemIds: No cell global item ids to process");
	}
	int newFlags = m_flags - sserialize::CellQueryResult::FF_CELL_GLOBAL_ITEM_IDS + sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS;

	struct State {
		uint32_t totalSize;
		const CellQueryResult * that;
		CellQueryResult * rPtr;
		std::atomic<uint32_t> i{0};
	} state;
	state.totalSize = cellCount();
	state.that = this;
	state.rPtr = new CellQueryResult(m_gh, m_idxStore, newFlags);
	state.rPtr->m_desc.resize(state.totalSize);
	state.rPtr->m_idx = (IndexDesc*) ::malloc(state.totalSize * sizeof(IndexDesc));
	
	struct Worker {
		State * state;
		std::vector<uint32_t> tmpidx;
		void operator()() {
			while (true) {
				uint32_t i = state->i.fetch_add(1, std::memory_order_relaxed);
				if (i >= state->totalSize) {
					break;
				}
				const CellDesc & cd = state->that->m_desc[i];
				if (cd.fullMatch) {
					state->rPtr->m_desc[i] = cd;
				}
				else { //TODO: speed this up with caches
					sserialize::ItemIndex srcIdx;
					if (cd.fetched) {
						srcIdx = state->that->idx(i);
					}
					else {
						srcIdx =  state->that->m_idxStore.at( state->that->idxId(i) );
					}
					//TODO: speed up mapping using caches
					sserialize::ItemIndex cellIdx = state->that->m_idxStore.at( state->that->m_gh.cellItemsPtr(cd.cellId) );
					SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(srcIdx.size(), cellIdx.size());
					SSERIALIZE_EXPENSIVE_ASSERT_EQUAL((cellIdx / srcIdx), srcIdx);
					auto srcIt = srcIdx.begin();
					auto cIt = cellIdx.begin();
					for(uint32_t localId(0), i(0), s(srcIdx.size()); i < s; ++localId, ++cIt) {
						if (*srcIt == *cIt) {
							tmpidx.emplace_back(localId);
							++srcIt;
							++i;
						}
					}
					state->rPtr->m_desc[i] = CellDesc(0x0, 0x1, cd.cellId);
					state->rPtr->uncheckedSet(i, sserialize::ItemIndexFactory::create(tmpidx, state->that->m_idxStore.indexTypes()));
					tmpidx.clear();
				}
			}
		}
		Worker(State * state) : state(state) {}
		Worker(const Worker & other) : state(other.state) {}
	};
	
	if (threadCount == 0) {
		threadCount = std::thread::hardware_concurrency();
	}
	
	if (threadCount > 1) {
		sserialize::ThreadPool::execute(Worker(&state), threadCount, sserialize::ThreadPool::CopyTaskTag());
	}
	else {
		Worker w(&state);
		w();
	}
	SSERIALIZE_CHEAP_ASSERT_EQUAL(cellCount(), state.rPtr->cellCount());
	return state.rPtr;
}

bool CellQueryResult::selfCheck() {
	uint32_t ghCellCount = m_gh.cellSize();
	int64_t lastCellId = -1;
	for(uint32_t i(0), s(this->cellCount()); i < s; ++i) {
		const CellDesc & cd = m_desc[i];
		if (cd.cellId >= ghCellCount) {
			return false;
		}
		if (int64_t(cd.cellId) < lastCellId) {
			return 0;
		}
		lastCellId = cd.cellId;
		uint32_t cellItemsCount = m_gh.cellItemsCount(cd.cellId);
		if (idxSize(i) > cellItemsCount) {
			return false;
		}
		if ((flags() & sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS) && !cd.fullMatch && this->idxSize(i)) {
			sserialize::ItemIndex idx;
			if (cd.fetched) {
				idx = this->idx(i);
			}
			else {
				idx = this->idxStore().at(this->idxId(i));
			}
			if (idx.front() >= cellItemsCount || idx.back() >= cellItemsCount) {
				return false;
			}
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


sserialize::spatial::GeoRect CellQueryResult::boundary() const {
	sserialize::spatial::GeoRect result;
	for(const CellDesc & cd : m_desc) {
		result.enlarge( m_gh.cellBoundary(cd.cellId) );
	}
	return result;
}

}}//end namespace
