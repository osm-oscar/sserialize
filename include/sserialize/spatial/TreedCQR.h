#ifndef SSERIALIZE_TREED_CELL_QUERY_RESULT_H
#define SSERIALIZE_TREED_CELL_QUERY_RESULT_H
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/containers/CompactUintArray.h>
#include <functional>

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

//This is a tree based implementation of the CellQueryResult. Treed from (to) tree. Verb form of tree, 

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
	///cellIdxId is ignored if fullMatch is set
	TreedCellQueryResult(bool fullMatch, uint32_t cellId, const GeoHierarchy & gh, const ItemIndexStore & idxStore, uint32_t cellIdxId);
	TreedCellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					const sserialize::CompactUintArray::const_iterator & partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	TreedCellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					const sserialize::RLEStream & partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	virtual ~TreedCellQueryResult();
	TreedCellQueryResult(const TreedCellQueryResult & other);
	TreedCellQueryResult & operator=(const TreedCellQueryResult & other);
	
	const GeoHierarchy & geoHierarchy() const;
	const ItemIndexStore & idxStore() const;
	
	uint32_t cellCount() const;
	sserialize::ItemIndex::Types defaultIndexType() const;
	bool fullMatch(uint32_t pos) const;
	
	TreedCellQueryResult operator/(const TreedCellQueryResult & other) const;
	TreedCellQueryResult operator+(const TreedCellQueryResult & other) const;
	TreedCellQueryResult operator-(const TreedCellQueryResult & other) const;
	TreedCellQueryResult operator^(const TreedCellQueryResult & other) const;
	TreedCellQueryResult allToFull() const;
	
	sserialize::CellQueryResult toCQR() const;
	sserialize::CellQueryResult toCQR(std::function<bool(std::size_t)> progressFunction) const;
	void dump(std::ostream & out) const;
	void dump() const;
};

std::ostream & operator<<(std::ostream & out, const sserialize::TreedCellQueryResult & src);

}


#endif