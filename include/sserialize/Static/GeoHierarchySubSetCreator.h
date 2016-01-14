#ifndef SSERIALIZE_GEO_HIERARCHY_SUB_SET_CREATOR_H
#define SSERIALIZE_GEO_HIERARCHY_SUB_SET_CREATOR_H
#include <sserialize/Static/GeoHierarchy.h>

//TODO: sparse hiearchy support:
//if only direct parents are available, then to calculate aproximate counts,
//one has to mark the upward graph and sum it up

namespace sserialize {
namespace spatial {
namespace detail {

class GeoHierarchySubSetCreator: public RefCountObject {
private:
	struct RegionDesc {
		RegionDesc() : parentsBegin(0xFFFFFFFF), storeId(0xFFFFFFFF) {}
		RegionDesc(uint32_t parentsBegin, uint32_t storeId) : parentsBegin(parentsBegin), storeId(storeId) {}
		uint32_t parentsBegin;
		uint32_t storeId;
	};
	struct CellDesc {
		CellDesc() : parentsBegin(0xFFFFFFFF), directParentsEnd(0xFFFFFFFF), itemsCount(0xFFFFFFFF) {}
		CellDesc(uint32_t parentsBegin, uint32_t directParentsEnd, uint32_t itemsCount) :
			parentsBegin(parentsBegin), directParentsEnd(directParentsEnd), itemsCount(itemsCount) {}
		uint32_t parentsBegin;
		uint32_t directParentsEnd;
		uint32_t itemsCount;
	};
public:
	typedef sserialize::Static::spatial::GeoHierarchy::SubSet SubSet;
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
	std::vector<uint32_t> m_cellParentsPtrs;
	std::vector<uint32_t> m_regionParentsPtrs;
	std::vector<RegionDesc> m_regionDesc;
	std::vector<CellDesc> m_cellDesc;
	sserialize::Static::ItemIndexStore m_idxStore;
private:
	template<bool SPARSE>
	SubSet::Node * createSubSet(const CellQueryResult & cqr, SubSet::Node** nodes, uint32_t size) const;
	SubSet::Node * createSubSet(const CellQueryResult & cqr, std::unordered_map<uint32_t, SubSet::Node*> & nodes) const;
public:
	GeoHierarchySubSetCreator();
	GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh);
	///@param filter a functor: operator()(uint32_t regionId) -> bool defining regions relevant for subsets
	template<typename TFilter>
	GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh, TFilter filter);
	~GeoHierarchySubSetCreator();
	SubSet::Node * subSet(const sserialize::CellQueryResult & cqr, bool sparse) const;
};

template<typename TFilter>
GeoHierarchySubSetCreator::GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh, TFilter filter) :
m_gh(gh)
{
	m_cellParentsPtrs.reserve(m_gh.cellPtrsSize());
	m_regionParentsPtrs.reserve(m_gh.regionPtrSize());
	m_regionDesc.reserve(m_gh.regionSize());
	m_cellDesc.reserve(m_gh.cellSize());
	
	const sserialize::Static::spatial::GeoHierarchy::RegionPtrListType & ghRegionPtrs = m_gh.regionPtrs();
	const sserialize::Static::spatial::GeoHierarchy::CellPtrListType & ghCellPtrs = m_gh.cellPtrs();
	
	for(uint32_t i(0), s(m_gh.regionSize()); i < s; ++i) {
		
		m_regionDesc.push_back( RegionDesc( m_regionParentsPtrs.size(), m_gh.region(i).storeId()) );
		
		for(uint32_t rPIt(m_gh.regionParentsBegin(i)), rPEnd(m_gh.regionParentsEnd(i)); rPIt != rPEnd; ++rPIt) {
			uint32_t rId = ghRegionPtrs.at(rPIt);
			if (filter(rId)) {
				m_regionParentsPtrs.push_back(rId);
			}
		}
	}
	
	for(uint32_t i(0), s(m_gh.cellSize()); i < s; ++i) {
		
		uint32_t cPIt(m_gh.cellParentsBegin(i));
		uint32_t cdPEnd(m_gh.cellDirectParentsEnd(i));
		uint32_t cPEnd(m_gh.cellParentsEnd(i));
		
		CellDesc cd;
		cd.parentsBegin = m_cellParentsPtrs.size();
		cd.itemsCount = m_gh.cellItemsCount(i);
		
		for(; cPIt != cdPEnd; ++cPIt) {
			uint32_t rId = ghCellPtrs.at(cPIt);
			if (filter(rId)) {
				m_cellParentsPtrs.push_back( rId );
			}
		}
		cd.directParentsEnd = m_cellParentsPtrs.size();
		for(cPIt = cdPEnd; cPIt != cPEnd; ++cPIt) {
			uint32_t rId = ghCellPtrs.at(cPIt);
			if (filter(rId)) {
				m_cellParentsPtrs.push_back( rId );
			}
		}
		m_cellDesc.push_back(cd);
	}
	//dummy end regions
	m_regionDesc.push_back( RegionDesc( m_regionParentsPtrs.size(), 0) );
	m_cellDesc.push_back( CellDesc( m_cellParentsPtrs.size(), 0, 0) );

	m_cellParentsPtrs.shrink_to_fit();
	m_regionParentsPtrs.shrink_to_fit();
	m_regionDesc.shrink_to_fit();
	m_cellDesc.shrink_to_fit();
}

template<bool SPARSE>
sserialize::Static::spatial::GeoHierarchy::SubSet::Node *
GeoHierarchySubSetCreator::createSubSet(const CellQueryResult & cqr, SubSet::Node* *nodes, uint32_t size) const {
	SubSet::Node * rootNode = new SubSet::Node(sserialize::Static::spatial::GeoHierarchy::npos, 0);

	const uint32_t * cPPtrsBegin = &(m_cellParentsPtrs[0]);
	const uint32_t * rPPtrsBegin = &(m_regionParentsPtrs[0]);

	for(CellQueryResult::const_iterator it(cqr.cbegin()), end(cqr.cend()); it != end; ++it) {
		uint32_t cellId = it.cellId();
		const CellDesc & cellDesc = m_cellDesc[cellId];
		uint32_t itemsInCell;
		
		itemsInCell = (it.fullMatch() ? cellDesc.itemsCount : it.idxSize());
		if (!SPARSE) {
			rootNode->maxItemsSize() += itemsInCell;
			rootNode->cellPositions().push_back(it.pos());
		}
		
		const uint32_t * cPIt = cPPtrsBegin + cellDesc.parentsBegin;
		const uint32_t * cPEnd;
		if (SPARSE) {
			cPEnd =  cPPtrsBegin+cellDesc.directParentsEnd;
		}
		else {
			cPEnd = cPPtrsBegin+m_cellDesc[cellId+1].parentsBegin;
		}
		for(; cPIt != cPEnd; ++cPIt) {
			uint32_t cP = *cPIt;
			SubSet::Node * n;
			if (!nodes[cP]) {
				n = new SubSet::Node(cP, 0);
				nodes[cP] = n;
			}
			else {
				n = nodes[cP];
			}
			n->maxItemsSize() += itemsInCell;
			n->cellPositions().push_back(it.pos());
		}
	}
	
	SubSet::Node* * end = nodes+size;
	for(SubSet::Node* * it(nodes); it != end; ++it) {
		if (*it) {
			uint32_t regionId = it-nodes;
			const uint32_t * rPIt = rPPtrsBegin + m_regionDesc[regionId].parentsBegin;
			const uint32_t * rPEnd = rPPtrsBegin + m_regionDesc[regionId+1].parentsBegin;
			if (rPIt != rPEnd) {
				for(; rPIt != rPEnd; ++rPIt) {
					uint32_t rp = *rPIt;
					if (SPARSE) {
						if (!nodes[rp]) { //sparse SubSet doesnt push parent ptrs upwards
							assert(rp > regionId); //otherwise we would not take care of the newly created region
							nodes[rp] = new SubSet::Node(rp, 0);
						}
						nodes[rp]->push_back(*it);
						nodes[rp]->maxItemsSize() += (*it)->maxItemsSize(); //approximate for item size
					}
					else {
						nodes[rp]->push_back(*it);
					}
				}
			}
			else {//region has no parent -> put into the root-region
				rootNode->push_back(*it);
				if (SPARSE) {
					rootNode->maxItemsSize() += (*it)->maxItemsSize(); //approximate for item size
				}
			}
		}
	}
	return rootNode;
}

}//end namespace detail

class GeoHierarchySubSetCreator {
private:
	RCPtrWrapper<detail::GeoHierarchySubSetCreator> m_ghs;
public:
	GeoHierarchySubSetCreator() {}
	///@param filter a functor: operator()(uint32_t regionId) -> bool defining regions relevant for subsets
	template<typename TFilter>
	GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh, TFilter filter) :
	m_ghs(new detail::GeoHierarchySubSetCreator(gh, filter))
	{}
	GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh) : 
	m_ghs(new detail::GeoHierarchySubSetCreator(gh))
	{}
	~GeoHierarchySubSetCreator() {}
	inline sserialize::Static::spatial::GeoHierarchy::SubSet subSet(const sserialize::CellQueryResult & cqr, bool sparse) const {
		return sserialize::Static::spatial::GeoHierarchy::SubSet(m_ghs->subSet(cqr, sparse), cqr, sparse);
	}
};


}}//end namespace

#endif