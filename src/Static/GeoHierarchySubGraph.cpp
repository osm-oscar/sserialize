#include <sserialize/Static/GeoHierarchySubGraph.h>

namespace sserialize {
namespace spatial {
namespace detail {

PassThroughGeoHierarchySubGraph::PassThroughGeoHierarchySubGraph(const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore)
{}

PassThroughGeoHierarchySubGraph::~PassThroughGeoHierarchySubGraph() {}

interface::GeoHierarchySubGraph::SubSet
PassThroughGeoHierarchySubGraph::subSet(const sserialize::CellQueryResult& cqr, bool sparse) const {
	return m_gh.subSet(cqr, sparse);
}

sserialize::ItemIndex
PassThroughGeoHierarchySubGraph::regionExclusiveCells(uint32_t regionId) const {
	return m_idxStore.at(m_gh.regionExclusiveCellIdxPtr(regionId));
}

GeoHierarchySubGraph::TempRegionInfos::TempRegionInfos(const sserialize::Static::spatial::GeoHierarchy & gh) :
regionDesc(gh.regionSize())
{}

void GeoHierarchySubGraph::TempRegionInfos::getAncestors(uint32_t rid, std::unordered_set<uint32_t> & dest) {
	SSERIALIZE_CHEAP_ASSERT(regionDesc.at(rid).valid());
	
	SSERIALIZE_CHEAP_ASSERT(rid < regionDesc.size());
	for(PointerContainer::const_iterator it(parentsBegin(rid)), end(parentsEnd(rid)); it != end; ++it) {
		getAncestors(*it, dest);
		dest.insert(*it);
	}
}

GeoHierarchySubGraph::PointerContainer::const_iterator 
GeoHierarchySubGraph::TempRegionInfos::parentsBegin(uint32_t rid) const {	
	uint32_t parentsBegin = regionDesc.at(rid).parentsBegin;
	SSERIALIZE_CHEAP_ASSERT(parentsBegin <= regionParentsPtrs.size());
	return regionParentsPtrs.begin()+parentsBegin;
}

GeoHierarchySubGraph::PointerContainer::const_iterator
GeoHierarchySubGraph::TempRegionInfos::parentsEnd(uint32_t rid) const {
	uint32_t parentsEnd = (rid == 0 ? regionParentsPtrs.size() : regionDesc.at(rid-1).parentsBegin);
	SSERIALIZE_CHEAP_ASSERT(parentsEnd <= regionParentsPtrs.size());
	return regionParentsPtrs.begin()+parentsEnd;
}

GeoHierarchySubGraph::GeoHierarchySubGraph() {}

GeoHierarchySubGraph::GeoHierarchySubGraph(const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
GeoHierarchySubGraph(gh, idxStore, [](uint32_t) { return true; })
{}

GeoHierarchySubGraph::~GeoHierarchySubGraph() {}

sserialize::Static::spatial::GeoHierarchy::SubSet
GeoHierarchySubGraph::subSet(const sserialize::CellQueryResult& cqr, bool sparse) const {
	SubSet::Node * rootNode = 0;
	if (cqr.cellCount() > m_cellDesc.size()*0.5 || sparse) {
		std::vector<SubSet::Node*> nodes(m_regionDesc.size()+1, 0);
		if (sparse) {
			rootNode = createSubSet<true>(cqr, &(nodes[0]), nodes.size());
		}
		else {
			rootNode = createSubSet<false>(cqr, &(nodes[0]), nodes.size());
		}
	}
	else {
		std::unordered_map<uint32_t, SubSet::Node*> nodes;
		rootNode = createSubSet(cqr, nodes);
	}
	return SubSet(rootNode, cqr, sparse);
}

sserialize::ItemIndex
GeoHierarchySubGraph::regionExclusiveCells(uint32_t regionId) const {
	if (regionId < m_rec.size()) {
		return m_rec.at(regionId);
	}
	return sserialize::ItemIndex();
}

sserialize::Static::spatial::GeoHierarchy::SubSet::Node *
GeoHierarchySubGraph::createSubSet(const CellQueryResult & cqr, std::unordered_map<uint32_t, SubSet::Node*> & nodes) const {
	SubSet::Node * rootNode = new SubSet::Node(sserialize::Static::spatial::GeoHierarchy::npos, 0);

	const uint32_t * cPPtrsBegin = &(m_cellParentsPtrs[0]);
	const uint32_t * rPPtrsBegin = &(m_regionParentsPtrs[0]);
	
	for(CellQueryResult::const_iterator it(cqr.cbegin()), end(cqr.cend()); it != end; ++it) {
		uint32_t cellId = it.cellId();
		const CellDesc & cellDesc = m_cellDesc[cellId];
		uint32_t itemsInCell;
		itemsInCell = it.idxSize();
		rootNode->maxItemsSize() += itemsInCell;
		rootNode->cellPositions().push_back(it.pos());

		const uint32_t * cPIt = cPPtrsBegin+cellDesc.parentsBegin;
		const uint32_t * cPEnd;
		cPEnd = cPPtrsBegin+m_cellDesc[cellId+1].parentsBegin;
		for(; cPIt != cPEnd; ++cPIt) {
			uint32_t cP = *cPIt;
			SubSet::Node * n;
			if (!nodes.count(cP)) {
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

	for(std::unordered_map<uint32_t, SubSet::Node*>::iterator it(nodes.begin()), end(nodes.end()); it != end; ++it) {
		uint32_t regionId = it->first;
		const uint32_t * rPIt = rPPtrsBegin + m_regionDesc[regionId].parentsBegin;
		const uint32_t * rPEnd = rPPtrsBegin + m_regionDesc[regionId+1].parentsBegin;
		if (rPIt != rPEnd) {
			for(; rPIt != rPEnd; ++rPIt) {
				uint32_t rp = *rPIt;
				nodes[rp]->push_back(it->second);
			}
		}
		else {
			rootNode->push_back(it->second);
		}
	}
	return rootNode;
}

void GeoHierarchySubGraph::getAncestors(uint32_t rid, std::unordered_set<uint32_t> & dest) {
	SSERIALIZE_CHEAP_ASSERT(rid < m_regionDesc.size());
	for(PointerContainer::const_iterator it(parentsBegin(rid)), end(parentsEnd(rid)); it != end; ++it) {
		getAncestors(*it, dest);
		dest.insert(*it);
	}
}

GeoHierarchySubGraph::PointerContainer::const_iterator 
GeoHierarchySubGraph::parentsBegin(uint32_t rid) const {	
	uint32_t parentsBegin = m_regionDesc.at(rid).parentsBegin;
	SSERIALIZE_CHEAP_ASSERT(parentsBegin <= m_regionParentsPtrs.size());
	return m_regionParentsPtrs.begin()+parentsBegin;
}

GeoHierarchySubGraph::PointerContainer::const_iterator
GeoHierarchySubGraph::parentsEnd(uint32_t rid) const {
	uint32_t parentsEnd = (rid+1 == m_regionDesc.size() ? m_regionParentsPtrs.size() : m_regionDesc.at(rid+1).parentsBegin);
	SSERIALIZE_CHEAP_ASSERT(parentsEnd <= m_regionParentsPtrs.size());
	return m_regionParentsPtrs.begin()+parentsEnd;
}



}//end namespace detail

GeoHierarchySubGraph::GeoHierarchySubGraph() {}

GeoHierarchySubGraph::GeoHierarchySubGraph(const GeoHierarchy & gh, const ItemIndexStore & idxStore, Type t) {
	switch(t) {
	case T_IN_MEMORY:
		m_ghs.reset(new detail::GeoHierarchySubGraph(gh, idxStore) );
		break;
	case T_PASS_THROUGH:
		m_ghs.reset(new detail::PassThroughGeoHierarchySubGraph(gh, idxStore) );
		break;
	default:
		throw sserialize::TypeMissMatchException("GeoHierarchySubGraph");
		break;
	};
}

GeoHierarchySubGraph::~GeoHierarchySubGraph() {}

sserialize::Static::spatial::GeoHierarchy::SubSet
GeoHierarchySubGraph::subSet(const sserialize::CellQueryResult & cqr, bool sparse) const {
	return m_ghs->subSet(cqr, sparse);
}

sserialize::ItemIndex GeoHierarchySubGraph::regionExclusiveCells(uint32_t regionId) const {
	return m_ghs->regionExclusiveCells(regionId);
}

}}//end namespace sserialize::spatial