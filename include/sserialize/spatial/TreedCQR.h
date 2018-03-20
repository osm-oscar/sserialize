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
	TreedCellQueryResult(const GeoHierarchy & gh, const ItemIndexStore & idxStore, int flags);
	TreedCellQueryResult(const ItemIndex & fullMatches,
					const GeoHierarchy & gh,
					const ItemIndexStore & idxStore,
					int flags = CellQueryResult::FF_DEFAULTS);
	///cellIdxId is ignored if fullMatch is set
	TreedCellQueryResult(bool fullMatch,
					uint32_t cellId,
					const GeoHierarchy & gh,
					const ItemIndexStore & idxStore,
					uint32_t cellIdxId,
					int flags = CellQueryResult::FF_DEFAULTS);
	TreedCellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					const sserialize::CompactUintArray::const_iterator & partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh,
					const ItemIndexStore & idxStore,
					int flags = CellQueryResult::FF_DEFAULTS);
	TreedCellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					const sserialize::RLEStream & partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh,
					const ItemIndexStore & idxStore,
					int flags = CellQueryResult::FF_DEFAULTS);
	TreedCellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					std::vector<sserialize::ItemIndex>::const_iterator partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore,
					int flags = CellQueryResult::FF_DEFAULTS);
	explicit TreedCellQueryResult(const sserialize::CellQueryResult & cqr);
	virtual ~TreedCellQueryResult();
	TreedCellQueryResult(const TreedCellQueryResult & other);
	TreedCellQueryResult & operator=(const TreedCellQueryResult & other);
	
	const GeoHierarchy & geoHierarchy() const;
	const ItemIndexStore & idxStore() const;
	int flags() const;
	
	uint32_t cellCount() const;
	bool hasHits() const;
	uint32_t cellId(uint32_t position) const;
	int defaultIndexTypes() const;
	bool fullMatch(uint32_t pos) const;
	
	TreedCellQueryResult operator/(const TreedCellQueryResult & other) const;
	TreedCellQueryResult operator+(const TreedCellQueryResult & other) const;
	TreedCellQueryResult operator-(const TreedCellQueryResult & other) const;
	TreedCellQueryResult operator^(const TreedCellQueryResult & other) const;
	TreedCellQueryResult allToFull() const;
	
	explicit inline operator sserialize::CellQueryResult() const { return toCQR(); }
	
	sserialize::CellQueryResult toCQR(uint32_t threadCount = 1, bool keepEmpty = false) const;
	sserialize::CellQueryResult toCQR(std::function<bool(std::size_t)> progressFunction, uint32_t threadCount = 1, bool keepEmpty = false) const;
	void dump(std::ostream & out) const;
	void dump() const;
};

std::ostream & operator<<(std::ostream & out, const sserialize::TreedCellQueryResult & src);

}


#endif
