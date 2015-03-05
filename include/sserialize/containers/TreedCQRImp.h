#ifndef SSERIALIZE_TREED_CELL_QUERY_RESULT_PRIVATE_H
#define SSERIALIZE_TREED_CELL_QUERY_RESULT_PRIVATE_H
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <memory>
#include <string.h>

namespace sserialize {
namespace detail {
namespace TreedCellQueryResult  {

	//TODO: Implement this with a flat tree aproach without pointers which should improve the speed a lot since then its all just sequential memory access

	union FlatNode {
		typedef enum {T_INVALID=0, T_PM_LEAF=1, T_FM_LEAF=2, T_FETCHED_LEAF=3, T_INTERSECT=4, T_UNITE=5, T_DIFF=6, T_SYM_DIFF=7} Type;
		uint64_t raw;
		struct {
			uint64_t type:3;
			uint64_t value:61;
		} common;
		struct {
			uint64_t type:3;
			uint64_t padding:1;
			uint64_t childA:30;//offset to child A from the position of THIS node of the tree
			uint64_t childB:30;//offset to child B from the position of THIS node of the tree
		} opNode;
		struct {
			uint64_t type:3;
			uint64_t cellId:29;
			uint64_t pmIdxId:32;
		} pmNode;
		struct {
			uint64_t type:3;
			uint64_t cellId:29;
			uint64_t padding:32;
		} fmNode;
		struct {
			uint64_t type:3;
			uint64_t internalIdxId:61;
		} fetchedNode;
		FlatNode(Type t) {
			common.type = t;
		}
		FlatNode(Type t) {
			common.type = t;
		}
	};
	class TreeHandler {
	public:
		typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
		typedef sserialize::Static::ItemIndexStore ItemIndexStore;
	private:
		typedef enum {FT_NONE=0x0, FT_PM=0x1, FT_FM=0x2, FT_FETCHED=0x4, FT_EMPTY=0x8} FlattenResultType;
	private:
		GeoHierarchy m_gh;
		ItemIndexStore m_idxStore;
	private:
		void flattenOpNode(const FlatNode * n, uint32_t cellId, sserialize::ItemIndex & idx, FlattenResultType & frt);
		void flatten(const FlatNode * n, const ItemIndex * idces, uint32_t cellId, sserialize::ItemIndex & idx, FlattenResultType & frt);
	public:
		TreeHandler(const GeoHierarchy & gh, const ItemIndexStore & idxStore);
		//flatten the node and adjust its type accordingly, put newly created ItemIndex into destInteralIdces
		void flatten(const FlatNode * n, FlatNode & destNode, std::vector<ItemIndex> & destInternalIdces);
	};

///Base class for the TreedCQR, which is a delayed set ops CQR. It delays the set operations of cells until calling finalize() which will return a CQR
class TreedCQRImp: public RefCountObject {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
private:
	struct CellDesc {
		static constexpr uint32_t notree = 0xFFFFFFFF;
		CellDesc(uint32_t fullMatch, uint32_t cellId, uint32_t pmIdxId, uint32_t treeBegin, uint32_t treeEnd);
		CellDesc(uint32_t fullMatch, uint32_t cellId, uint32_t pmIdxId);
		CellDesc(const CellDesc & other);
		CellDesc(CellDesc && other);
		~CellDesc();
		inline uint32_t treeSize() const { return treeEnd-treeBegin; }
		inline bool hasTree() const { return treeSize(); }
		uint64_t fullMatch:1;
		uint64_t cellId:31;
		uint64_t pmIdxId:32;
		//by definition: if this is 0xFFFFFFFF then the start is invalid
		uint32_t treeBegin;
		uint32_t treeEnd; //this is needed for fast copying of trees (and alignment gives it for free)
	};
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
	sserialize::Static::ItemIndexStore m_idxStore;
	std::vector<CellDesc> m_desc;
	std::vector<FlatNode> m_trees;
private:
	TreedCQRImp(const GeoHierarchy & gh, const ItemIndexStore & idxStore);
public:
	TreedCQRImp();
	TreedCQRImp(const ItemIndex & fullMatches, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	TreedCQRImp(uint32_t cellId, uint32_t cellIdxId, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	inline const sserialize::Static::spatial::GeoHierarchy & geoHierarchy() const { return m_gh; }
	
	///@parameter fmBegin begining of the fully matched cells
	template<typename T_PMITEMSPTR_IT>
	TreedCQRImp(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
					T_PMITEMSPTR_IT pmItemsBegin, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	virtual ~TreedCQRImp();
	sserialize::ItemIndex::Types defaultIndexType() const { return m_idxStore.indexType(); }
	inline bool fullMatch(uint32_t pos) const { return m_desc[pos].fullMatch; }
	inline uint32_t cellId(uint32_t pos) const { return m_desc[pos].cellId;}
	inline uint32_t cellCount() const { return m_desc.size();}
	TreedCQRImp * intersect(const TreedCQRImp * other) const;
	TreedCQRImp * unite(const TreedCQRImp * other) const;
	TreedCQRImp * diff(const TreedCQRImp * other) const;
	TreedCQRImp * symDiff(const TreedCQRImp * other) const;
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
		if(fCellId <= pCellId) {
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