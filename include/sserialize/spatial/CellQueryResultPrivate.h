#ifndef SSERIALIZE_CELL_QUERY_RESULT_PRIVATE_H
#define SSERIALIZE_CELL_QUERY_RESULT_PRIVATE_H
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/spatial/CellQueryResult.h>
#include <memory>
#include <string.h>

namespace sserialize {
namespace detail {
namespace TreedCellQueryResult {
	class TreedCQRImp;
}//end namespace TreedCellQueryResult

class CellQueryResult: public RefCountObject {
public:
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
	using CellInfo = sserialize::CellQueryResult::CellInfo;
private:
	
	static_assert(sserialize::is_trivially_relocatable<sserialize::ItemIndex>::value, "ItemIndex has to be trivially relocatable");

	friend class sserialize::detail::TreedCellQueryResult::TreedCQRImp;
	
	struct CellDesc {
		CellDesc() {}
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
	CellInfo m_ci;
	sserialize::Static::ItemIndexStore m_idxStore;
	int m_flags;
	std::vector<CellDesc> m_desc;
	IndexDesc * m_idx;
private:
	void uncheckedSet(uint32_t pos, const sserialize::ItemIndex & idx);
	void uncheckedSet(uint32_t pos, sserialize::ItemIndex && idx);
	static bool flagCheck(int first, int second);
public:
	CellQueryResult();
	CellQueryResult(const CellInfo & ci, const ItemIndexStore & idxStore, int flags);
	CellQueryResult(const ItemIndex & fullMatches, const CellInfo & ci, const ItemIndexStore & idxStore, int flags);
	CellQueryResult(bool fullMatch, uint32_t cellId, const CellInfo & ci, const ItemIndexStore & idxStore, uint32_t cellIdxId, int flags);
	///@parameter fmBegin begining of the fully matched cells
	template<typename T_PMITEMSPTR_IT>
	CellQueryResult(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
					T_PMITEMSPTR_IT pmItemsBegin, const CellInfo & ci, const ItemIndexStore & idxStore, int flags);
	CellQueryResult(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
					std::vector<sserialize::ItemIndex>::const_iterator pmItemsBegin, const CellInfo & ci, const ItemIndexStore & idxStore, int flags);
	virtual ~CellQueryResult();
	inline const CellInfo & cellInfo() const { return m_ci; }
	inline const ItemIndexStore & idxStore() const { return m_idxStore; }
	inline int flags() const { return m_flags; }
	int defaultIndexTypes() const { return m_idxStore.indexTypes(); }
	uint32_t idxSize(uint32_t pos) const;
	///this is thread-safe for different pos
	const sserialize::ItemIndex & idx(uint32_t pos) const;
	///this is thread-safe for different pos
	sserialize::ItemIndex items(uint32_t pos) const;
	///This is only correct for (fullMatch() || !fetched()) and cell global item ids
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
	CellQueryResult * toGlobalItemIds(uint32_t threadCount) const;
	CellQueryResult * toCellLocalItemIds(uint32_t threadCount) const;
	bool selfCheck();
	sserialize::ItemIndex cells() const;
	sserialize::spatial::GeoRect boundary() const;
};

template<typename T_PMITEMSPTR_IT>
CellQueryResult::CellQueryResult(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
				T_PMITEMSPTR_IT pmItemsIt, const CellInfo & ci, const ItemIndexStore & idxStore, int flags) :
m_ci(ci),
m_idxStore(idxStore),
m_flags(flags)
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
