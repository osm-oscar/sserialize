#ifndef SSERIALIZE_CELL_QUERY_RESULT_PRIVATE_H
#define SSERIALIZE_CELL_QUERY_RESULT_PRIVATE_H
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <memory>
#include <string.h>

namespace sserialize {
namespace detail {
namespace TreedCellQueryResult {
	class TreedCQRImp;
}//end namespace TreedCellQueryResult

class CellQueryResult: public RefCountObject {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
private:

	friend class sserialize::detail::TreedCellQueryResult::TreedCQRImp;
	
	struct CellDesc {
		CellDesc(uint32_t inFullMatch, uint32_t inFetched, uint32_t inCellId) {
			fullMatch = inFullMatch;
			fetched = inFetched;
			cellId = inCellId;
		}
		uint32_t fullMatch:1;//about 20% slower but reduces the memory usage by a factor of 5/3
		uint32_t fetched:1;
		uint32_t cellId:30;
		///raw data in the format (cellId|fetched|fullMatch) least-significant bit to the right
		uint32_t raw() const { return (uint32_t)(cellId << 2 | fetched << 1 | fullMatch); }
	};
	
	union IndexDesc {
		ItemIndex idx;
		uint32_t idxPtr;
	};
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
	sserialize::Static::ItemIndexStore m_idxStore;
	std::vector<CellDesc> m_desc;
	IndexDesc * m_idx;
private:
	void uncheckedSet(uint32_t pos, const sserialize::ItemIndex & idx);
	CellQueryResult(const GeoHierarchy & gh, const ItemIndexStore & idxStore);
public:
	CellQueryResult();
	CellQueryResult(const ItemIndex & fullMatches, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	CellQueryResult(bool fullMatch, uint32_t cellId, const GeoHierarchy & gh, const ItemIndexStore & idxStore, uint32_t cellIdxId);
	inline const GeoHierarchy & geoHierarchy() const { return m_gh; }
	inline const ItemIndexStore & idxStore() const { return m_idxStore; }
	
	///@parameter fmBegin begining of the fully matched cells
	template<typename T_PMITEMSPTR_IT>
	CellQueryResult(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
					T_PMITEMSPTR_IT pmItemsBegin, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	CellQueryResult(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
					std::vector<sserialize::ItemIndex>::const_iterator pmItemsBegin, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	virtual ~CellQueryResult();
	sserialize::ItemIndex::Types defaultIndexType() const { return m_idxStore.indexType(); }
	uint32_t idxSize(uint32_t pos) const;
	const sserialize::ItemIndex & idx(uint32_t pos) const;
	///This is only correct for (fullMatch() || !fetched())
	uint32_t idxId(uint32_t pos) const;
	inline bool fullMatch(uint32_t pos) const { return m_desc[pos].fullMatch; }
	inline uint32_t cellId(uint32_t pos) const { return m_desc[pos].cellId;}
	inline uint32_t cellCount() const { return (uint32_t)m_desc.size();}
	uint32_t maxItems() const;
	inline bool fetched(uint32_t pos) const { return m_desc[pos].fetched; }
	inline uint32_t rawDesc(uint32_t pos) const { return m_desc[pos].raw(); }
	CellQueryResult * intersect(const CellQueryResult * other) const;
	CellQueryResult * unite(const CellQueryResult * other) const;
	CellQueryResult * diff(const CellQueryResult * other) const;
	CellQueryResult * symDiff(const CellQueryResult * other) const;
	CellQueryResult * allToFull() const;
	CellQueryResult * removeEmpty(uint32_t emptyCellCount = 0) const;
	bool selfCheck();
	sserialize::ItemIndex cells() const;
};

template<typename T_PMITEMSPTR_IT>
CellQueryResult::CellQueryResult(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
				T_PMITEMSPTR_IT pmItemsIt, const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore)
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend()), pmIt(pmIdx.cbegin()), pmEnd(pmIdx.cend());

	uint32_t totalSize = fmIdx.size() + pmIdx.size();
	m_desc.reserve(totalSize);
	m_idx = (IndexDesc*) malloc(totalSize * sizeof(IndexDesc));
	IndexDesc * idxPtr = m_idx;

	for(; fmIt != fmEnd && pmIt != pmEnd; ++idxPtr) {
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
			m_desc.emplace_back(0, 0, pCellId);
			idxPtr->idxPtr = (sserialize::Static::ItemIndexStore::IdType)*pmItemsIt;
			++pmIt;
			++pmItemsIt;
		}
	}
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.emplace_back(1, 0, *fmIt);
	}
	
	for(; pmIt != pmEnd; ++idxPtr, ++pmIt, ++pmItemsIt) {
		m_desc.emplace_back(0, 0, *pmIt);
		idxPtr->idxPtr = (sserialize::Static::ItemIndexStore::IdType)*pmItemsIt;
	}
	
	m_desc.shrink_to_fit();
	//return should stay the same since gthis is just a shrink
	m_idx = (IndexDesc*)::realloc(m_idx, m_desc.size()*sizeof(IndexDesc));

	SSERIALIZE_EXPENSIVE_ASSERT(selfCheck());
}


}}//end namespace


#endif