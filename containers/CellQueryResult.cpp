#include <sserialize/containers/CellQueryResult.h>
#include <sserialize/containers/CellQueryResultPrivate.h>

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

CellQueryResultIterator CellQueryResultIterator::operator+(differnce_type v) const {
	return CellQueryResultIterator(m_d, m_pos+v);
}

}//end namespace detail


CellQueryResult::CellQueryResult(detail::CellQueryResult * priv) :
m_priv(priv)
{}

CellQueryResult::CellQueryResult() :
m_priv(new detail::CellQueryResult())
{}

CellQueryResult::CellQueryResult(const ItemIndex & fullMatches, const GeoHierarchy & gh, const ItemIndexStore & idxStore) : 
m_priv( new detail::CellQueryResult(fullMatches, gh, idxStore) )
{}


CellQueryResult::CellQueryResult(bool fullMatch, uint32_t cellId, const GeoHierarchy & gh, const ItemIndexStore & idxStore, uint32_t cellIdxId) :
m_priv( new detail::CellQueryResult(fullMatch, cellId, gh, idxStore, cellIdxId) )
{}

CellQueryResult::CellQueryResult(const sserialize::ItemIndex& fullMatches, const sserialize::ItemIndex& partialMatches, const sserialize::CompactUintArray::const_iterator& partialMatchesItemsPtrBegin, const sserialize::CellQueryResult::GeoHierarchy& gh, const sserialize::CellQueryResult::ItemIndexStore& idxStore) :
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore))
{}

CellQueryResult::CellQueryResult(const sserialize::ItemIndex& fullMatches, const sserialize::ItemIndex& partialMatches, const sserialize::RLEStream& partialMatchesItemsPtrBegin, const sserialize::CellQueryResult::GeoHierarchy& gh, const sserialize::CellQueryResult::ItemIndexStore& idxStore) :
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore))
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

uint32_t CellQueryResult::cellCount() const {
	return m_priv->cellCount();
}

sserialize::ItemIndex::Types CellQueryResult::defaultIndexType() const {
	return m_priv->defaultIndexType();
}

uint32_t CellQueryResult::idxSize(uint32_t pos) const {
	return m_priv->idxSize(pos);
}

sserialize::ItemIndex CellQueryResult::idx(uint32_t pos) const {
	return m_priv->idx(pos);
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

sserialize::ItemIndex CellQueryResult::flaten() const {
	auto func = [](const sserialize::ItemIndex & a, const sserialize::ItemIndex & b) -> sserialize::ItemIndex { return a + b; } ;
	return sserialize::treeReduce<const_iterator, sserialize::ItemIndex>(cbegin(), cend(), func);
}

ItemIndex CellQueryResult::topK(uint32_t numItems) const {
	auto func = [numItems](const sserialize::ItemIndex & a, const sserialize::ItemIndex & b) -> sserialize::ItemIndex { return sserialize::ItemIndex::uniteK(a, b, numItems); };
	return sserialize::treeReduce<const_iterator, sserialize::ItemIndex>(cbegin(), cend(), func);
}


void CellQueryResult::dump(std::ostream & out) const {
	out << "CQR<" << cellCount() << ">";
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

std::ostream& operator<<(std::ostream& out, const CellQueryResult& src) {
	src.dump(out);
	return out;
}


}