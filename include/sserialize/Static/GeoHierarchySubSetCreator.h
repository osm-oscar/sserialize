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
	typedef std::vector<uint32_t> PointerContainer;
	
	struct RegionDesc {
		RegionDesc() : parentsBegin(0xFFFFFFFF), storeId(0xFFFFFFFF) {}
		RegionDesc(uint32_t parentsBegin, uint32_t storeId) : parentsBegin(parentsBegin), storeId(storeId) {}
		uint32_t parentsBegin;
		uint32_t storeId; //0xFFFFFFFF indicates that this region is not in our hierarchy
		inline bool valid() const { return storeId != 0xFFFFFFFF; }
	};
	struct CellDesc {
		CellDesc() : parentsBegin(0xFFFFFFFF), directParentsEnd(0xFFFFFFFF), itemsCount(0xFFFFFFFF) {}
		CellDesc(uint32_t parentsBegin, uint32_t directParentsEnd, uint32_t itemsCount) :
			parentsBegin(parentsBegin), directParentsEnd(directParentsEnd), itemsCount(itemsCount) {}
		uint32_t parentsBegin;
		uint32_t directParentsEnd;
		uint32_t itemsCount;
	};
	
	struct TempRegionInfos {
		TempRegionInfos(const sserialize::Static::spatial::GeoHierarchy & gh);
		//pointers are stored in reverse order: the arrays of regionptrs are sored from highest to lowest region
		//arrays themselves are stored from lowest to highest parent
		std::vector<uint32_t> regionParentsPtrs;
		std::vector<RegionDesc> regionDesc;
		void getAncestors(uint32_t rid, std::unordered_set<uint32_t> & dest);
		GeoHierarchySubSetCreator::PointerContainer::const_iterator parentsBegin(uint32_t rid) const;
		GeoHierarchySubSetCreator::PointerContainer::const_iterator parentsEnd(uint32_t rid) const;
	};
public:
	typedef sserialize::Static::spatial::GeoHierarchy::SubSet SubSet;
private:
	std::vector<uint32_t> m_cellParentsPtrs;
	std::vector<uint32_t> m_regionParentsPtrs;
	std::vector<RegionDesc> m_regionDesc;
	std::vector<CellDesc> m_cellDesc;
private:
	template<bool SPARSE>
	SubSet::Node * createSubSet(const CellQueryResult & cqr, SubSet::Node** nodes, uint32_t size) const;
	SubSet::Node * createSubSet(const CellQueryResult & cqr, std::unordered_map<uint32_t, SubSet::Node*> & nodes) const;
private://used during construction
// 	void getAncestors(uint32_t rid, std::unordered_set<uint32_t> & dest);
	PointerContainer::const_iterator parentsBegin(uint32_t rid) const;
	PointerContainer::const_iterator parentsEnd(uint32_t rid) const;
public:
	GeoHierarchySubSetCreator();
	GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh);
	///@param filter a functor: operator()(uint32_t regionId) -> bool defining regions relevant for subsets
	template<typename TFilter>
	GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh, TFilter filter);
	~GeoHierarchySubSetCreator();
	SubSet::Node * subSet(const sserialize::CellQueryResult & cqr, bool sparse) const;
};

//This is broken! If a parent is remove from the set of parents, then the parents of that parent need to be considered

template<typename TFilter>
GeoHierarchySubSetCreator::GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh, TFilter filter)
{
	if (!gh.regionSize()) {
		return;
	}
	const sserialize::Static::spatial::GeoHierarchy::RegionPtrListType & ghRegionPtrs = gh.regionPtrs();
	const sserialize::Static::spatial::GeoHierarchy::CellPtrListType & ghCellPtrs = gh.cellPtrs();
	
	//now check regions that are added in reverse
	//We do this in reverse since regions that are higher up in the hierarchy have higher ids
	//so if we take care of a region we use the fact that all ancestors have been taken care of already
	{
		TempRegionInfos regionInfo(gh);
		std::vector<uint32_t> added, removed;
		for(int64_t i(gh.regionSize()-1); i >= 0; --i) {
			added.clear();
			removed.clear();
			
			RegionDesc & rd = regionInfo.regionDesc.at((std::size_t)i);
			rd.parentsBegin = regionInfo.regionParentsPtrs.size();

			if (!filter(i)) { //region is not part of the hierarchy
				//use storeId to indicate that a region is not part of the hierarchy
				continue;
			}
			else {
				rd.storeId = gh.region(i).storeId();
			}
			
			//first find all the parents that are part of this hierarchy and that are removed
			for(uint32_t rPIt(gh.regionParentsBegin(i)), rPEnd(gh.regionParentsEnd(i)); rPIt != rPEnd; ++rPIt) {
				uint32_t rId = ghRegionPtrs.at(rPIt);
				if (filter(rId)) {
					added.push_back(rId);
				}
				else {
					removed.push_back(rId);
				}
			}
			SSERIALIZE_NORMAL_ASSERT(sserialize::is_strong_monotone_ascending(added.begin(), added.end()));
			
			//we now have to check if we need to find a new parent for the removed parent
			//to do this, we have to find for each removed parent an ancestor that is part of our remaining parents
			//to simplify this, we first gather all ancestors from our remaining parents
			if (removed.size()) {
				std::unordered_set<uint32_t> ancestors;
				for(auto x : added) {
					regionInfo.getAncestors(x, ancestors);
				}
				
				for(uint32_t i(0); i < removed.size(); ++i) {
					uint32_t candidateParent = removed[i];
					if (ancestors.count(candidateParent)) { //already an ancestor
						continue;
					}
					if (filter(candidateParent)) { //found a valid parent
						//add as ancestor and add all of its ancestors aswell
						ancestors.insert(candidateParent);
						regionInfo.getAncestors(candidateParent, ancestors);
						added.push_back(candidateParent);
					}
					else {//add its parents into our queue
						//here we have to use the parents from the original gh since these parents may have been removed aswell
						for(uint32_t rPIt(gh.regionParentsBegin(candidateParent)), rPEnd(gh.regionParentsEnd(candidateParent)); rPIt != rPEnd; ++rPIt) {
							uint32_t rId = ghRegionPtrs.at(rPIt);
							removed.push_back(rId);
						}
					}
				}
				std::sort(added.begin(), added.end());
				SSERIALIZE_NORMAL_ASSERT(sserialize::is_strong_monotone_ascending(added.begin(), added.end()));
			}
			
			regionInfo.regionParentsPtrs.insert(regionInfo.regionParentsPtrs.end(), added.begin(), added.end());
		}
		//now move them to the real hierarchy
		SSERIALIZE_CHEAP_ASSERT(regionInfo.regionDesc.size() == gh.regionSize());
		for(uint32_t rId(0), s(regionInfo.regionDesc.size()); rId < s; ++rId) {
			m_regionDesc.emplace_back(m_regionParentsPtrs.size(), regionInfo.regionDesc.at(rId).storeId);
			m_regionParentsPtrs.insert(m_regionParentsPtrs.end(), regionInfo.parentsBegin(rId), regionInfo.parentsEnd(rId));
		}
	}
	#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	for(uint32_t rid(0), s(m_regionDesc.size()); rid != s; ++rid) {
		SSERIALIZE_NORMAL_ASSERT(m_regionDesc.at(rid).valid() || (parentsEnd(rid)-parentsBegin(rid)) == 0);
		for(PointerContainer::const_iterator it(parentsBegin(rid)), end(parentsEnd(rid)); it != end; ++it) {
			SSERIALIZE_CHEAP_ASSERT(m_regionDesc.at(*it).valid());
		}
	}
	#endif
	
	//BUG: take care of directParent
	for(uint32_t i(0), s(gh.cellSize()); i < s; ++i) {
		
		uint32_t cPIt(gh.cellParentsBegin(i));
		uint32_t cdPEnd(gh.cellDirectParentsEnd(i));
		uint32_t cPEnd(gh.cellParentsEnd(i));
		
		CellDesc cd;
		cd.parentsBegin = m_cellParentsPtrs.size();
		cd.itemsCount = gh.cellItemsCount(i);
		
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
	m_regionDesc.emplace_back(m_regionParentsPtrs.size(), 0);
	m_cellDesc.emplace_back(m_cellParentsPtrs.size(), 0, 0);

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