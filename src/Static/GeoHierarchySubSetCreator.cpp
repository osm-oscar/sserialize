#include <sserialize/Static/GeoHierarchySubSetCreator.h>

namespace sserialize {
namespace spatial {
namespace detail {

GeoHierarchySubSetCreator::GeoHierarchySubSetCreator() {}

GeoHierarchySubSetCreator::GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh) :
GeoHierarchySubSetCreator(gh, [](uint32_t) { return true; })
{}

GeoHierarchySubSetCreator::~GeoHierarchySubSetCreator() {}

sserialize::Static::spatial::GeoHierarchy::SubSet::Node *
GeoHierarchySubSetCreator::subSet(const sserialize::CellQueryResult& cqr, bool sparse) const {
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

}}}//end namespace