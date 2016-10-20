#ifndef SSERIALIZE_TREED_CELL_QUERY_RESULT_PRIVATE_H
#define SSERIALIZE_TREED_CELL_QUERY_RESULT_PRIVATE_H
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/spatial/CellQueryResultPrivate.h>
#include <memory>
#include <string.h>
#include <mutex>
#include <thread>

namespace sserialize {
namespace detail {
namespace TreedCellQueryResult  {

	//TODO: Add ability to stop flattening (i.e. by call back function, could also provide a progress bar)

	union FlatNode {
		typedef enum {T_INVALID=0, T_PM_LEAF=1, T_FM_LEAF=2, T_FETCHED_LEAF=3, T_TO_FM=4, T_INTERSECT=5, T_UNITE=6, T_DIFF=7, T_SYM_DIFF=8} Type;
		uint64_t raw;
		struct {
			uint64_t type:4;
			uint64_t value:60;
		} common;
		struct {
			uint64_t type:4;
			uint64_t childB:61;//offset to child B from the position of THIS node of the tree, childA is always at position 1
		} opNode;
		struct {
			uint64_t type:4;
			uint64_t cellId:28; //around 250M Cells which is more than enough
			uint64_t pmIdxId:32;//29 bits is not enough if ItemIndexStore has no deduplication
		} pmNode;
		struct {
			uint64_t type:4;
			uint64_t cellId:28;
			uint64_t padding:32;
		} fmNode;
		struct {
			uint64_t type:4;
			uint64_t internalIdxId:60;
		} fetchedNode;
		FlatNode(Type t) {
			common.type = t;
		}
	};

///Base class for the TreedCQR, which is a delayed set ops CQR. It delays the set operations of cells until calling finalize() which will return a CQR
class TreedCQRImp: public RefCountObject {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
private:
	struct CellDesc final {
		static constexpr uint32_t notree = 0xFFFFFFFF;
		CellDesc(uint32_t fullMatch, uint32_t hasFetchedNode, uint32_t cellId, uint32_t pmIdxId, uint32_t treeBegin, uint32_t treeEnd);
		CellDesc(uint32_t fullMatch, uint32_t hasFetchedNode, uint32_t cellId, uint32_t pmIdxId);
		CellDesc(uint32_t fullMatch, uint32_t cellId, uint32_t pmIdxId);
		CellDesc(const CellDesc & other);
		~CellDesc();
		CellDesc & operator=(const CellDesc & other);
		inline uint32_t treeSize() const { return treeEnd-treeBegin; }
		inline bool hasTree() const { return treeSize(); }
		uint64_t fullMatch:1;
		uint64_t hasFetchedNode:1;
		uint64_t cellId:28;
		uint64_t pmIdxId:32;
		uint64_t dummy:2;
		//by definition: if this is 0xFFFFFFFF then the start is invalid
		uint32_t treeBegin;
		uint32_t treeEnd; //this is needed for fast copying of trees (and alignment gives it for free)
	};
	
	typedef enum {FT_NONE=0x0, FT_PM=0x1, FT_FM=0x2, FT_FETCHED=0x4, FT_EMPTY=0x8} FlattenResultType;
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
	sserialize::Static::ItemIndexStore m_idxStore;
	std::vector<CellDesc> m_desc;
	std::vector<FlatNode> m_trees;
	std::vector<ItemIndex> m_fetchedIdx;
	bool m_hasFetchedNodes;
private:
	TreedCQRImp(const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	///flattens a cell tree, @pmIdxId set iff frt == FT_PM, @idx set iff frt == FT_FETCHED
	void flattenCell(const FlatNode * n, uint32_t cellId, sserialize::ItemIndex & idx, uint32_t & pmIdxId, FlattenResultType & frt) const;
	///copy the tree of the cell at cellPos while taking care of indexes
	///also sets the appropriate m_hasFetchedNodes for dest
	static void copyTree(const TreedCQRImp & src, const CellDesc & cd, TreedCQRImp & dest);
	///copies fetched index and adjusts the tree index pointers accordingly
	///the internal index pointers point to indexes in src before and to indexes in dest afterwards
	static void copyFetchedIndices(const TreedCQRImp& src, TreedCQRImp & dest, CellDesc& destCD);
public:
	TreedCQRImp();
	TreedCQRImp(const ItemIndex & fullMatches, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	TreedCQRImp(bool fullMatch, uint32_t cellId, const GeoHierarchy & gh, const ItemIndexStore & idxStore, uint32_t cellIdxId);
	
	///@parameter fmBegin begining of the fully matched cells
	template<typename T_PMITEMSPTR_IT>
	TreedCQRImp(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
					T_PMITEMSPTR_IT pmItemsBegin, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	TreedCQRImp(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
					std::vector<sserialize::ItemIndex>::const_iterator pmItemsBegin, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	TreedCQRImp(const sserialize::CellQueryResult & cqr);
	
	virtual ~TreedCQRImp();
	
	inline const GeoHierarchy & geoHierarchy() const { return m_gh; }
	inline const ItemIndexStore & idxStore() const { return m_idxStore; }
	sserialize::ItemIndex::Types defaultIndexType() const { return m_idxStore.indexType(); }
	inline bool fullMatch(uint32_t pos) const { return m_desc[pos].fullMatch; }
	inline bool hasTree(uint32_t pos) const { return m_desc[pos].hasTree();}
	inline uint32_t cellId(uint32_t pos) const { return m_desc[pos].cellId;}
	inline uint32_t cellCount() const { return (uint32_t)m_desc.size();}
	TreedCQRImp * intersect(const TreedCQRImp * other) const;
	TreedCQRImp * unite(const TreedCQRImp * other) const;
	TreedCQRImp * diff(const TreedCQRImp * other) const;
	TreedCQRImp * symDiff(const TreedCQRImp * other) const;
	TreedCQRImp * allToFull() const;
	///@pf progress function pf(uint32_t numFinishedCells, uint32_t currentResultCellCount)
	///iff threadCount > 1 => pf must be thread-safe
	template<typename T_PROGRESS_FUNCION>
	sserialize::detail::CellQueryResult * toCQR(T_PROGRESS_FUNCION pf, uint32_t threadCount) const;
};

template<typename T_PMITEMSPTR_IT>
TreedCQRImp::TreedCQRImp(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
				T_PMITEMSPTR_IT pmItemsIt, const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore),
m_hasFetchedNodes(false)
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend()), pmIt(pmIdx.cbegin()), pmEnd(pmIdx.cend());

	uint32_t totalSize = fmIdx.size() + pmIdx.size();
	m_desc.reserve(totalSize);

	for(; fmIt != fmEnd && pmIt != pmEnd;) {
		uint32_t fCellId = *fmIt;
		uint32_t pCellId = *pmIt;
		if(fCellId <= pCellId) {
			m_desc.emplace_back(1, fCellId, 0);
			++fmIt;
		}
		else {
			m_desc.emplace_back(0, pCellId, *pmItemsIt);
			++pmIt;
			++pmItemsIt;
		}
	}
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.emplace_back(1, *fmIt, 0);
	}
	
	for(; pmIt != pmEnd; ++pmIt, ++pmItemsIt) {
		m_desc.emplace_back(0, *pmIt, *pmItemsIt);
	}
}

template<typename T_PROGRESS_FUNCION>
sserialize::detail::CellQueryResult *  TreedCQRImp::toCQR(T_PROGRESS_FUNCION pf, uint32_t threadCount) const {
	if (!threadCount) {
		threadCount = std::thread::hardware_concurrency();
	}

	if (threadCount > 1 && cellCount() > 1000) {//is 1000 good? probably not
		struct State {
			uint32_t srcPos;
			std::mutex srcPosLck;
			const TreedCQRImp * src;
			CellQueryResult * dest;
			std::atomic<uint32_t> emptyCellCount;
			State() : srcPos(0), src(0), dest(0), emptyCellCount(0) {}
		};
		
		//we use the cqr data as follows: if a cell is empty, then it should be a partial-matched cell that is not fetched an whose indexId is 0
		//since by definition the index with the id=0 is empty anyway
		struct Proc {
			State * state;
			sserialize::ItemIndex idx;
			uint32_t pmIdxId;
			FlattenResultType frt;
			Proc(State * s) : state(s) {}
			void operator()() {
				std::unique_lock<std::mutex> srcPosLck(state->srcPosLck);
				srcPosLck.unlock();
				while (true) {
					srcPosLck.lock();
					if (state->srcPos >= state->src->cellCount()) {
						return;
					}
					uint32_t myPos = state->srcPos;
					state->srcPos += 1;
					srcPosLck.unlock();
					idx = sserialize::ItemIndex();
					pmIdxId = 0;
					frt = FT_NONE;
					const CellDesc & cd = state->src->m_desc[myPos];
					if (cd.hasTree()) {
						state->src->flattenCell((&(state->src->m_trees[0]))+cd.treeBegin, cd.cellId, idx, pmIdxId, frt);
						SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(frt, FT_NONE);
						if (frt == FT_FM) {
							state->dest->m_desc.at(myPos) = detail::CellQueryResult::CellDesc(1, 0, cd.cellId);
						}
						else if (frt == FT_PM) {
							state->dest->m_desc.at(myPos) = detail::CellQueryResult::CellDesc(0, 0, cd.cellId);
							state->dest->m_idx[myPos].idxPtr = pmIdxId;
						}
						else if (frt == FT_FETCHED && idx.size()) { //frt == FT_FETCHED
							state->dest->m_desc.at(myPos) = detail::CellQueryResult::CellDesc(0, 1, cd.cellId);
							state->dest->uncheckedSet(myPos, idx);
						}
						else if (frt == FT_EMPTY) {
							state->dest->m_desc.at(myPos) = detail::CellQueryResult::CellDesc(0, 0, cd.cellId);
							state->dest->m_idx[myPos].idxPtr = 0;
							state->emptyCellCount += 1;
						}
					}
					else {
						if (!cd.fullMatch) {
							state->dest->m_idx[myPos].idxPtr = (uint32_t) cd.pmIdxId;
						}
						state->dest->m_desc.at(myPos) = detail::CellQueryResult::CellDesc(cd.fullMatch, 0, cd.cellId);
					}
				}
			}
		};
		
		State state;
		state.src = this;
		state.dest = new detail::CellQueryResult(m_gh, m_idxStore);
		state.dest->m_desc.resize(cellCount(), detail::CellQueryResult::CellDesc(0, 0, 0));
		state.dest->m_idx = (detail::CellQueryResult::IndexDesc*) ::malloc(sizeof(sserialize::detail::CellQueryResult::IndexDesc) * m_desc.size());

		std::vector<std::thread> threads;
		
		for(uint32_t i(0); i < threadCount; ++i) {
			threads.emplace_back(Proc(&state));
		}
		for(uint32_t i(0); i < threadCount; ++i) {
			threads[i].join();
		}
		//check if we have to copy the stuff
		if (state.emptyCellCount.load() == 0) {
			return state.dest;
		}
		//now copy the real stuff
		{
			detail::CellQueryResult * rPtr = new detail::CellQueryResult(m_gh, m_idxStore);
			detail::CellQueryResult & r = *rPtr;
			
			SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(state.dest->cellCount(), state.emptyCellCount);
			
			uint32_t realCellCount = state.dest->cellCount()-state.emptyCellCount;
			
			r.m_desc.reserve(realCellCount);
			r.m_idx = (detail::CellQueryResult::IndexDesc*) ::malloc(sizeof(sserialize::detail::CellQueryResult::IndexDesc) * realCellCount);
			
			for(uint32_t i(0), s(state.dest->cellCount()); i < s; ++i) {
				const detail::CellQueryResult::CellDesc & cd = state.dest->m_desc[i];
				if (cd.fullMatch) {
					r.m_desc.emplace_back(cd);
				}
				else if (!cd.fetched && state.dest->m_idx[i].idxPtr != 0) { //remove empty indices
					r.m_idx[r.m_desc.size()].idxPtr = state.dest->m_idx[i].idxPtr;//this has to come first due to the usage of m_desc.size()
					r.m_desc.emplace_back(cd);
				}
				else if (cd.fetched) {
					r.uncheckedSet((uint32_t)r.m_desc.size(), state.dest->m_idx[i].idx);
					r.m_desc.emplace_back(cd);//this has to come first due to the usage of m_desc.size()
				}
			}
			//delete old cqr
			delete state.dest;
			
			return rPtr;
		}
		
	}
	else {
	
		detail::CellQueryResult * rPtr = new detail::CellQueryResult(m_gh, m_idxStore);
		detail::CellQueryResult & r = *rPtr;
		
		r.m_desc.reserve(cellCount());
		r.m_idx = (detail::CellQueryResult::IndexDesc*) ::malloc(sizeof(sserialize::detail::CellQueryResult::IndexDesc) * m_desc.size());

		sserialize::ItemIndex idx;
		uint32_t pmIdxId;
		FlattenResultType frt = FT_NONE;

		for(std::size_t i(0), s(m_desc.size()); i < s && pf(i, m_desc.size()); ++i) {
			const CellDesc & cd = m_desc[i];
			if (cd.hasTree()) {
				flattenCell((&m_trees[0])+cd.treeBegin, cd.cellId, idx, pmIdxId, frt);
				SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(frt, FT_NONE);
				if (frt == FT_FM) {
					r.m_desc.push_back(detail::CellQueryResult::CellDesc(1, 0, cd.cellId));
				}
				else if (frt == FT_PM) {
					r.m_idx[r.m_desc.size()].idxPtr = pmIdxId;//this has to come first due to the usage of m_desc.size()
					r.m_desc.push_back(detail::CellQueryResult::CellDesc(0, 0, cd.cellId));
				}
				else if (frt == FT_FETCHED && idx.size()) { //frt == FT_FETCHED
					r.uncheckedSet((uint32_t)r.m_desc.size(), idx);//this has to come first due to the usage of m_desc.size()
					r.m_desc.push_back(detail::CellQueryResult::CellDesc(0, 1, cd.cellId));
				}
				//frt == FT_EMPTY
			}
			else {
				if (!cd.fullMatch) {
					r.m_idx[r.m_desc.size()].idxPtr = cd.pmIdxId;//this has to come first due to the usage of m_desc.size()
				}
				r.m_desc.push_back(detail::CellQueryResult::CellDesc(cd.fullMatch, 0, cd.cellId));
			}
		}
		r.m_desc.shrink_to_fit();
		r.m_idx = (detail::CellQueryResult::IndexDesc*) realloc(r.m_idx, r.m_desc.size()*sizeof(detail::CellQueryResult::IndexDesc));
		return rPtr;
	}
}

}}}//end namespace


#endif