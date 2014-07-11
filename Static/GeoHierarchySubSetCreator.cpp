#include <sserialize/Static/GeoHierarchySubSetCreator.h>

namespace sserialize {
namespace spatial {
namespace detail {

GeoHierarchySubSetCreator::GeoHierarchySubSetCreator() {}

GeoHierarchySubSetCreator::GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh) :
m_gh(gh)
{
	m_cellParentsPtrs.reserve(m_gh.cellPtrSize());
	m_regionParentsPtrs.reserve(m_gh.regionPtrSize());
	m_regionDesc.reserve(m_gh.regionSize());
	m_cellDesc.reserve(m_gh.cellSize());
	
	const sserialize::Static::spatial::GeoHierarchy::RegionPtrListType & ghRegionPtrs = m_gh.regionPtrs();
	const sserialize::Static::spatial::GeoHierarchy::CellPtrListType & ghCellPtrs = m_gh.cellPtrs();
	
	for(uint32_t i(0), s(m_gh.regionSize()); i < s; ++i) {
		m_regionDesc.push_back( RegionDesc( m_regionParentsPtrs.size()) );
		
		for(uint32_t rPIt(m_gh.regionParentsBegin(i)), rPEnd(m_gh.regionParentsEnd(i)); rPIt != rPEnd; ++rPIt) {
			m_regionParentsPtrs.push_back( ghRegionPtrs.at(rPIt) );
		}
	}
	
	for(uint32_t i(0), s(m_gh.cellSize()); i < s; ++i) {
		
		
		uint32_t cPIt(m_gh.cellParentsBegin(i));
		uint32_t cdPEnd(m_gh.cellDirectParentsEnd(i));
		uint32_t cPEnd(m_gh.cellParentsEnd(i));
		
		m_cellDesc.push_back( CellDesc( m_cellParentsPtrs.size(), m_cellParentsPtrs.size() + (cdPEnd-cPIt), m_gh.cellItemsCount(i)));
		
		for(; cPIt != cPEnd; ++cPIt) {
			m_cellParentsPtrs.push_back( ghCellPtrs.at(cPIt) );
		}

	}
	//dummy end regions
	m_regionDesc.push_back( RegionDesc( m_regionParentsPtrs.size()) );
	m_cellDesc.push_back( CellDesc( m_cellParentsPtrs.size(), 0, 0) );
}

GeoHierarchySubSetCreator::~GeoHierarchySubSetCreator() {}

sserialize::Static::spatial::GeoHierarchy::SubSet::Node *
GeoHierarchySubSetCreator::subSet(const sserialize::CellQueryResult& cqr, bool sparse) {
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
	return rootNode;
}

sserialize::Static::spatial::GeoHierarchy::SubSet::Node *
GeoHierarchySubSetCreator::createSubSet(const CellQueryResult & cqr, std::unordered_map<uint32_t, SubSet::Node*> & nodes) const {
	SubSet::Node * rootNode = new SubSet::Node(sserialize::Static::spatial::GeoHierarchy::npos);

	const uint32_t * cPPtrsBegin = &(m_cellParentsPtrs[0]);
	const uint32_t * rPPtrsBegin = &(m_regionParentsPtrs[0]);
	
	for(CellQueryResult::const_iterator it(cqr.cbegin()), end(cqr.cend()); it != end; ++it) {
		uint32_t cellId = it.cellId();
		const CellDesc & cellDesc = m_cellDesc[cellId];
		uint32_t itemsInCell;
		itemsInCell = (it.fullMatch() ? cellDesc.itemsCount : it.idxSize());
		rootNode->maxItemsSize() += itemsInCell;
		rootNode->cellPositions().push_back(it.pos());

		const uint32_t * cPIt = cPPtrsBegin+cellDesc.parentsBegin;
		const uint32_t * cPEnd;
		cPEnd = cPPtrsBegin+m_cellDesc[cellId+1].parentsBegin;
		for(; cPIt != cPEnd; ++cPIt) {
			uint32_t cP = *cPIt;
			SubSet::Node * n;
			if (!nodes.count(cP)) {
				n = new SubSet::Node(cP);
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

}}}//end namespace