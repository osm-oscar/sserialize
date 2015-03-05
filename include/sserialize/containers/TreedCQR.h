#ifndef SSERIALIZE_TREED_CELL_QUERY_RESULT_H
#define SSERIALIZE_TREED_CELL_QUERY_RESULT_H
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/CellQueryResult.h>
#include <sserialize/utility/CompactUintArray.h>

namespace sserialize {

namespace detail {
namespace TreedCellQueryResult {
	class TreedCQRImp;
}}
namespace Static {
namespace spatial {
	class GeoHierarchy;
}
	class ItemIndexStore;
}

class TreedCellQueryResult {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
private:
	RCPtrWrapper<detail::TreedCellQueryResult::TreedCQRImp> m_priv;
private:
	TreedCellQueryResult(detail::TreedCellQueryResult::TreedCQRImp * priv);
public:
	TreedCellQueryResult();
	TreedCellQueryResult(const ItemIndex & fullMatches, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	TreedCellQueryResult(uint32_t cellId, uint32_t cellIdxId, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	TreedCellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					const sserialize::CompactUintArray::const_iterator & partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	virtual ~TreedCellQueryResult();
	TreedCellQueryResult(const TreedCellQueryResult & other);
	TreedCellQueryResult & operator=(const TreedCellQueryResult & other);
	
	const sserialize::Static::spatial::GeoHierarchy & geoHierarchy() const;
	
	uint32_t cellCount() const;
	sserialize::ItemIndex::Types defaultIndexType() const;
	bool fullMatch(uint32_t pos) const;
	
	TreedCellQueryResult operator/(const TreedCellQueryResult & other) const;
	TreedCellQueryResult operator+(const TreedCellQueryResult & other) const;
	TreedCellQueryResult operator-(const TreedCellQueryResult & other) const;
	TreedCellQueryResult operator^(const TreedCellQueryResult & other) const;
	
	sserialize::CellQueryResult toCQR() const;
	void dump(std::ostream & out) const;
	void dump() const;
};

std::ostream & operator<<(std::ostream & out, const sserialize::TreedCellQueryResult & src);

}


#endif