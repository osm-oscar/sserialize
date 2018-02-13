#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/spatial/CellQueryResultPrivate.h>

namespace sserialize {
namespace detail {

CellQueryResultIterator::CellQueryResultIterator(const RCPtrWrapper<detail::CellQueryResult> & cqr, uint32_t pos) :
m_d(cqr),
m_pos(pos)
{}

CellQueryResultIterator::CellQueryResultIterator(const CellQueryResultIterator & other) : 
m_d(other.m_d),
m_pos(other.m_pos)
{}

CellQueryResultIterator::~CellQueryResultIterator() {}

CellQueryResultIterator & CellQueryResultIterator::operator=(const CellQueryResultIterator & other) {
	m_d = other.m_d;
	m_pos = other.m_pos;
	return *this;
}

const sserialize::ItemIndex & CellQueryResultIterator::operator*() const {
	return m_d->idx(m_pos);
}

uint32_t CellQueryResultIterator::cellId() const {
	return m_d->cellId(m_pos);
}

bool CellQueryResultIterator::fullMatch() const {
	return m_d->fullMatch(m_pos);
}

//TODO:improve this if more information from the text-search is available
uint32_t CellQueryResultIterator::idxSize() const {
	return m_d->idxSize(m_pos);
}

uint32_t CellQueryResultIterator::idxId() const {
	return m_d->idxId(m_pos);
}

uint32_t CellQueryResultIterator::rawDesc() const {
	return m_d->rawDesc(m_pos);
}

bool CellQueryResultIterator::fetched() const {
	return m_d->fetched(m_pos);
}

CellQueryResultIterator CellQueryResultIterator::operator++(int ) {
	++m_pos;
	return CellQueryResultIterator(m_d, m_pos-1);
}

CellQueryResultIterator & CellQueryResultIterator::operator++() {
	++m_pos;
	return *this;
}

CellQueryResultIterator CellQueryResultIterator::operator+(difference_type v) const {
	SSERIALIZE_CHEAP_ASSERT(v >= 0);
	return CellQueryResultIterator(m_d, m_pos+(uint32_t)v);
}

}//end namespace detail


CellQueryResult::CellQueryResult(detail::CellQueryResult * priv) :
m_priv(priv)
{}

CellQueryResult::CellQueryResult(int flags) :
m_priv(new detail::CellQueryResult(flags))
{}

CellQueryResult::CellQueryResult(const ItemIndex & fullMatches, const GeoHierarchy & gh, const ItemIndexStore & idxStore, int flags) : 
m_priv( new detail::CellQueryResult(fullMatches, gh, idxStore, flags) )
{}


CellQueryResult::CellQueryResult(
	bool fullMatch,
	uint32_t cellId,
	const GeoHierarchy & gh,
	const ItemIndexStore & idxStore,
	uint32_t cellIdxId,
	int flags)
:
m_priv( new detail::CellQueryResult(fullMatch, cellId, gh, idxStore, cellIdxId, flags) )
{}

CellQueryResult::CellQueryResult(
	const sserialize::ItemIndex& fullMatches,
	const sserialize::ItemIndex& partialMatches,
	const sserialize::CompactUintArray::const_iterator& partialMatchesItemsPtrBegin,
	const sserialize::CellQueryResult::GeoHierarchy& gh,
	const sserialize::CellQueryResult::ItemIndexStore& idxStore,
	int flags)
:
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore, flags))
{}

CellQueryResult::CellQueryResult(
	const sserialize::ItemIndex& fullMatches,
	const sserialize::ItemIndex& partialMatches,
	const sserialize::RLEStream& partialMatchesItemsPtrBegin,
	const sserialize::CellQueryResult::GeoHierarchy& gh,
	const sserialize::CellQueryResult::ItemIndexStore& idxStore,
	int flags)
:
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore, flags))
{}

CellQueryResult::CellQueryResult(
	const sserialize::ItemIndex& fullMatches,
	const sserialize::ItemIndex& partialMatches,
	std::vector< uint32_t >::const_iterator partialMatchesItemsPtrBegin,
	const sserialize::CellQueryResult::GeoHierarchy& gh,
	const sserialize::CellQueryResult::ItemIndexStore& idxStore,
	int flags)
:
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore, flags))
{}

CellQueryResult::CellQueryResult(
	const sserialize::ItemIndex& fullMatches,
	const sserialize::ItemIndex& partialMatches,
	std::vector<sserialize::ItemIndex>::const_iterator partialMatchesIdx,
	const sserialize::CellQueryResult::GeoHierarchy& gh,
	const sserialize::CellQueryResult::ItemIndexStore& idxStore,
	int flags) :
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesIdx, gh, idxStore, flags))
{}

CellQueryResult::~CellQueryResult() {}

CellQueryResult::CellQueryResult(const CellQueryResult & other) : m_priv(other.m_priv) {}

CellQueryResult & CellQueryResult::operator=(const CellQueryResult & other) {
	m_priv = other.m_priv;
	return *this;
}

const Static::spatial::GeoHierarchy& CellQueryResult::geoHierarchy() const {
	return m_priv->geoHierarchy();
}

const CellQueryResult::ItemIndexStore& CellQueryResult::idxStore() const {
	return m_priv->idxStore();
}

int CellQueryResult::flags() const {
	return m_priv->flags();
}

uint32_t CellQueryResult::cellCount() const {
	return m_priv->cellCount();
}

uint32_t CellQueryResult::maxItems() const {
	return m_priv->maxItems();
}

sserialize::ItemIndex::Types CellQueryResult::defaultIndexType() const {
	return m_priv->defaultIndexType();
}

uint32_t CellQueryResult::idxSize(uint32_t pos) const {
	return m_priv->idxSize(pos);
}

uint32_t CellQueryResult::cellId(uint32_t pos) const {
	return m_priv->cellId(pos);
}

sserialize::ItemIndex CellQueryResult::idx(uint32_t pos) const {
	return m_priv->idx(pos);
}

sserialize::ItemIndex CellQueryResult::items(uint32_t pos) const {
	return m_priv->items(pos);
}

uint32_t CellQueryResult::idxId(uint32_t pos) const {
	return m_priv->idxId(pos);
}

bool CellQueryResult::fetched(uint32_t pos) const {
	return m_priv->fetched(pos);
}

bool CellQueryResult::fullMatch(uint32_t pos) const {
	return m_priv->fullMatch(pos);
}

CellQueryResult CellQueryResult::operator/(const sserialize::CellQueryResult& o) const {
	return CellQueryResult(m_priv->intersect(o.m_priv.priv()));
}

CellQueryResult CellQueryResult::operator+(const sserialize::CellQueryResult & o) const {
	return CellQueryResult(m_priv->unite(o.m_priv.priv()));
}

CellQueryResult CellQueryResult::operator-(const CellQueryResult & o) const {
	return CellQueryResult(m_priv->diff(o.m_priv.priv()));
}

CellQueryResult CellQueryResult::operator^(const CellQueryResult & o) const {
	return CellQueryResult(m_priv->symDiff(o.m_priv.priv()));
}

CellQueryResult CellQueryResult::allToFull() const {
	return CellQueryResult(m_priv->allToFull());
}

CellQueryResult CellQueryResult::removeEmpty() const {
	return CellQueryResult(m_priv->removeEmpty());
}

bool CellQueryResult::operator!=(const CellQueryResult& other) const {
	return !CellQueryResult::operator==(other);
}

bool CellQueryResult::operator==(const CellQueryResult& other) const {
	if (cellCount() != other.cellCount()) {
		return false;
	}
	if (flags() != other.flags()) {
		return false;
	}
	for(uint32_t i(0), s(cellCount()); i < s; ++i) {
		if (cellId(i) != other.cellId(i)) {
			return false;
		}
		if (fullMatch(i) != other.fullMatch(i)) {
			return false;
		}
		if (!fullMatch(i)) {
			if (!fetched(i) && !other.fetched(i)) {
				if (idxId(i) != other.idxId(i)) {
					return false;
				}
			}
			else if (idx(i) != other.idx(i)) {
				return false;
			}
		}
	}
	return true;
}

CellQueryResult::const_iterator CellQueryResult::begin() const {
	return const_iterator(m_priv, 0);
}

CellQueryResult::const_iterator CellQueryResult::cbegin() const {
	return const_iterator(m_priv, 0);
}

CellQueryResult::const_iterator CellQueryResult::end() const {
	return const_iterator(m_priv, m_priv->cellCount());
}

CellQueryResult::const_iterator CellQueryResult::cend() const {
	return const_iterator(m_priv, m_priv->cellCount());
}

sserialize::ItemIndex CellQueryResult::flaten(uint32_t threadCount) const {
	if ((flags() & FF_CELL_LOCAL_ITEM_IDS) != 0) {
		throw sserialize::UnimplementedFunctionException("CellQueryResult::flaten: cell local ids are not supported yet");	
	}
	auto func = [](const sserialize::ItemIndex & a, const sserialize::ItemIndex & b) -> sserialize::ItemIndex { return a + b; } ;
	return sserialize::treeReduce<const_iterator, sserialize::ItemIndex>(cbegin(), cend(), func, threadCount);
}

ItemIndex CellQueryResult::topK(uint32_t numItems) const {
	if ((flags() & FF_CELL_LOCAL_ITEM_IDS) != 0) {
		throw sserialize::UnimplementedFunctionException("CellQueryResult::topK: cell local ids are not supported yet");	
	}
	auto func = [numItems](const sserialize::ItemIndex & a, const sserialize::ItemIndex & b) -> sserialize::ItemIndex {
		return sserialize::ItemIndex::uniteK(a, b, numItems);
	};
	return sserialize::treeReduce<const_iterator, sserialize::ItemIndex>(cbegin(), cend(), func);
}

CellQueryResult CellQueryResult::toGlobalItemIds(uint32_t threadCount) const {
	SSERIALIZE_CHEAP_ASSERT(flags() & FF_CELL_LOCAL_ITEM_IDS);
	return CellQueryResult( m_priv->toGlobalItemIds(threadCount) );
}

CellQueryResult CellQueryResult::toCellLocalItemIds() const {
	SSERIALIZE_CHEAP_ASSERT(flags() & FF_CELL_GLOBAL_ITEM_IDS);
	return CellQueryResult( m_priv->toCellLocalItemIds() );
}

void CellQueryResult::dump(std::ostream & out) const {
	out << "CQR<" << cellCount();
	if (flags() & FF_CELL_LOCAL_ITEM_IDS) {
		out << ", local ids";
	}
	else {
		out << ", global ids";
	}
	out << ">";
	if (!cellCount()) {
		out << "{}";
		return;
	}
	char sep = '{';
	for(const_iterator it(cbegin()), end(cend()); it != end; ++it) {
		out << sep << ' ';
		out << it.cellId() << ":";
		if (it.fullMatch()) {
			out << "f";
		}
		else {
			out << *it;
		}
		sep = ',';
	}
	out << " }";
	return;
}

void CellQueryResult::dump() const {
	this->dump(std::cout);
	std::cout << std::endl;
}

ItemIndex CellQueryResult::cells() const {
	return m_priv->cells();
}

std::ostream& operator<<(std::ostream& out, const CellQueryResult& src) {
	src.dump(out);
	return out;
}

}
