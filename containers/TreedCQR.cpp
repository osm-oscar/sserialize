#include <sserialize/containers/TreedCQR.h>
#include <sserialize/containers/TreedCQRImp.h>

namespace sserialize {

TreedCellQueryResult::TreedCellQueryResult(detail::TreedCellQueryResult::TreedCQRImp * priv) :
m_priv(priv)
{}

TreedCellQueryResult::TreedCellQueryResult() :
m_priv(new detail::TreedCellQueryResult::TreedCQRImp())
{}

TreedCellQueryResult::TreedCellQueryResult(const ItemIndex & fullMatches, const GeoHierarchy & gh, const ItemIndexStore & idxStore) : 
m_priv( new detail::TreedCellQueryResult::TreedCQRImp(fullMatches, gh, idxStore) )
{}


TreedCellQueryResult::TreedCellQueryResult(uint32_t cellId, uint32_t cellIdxId, const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_priv( new detail::TreedCellQueryResult::TreedCQRImp(cellId, cellIdxId, gh, idxStore) )
{}

TreedCellQueryResult::TreedCellQueryResult(const sserialize::ItemIndex& fullMatches, const sserialize::ItemIndex& partialMatches, const sserialize::CompactUintArray::const_iterator& partialMatchesItemsPtrBegin, const sserialize::TreedCellQueryResult::GeoHierarchy& gh, const sserialize::TreedCellQueryResult::ItemIndexStore& idxStore) :
m_priv(new detail::TreedCellQueryResult::TreedCQRImp(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore))
{}

TreedCellQueryResult::~TreedCellQueryResult() {}

TreedCellQueryResult::TreedCellQueryResult(const TreedCellQueryResult & other) : m_priv(other.m_priv) {}

TreedCellQueryResult & TreedCellQueryResult::operator=(const TreedCellQueryResult & other) {
	m_priv = other.m_priv;
	return *this;
}

const Static::spatial::GeoHierarchy& TreedCellQueryResult::geoHierarchy() const {
	return m_priv->geoHierarchy();
}

uint32_t TreedCellQueryResult::cellCount() const {
	return m_priv->cellCount();
}

sserialize::ItemIndex::Types TreedCellQueryResult::defaultIndexType() const {
	return m_priv->defaultIndexType();
}

bool TreedCellQueryResult::fullMatch(uint32_t pos) const {
	return m_priv->fullMatch(pos);
}

TreedCellQueryResult TreedCellQueryResult::operator/(const sserialize::TreedCellQueryResult& o) const {
	return TreedCellQueryResult(m_priv->intersect(o.m_priv.priv()));
}

TreedCellQueryResult TreedCellQueryResult::operator+(const sserialize::TreedCellQueryResult & o) const {
	return TreedCellQueryResult(m_priv->unite(o.m_priv.priv()));
}

TreedCellQueryResult TreedCellQueryResult::operator-(const TreedCellQueryResult & o) const {
	return TreedCellQueryResult(m_priv->diff(o.m_priv.priv()));
}

TreedCellQueryResult TreedCellQueryResult::operator^(const TreedCellQueryResult & o) const {
	return TreedCellQueryResult(m_priv->symDiff(o.m_priv.priv()));
}

void TreedCellQueryResult::dump(std::ostream & out) const {
	out << "TreedCQR<" << cellCount() << ">";
	if (!cellCount()) {
		out << "{}";
		return;
	}
	char sep = '{';
	out << " }";
	return;
}

void TreedCellQueryResult::dump() const {
	this->dump(std::cout);
	std::cout << std::endl;
}

std::ostream& operator<<(std::ostream& out, const TreedCellQueryResult& src) {
	src.dump(out);
	return out;
}

}