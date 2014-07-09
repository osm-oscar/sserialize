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
	return m_d->idx(m_pos).size();
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

CellQueryResult::CellQueryResult(const sserialize::ItemIndex& fullMatches, const sserialize::ItemIndex& partialMatches, const sserialize::CompactUintArray::const_iterator& partialMatchesItemsPtrBegin, const sserialize::CellQueryResult::GeoHierarchy& gh, const sserialize::CellQueryResult::ItemIndexStore& idxStore) :
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore))
{}

CellQueryResult::~CellQueryResult() {}

CellQueryResult::CellQueryResult(const CellQueryResult & other) : m_priv(other.m_priv) {}

CellQueryResult & CellQueryResult::operator=(const CellQueryResult & other) {
	m_priv = other.m_priv;
	return *this;
}

uint32_t CellQueryResult::cellCount() const {
	return m_priv->cellCount();
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
	return sserialize::treeReduce<const_iterator, sserialize::ItemIndex>(cbegin(), cend(), [](const sserialize::ItemIndex & a, const sserialize::ItemIndex & b) { return a + b; } );
}

}