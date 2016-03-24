#include <sserialize/spatial/TreedCQR.h>
#include <sserialize/spatial/TreedCQRImp.h>

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


TreedCellQueryResult::TreedCellQueryResult(bool fullMatch, uint32_t cellId, const GeoHierarchy & gh, const ItemIndexStore & idxStore, uint32_t cellIdxId) :
m_priv( new detail::TreedCellQueryResult::TreedCQRImp(fullMatch, cellId, gh, idxStore, cellIdxId) )
{}

TreedCellQueryResult::TreedCellQueryResult(const sserialize::ItemIndex& fullMatches, const sserialize::ItemIndex& partialMatches, const sserialize::CompactUintArray::const_iterator& partialMatchesItemsPtrBegin, const sserialize::TreedCellQueryResult::GeoHierarchy& gh, const sserialize::TreedCellQueryResult::ItemIndexStore& idxStore) :
m_priv(new detail::TreedCellQueryResult::TreedCQRImp(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore))
{}

TreedCellQueryResult::TreedCellQueryResult(const sserialize::ItemIndex& fullMatches, const sserialize::ItemIndex& partialMatches, const sserialize::RLEStream& partialMatchesItemsPtrBegin, const sserialize::TreedCellQueryResult::GeoHierarchy& gh, const sserialize::TreedCellQueryResult::ItemIndexStore& idxStore) :
m_priv(new detail::TreedCellQueryResult::TreedCQRImp(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore))
{}

TreedCellQueryResult::TreedCellQueryResult(const sserialize::ItemIndex& fullMatches, const sserialize::ItemIndex& partialMatches, std::vector< sserialize::ItemIndex >::const_iterator partialMatchesItemsPtrBegin, const sserialize::TreedCellQueryResult::GeoHierarchy& gh, const sserialize::TreedCellQueryResult::ItemIndexStore& idxStore) :
m_priv(new detail::TreedCellQueryResult::TreedCQRImp(fullMatches, partialMatches, partialMatchesItemsPtrBegin, gh, idxStore))
{}

TreedCellQueryResult::TreedCellQueryResult(const CellQueryResult& cqr) :
m_priv(new detail::TreedCellQueryResult::TreedCQRImp(cqr))
{}

TreedCellQueryResult::~TreedCellQueryResult() {}

TreedCellQueryResult::TreedCellQueryResult(const TreedCellQueryResult & other) : m_priv(other.m_priv) {}

TreedCellQueryResult & TreedCellQueryResult::operator=(const TreedCellQueryResult & other) {
	m_priv = other.m_priv;
	return *this;
}

const TreedCellQueryResult::GeoHierarchy& TreedCellQueryResult::geoHierarchy() const {
	return m_priv->geoHierarchy();
}

const TreedCellQueryResult::ItemIndexStore& TreedCellQueryResult::idxStore() const {
	return m_priv->idxStore();
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

TreedCellQueryResult TreedCellQueryResult::allToFull() const {
	return TreedCellQueryResult(m_priv->allToFull());
}

void TreedCellQueryResult::dump(std::ostream & out) const {
	out << "TreedCQR<" << cellCount() << ">";
	if (!cellCount()) {
		out << "{}";
		return;
	}
	char sep = '{';
	for(uint32_t i(0), s(cellCount()); i < s; ++i) {
		out << sep << ' ';
		out << m_priv->cellId(i) << ":";
		if (m_priv->fullMatch(i)) {
			out << "f";
		}
		else if (m_priv->hasTree(i)) {
			out << "t";
		}
		else {
			out << "p";
		}
		sep = ',';
	}
	out << " }";
	return;
}

void TreedCellQueryResult::dump() const {
	this->dump(std::cout);
	std::cout << std::endl;
}

sserialize::CellQueryResult TreedCellQueryResult::toCQR(uint32_t threadCount) const {
	return CellQueryResult( m_priv->toCQR([](std::size_t, std::size_t) { return true; }, threadCount) );
}

sserialize::CellQueryResult TreedCellQueryResult::toCQR(std::function<bool(std::size_t)> progressFunction, uint32_t threadCount) const {
	return CellQueryResult( m_priv->toCQR([&progressFunction](std::size_t progress, std::size_t) { return progressFunction(progress); }, threadCount) );
}

std::ostream& operator<<(std::ostream& out, const TreedCellQueryResult& src) {
	src.dump(out);
	return out;
}

}