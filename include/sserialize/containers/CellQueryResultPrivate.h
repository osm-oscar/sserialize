#ifndef SSERIALIZE_CELL_QUERY_RESULT_PRIVATE_H
#define SSERIALIZE_CELL_QUERY_RESULT_PRIVATE_H
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <memory>
#include <string.h>

namespace sserialize {
namespace detail {

class CellQueryResult: public RefCountObject {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
private:

	friend class const_iterator;
	
	struct CellDesc {
		CellDesc(uint32_t inFullMatch, uint32_t inFetched, uint32_t inCellId) {
			fullMatch = inFullMatch;
			fetched = inFetched;
			cellId = inCellId;
		}
		uint32_t fullMatch:1;
		uint32_t fetched:1;
		uint32_t cellId:30;
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
	const sserialize::ItemIndex & idx(uint32_t pos) const;
	
public:
	CellQueryResult();
	///@parameter fmBegin begining of the fully matched cells
	template<typename T_FM_IT, typename T_PM_IT, typename T_PMITEMSPTR_IT>
	CellQueryResult(T_FM_IT fmBegin, const T_FM_IT & fmEnd,
					T_PM_IT pmBegin, const T_PM_IT & pmEnd,
					T_PMITEMSPTR_IT pmItemsBegin, const T_PMITEMSPTR_IT & pmItemsEnd,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	virtual ~CellQueryResult();
	
	CellQueryResult * intersect(const CellQueryResult * other) const;
	CellQueryResult * unite(const CellQueryResult * other) const;
	CellQueryResult * diff(const CellQueryResult * other) const;
	CellQueryResult * symDiff(const CellQueryResult * other) const;
};

template<typename T_FM_IT, typename T_PM_IT, typename T_PMITEMSPTR_IT>
CellQueryResult::CellQueryResult(T_FM_IT fmIt, const T_FM_IT & fmEnd,
				T_PM_IT pmIt, const T_PM_IT & pmEnd,
				T_PMITEMSPTR_IT pmItemsIt, const T_PMITEMSPTR_IT & pmItemsEnd,
				const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore)
{

	uint32_t totalSize = (fmEnd-fmIt) + (pmEnd-pmIt);
	m_desc.reserve(totalSize);
	m_idx = (IndexDesc*) malloc(totalSize * sizeof(IndexDesc));
	IndexDesc * idxPtr = m_idx;

	for(; fmIt != fmEnd && pmIt != pmEnd; ++idxPtr) {
		uint32_t fCellId = *fmIt;
		uint32_t pCellId = *pmIt;
		if(fCellId < pCellId) {
			m_desc.push_back(CellDesc(1, 0, fCellId));
			++fmIt;
		}
		else {
			m_desc.push_back(CellDesc(0, 0, pCellId));
			idxPtr->idxPtr = *pmItemsIt;
			++pmIt;
			++pmItemsIt;
		}
	}
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.push_back( CellDesc(1, 0, *fmIt) );
	}
	
	for(; pmIt != pmEnd; ++idxPtr, ++pmIt, ++pmItemsIt) {
		m_desc.push_back(CellDesc(0, 0, *pmIt));
		idxPtr->idxPtr = *pmItemsIt;
	}

}


}}//end namespace


#endif