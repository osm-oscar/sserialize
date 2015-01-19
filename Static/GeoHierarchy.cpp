#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/printers.h>


namespace sserialize {
namespace Static {
namespace spatial {
namespace detail {

void SubSet::Node::dump(std::ostream& out) {
	out << "sserialize::spatial::GeoHierarchy::SubSet::Node\n";
	out << "GH-Id: " << m_ghId << "\n";
	out << "ItemSize: " << m_itemSize << "\n";
	out << "Cell-Positions: " << m_cellPositions << "";
}

void SubSet::Node::dump() {
	dump(std::cout);
}

sserialize::ItemIndex SubSet::idx(const NodePtr & node) const {

	struct MapFunc {
		const SubSet * subset;
		sserialize::ItemIndex operator()(uint32_t pos) const {
			return subset->cqr().idx(pos);
		}
	} mapfunc;
	mapfunc.subset = this;

	struct RedFunc {
		sserialize::ItemIndex operator()(const sserialize::ItemIndex & a, const sserialize::ItemIndex & b) const {
			return a + b;
		}
	} redfunc;


	if (m_sparse) {
		std::unordered_set<uint32_t> idcsPos;
		for(uint32_t i(0), s(node->size()); i < s; ++i) {
			insertCellPositions(node->at(i), idcsPos);
		}
		std::vector<uint32_t> tmp(idcsPos.cbegin(), idcsPos.cend());
		return treeReduceMap<std::vector<uint32_t>::const_iterator, sserialize::ItemIndex, const RedFunc &, const MapFunc &>(
					tmp.cbegin(), tmp.cend(), redfunc, mapfunc);
	}
	else {
		return treeReduceMap<SubSet::Node::CellPositionsContainer::const_iterator, sserialize::ItemIndex, const RedFunc &, const MapFunc &>(
					node->cellPositions().cbegin(), node->cellPositions().cend(), redfunc, mapfunc);
	}
}

bool SubSet::regionByGhId(const SubSet::NodePtr& node, uint32_t ghId, SubSet::NodePtr & dest) const {
	if (node->ghId() == ghId) {
		dest = node;
		return true;
	}
	for(const NodePtr & child : *node) {
		if (regionByGhId(child, ghId, dest)) {
			return true;
		}
	}
	return false;
}

const sserialize::Static::spatial::GeoHierarchy & SubSet::geoHierarchy() const {
	return m_cqr.geoHierarchy();
}

uint32_t SubSet::storeId(const SubSet::NodePtr& node) const {
	return geoHierarchy().ghIdToStoreId(node->ghId());
}

SubSet::NodePtr SubSet::regionByStoreId(uint32_t storeId) const {
	NodePtr res;
	regionByGhId(root(), cqr().geoHierarchy().storeIdToGhId(storeId), res);
	return res;
}


Cell::Cell() : 
m_pos(0),
m_db(0)
{}

Cell::Cell(uint32_t pos, const RCPtrWrapper<GeoHierarchy> & db) : 
m_pos(pos),
m_db(db)
{}

Cell::~Cell() {}

uint32_t Cell::itemPtr() const {
	return (m_db->cells().at(m_pos, CD_ITEM_PTR));
}

uint32_t Cell::itemCount() const {
	return m_db->cellItemsCount(m_pos);
}

uint32_t Cell::parentsBegin() const {
	return (m_db->cells().at(m_pos, CD_PARENTS_BEGIN));
}

uint32_t Cell::directParentsEnd() const {
	return  parentsBegin() + (m_db->cells().at(m_pos, CD_DIRECT_PARENTS_OFFSET));
}

uint32_t Cell::parentsEnd() const {
	return  m_db->cells().at(m_pos+1, CD_PARENTS_BEGIN);
}

uint32_t Cell::parentsSize() const {
	return parentsEnd() - parentsBegin();
}

uint32_t Cell::parent(uint32_t pos) const {
	uint32_t pB = parentsBegin();
	uint32_t pE = parentsEnd();
	if (pB + pos < pE) {
		return m_db->cellPtrs().at( pB + pos );
	}
	return GeoHierarchy::npos;
}

Region::Region() :
m_pos(0),
m_db(0)
{}

Region::Region(uint32_t pos, const RCPtrWrapper<GeoHierarchy> & db) :
m_pos(pos),
m_db(db)
{}

Region::~Region() {}

sserialize::spatial::GeoShapeType Region::type() const {
	return  (sserialize::spatial::GeoShapeType) m_db->regions().at(m_pos, RD_TYPE);
}

uint32_t Region::storeId() const {
	return m_db->ghIdToStoreId(this->ghId());
}

sserialize::spatial::GeoRect Region::boundary() const {
	return m_db->boundary(m_pos);
}

uint32_t Region::cellIndexPtr() const {
	return m_db->regions().at(m_pos, RD_CELL_LIST_PTR);
}

uint32_t Region::parentsBegin() const {
	return m_db->regionParentsBegin(m_pos);
}

uint32_t Region::parentsEnd() const {
	return  m_db->regionParentsEnd(m_pos);
}

uint32_t Region::parentsSize() const {
	return parentsEnd() - parentsBegin();
}

uint32_t Region::parent(uint32_t pos) const {
	uint32_t pB = parentsBegin();
	uint32_t pE = parentsEnd();
	if (pB + pos < pE) {
		return m_db->regionPtrs().at( pB + pos );
	}
	return GeoHierarchy::npos;
}

uint32_t Region::childrenBegin() const {
	return m_db->regions().at(m_pos, RD_CHILDREN_BEGIN);
}

uint32_t Region::childrenEnd() const {
	return parentsBegin();
}

uint32_t Region::childrenSize() const {
	return m_db->regions().at(m_pos, RD_PARENTS_OFFSET);
}

uint32_t Region::child(uint32_t pos) const {
	uint32_t cB = childrenBegin();
	uint32_t cE = childrenEnd();
	if (cB + pos < cE) {
		return m_db->regionPtrs().at( cB + pos );
	}
	return GeoHierarchy::npos;
}

uint32_t Region::itemsPtr() const {
	return m_db->regions().at(m_pos, RD_ITEMS_PTR);
}

uint32_t Region::itemsCount() const {
	return m_db->regionItemsCount(m_pos);
}

GeoHierarchy::GeoHierarchy() {}

GeoHierarchy::GeoHierarchy(const UByteArrayAdapter & data) :
m_storeIdToGhId(data + 1),
m_regions(data + (1+m_storeIdToGhId.getSizeInBytes())),
m_regionPtrs(data + (1+m_storeIdToGhId.getSizeInBytes()+m_regions.getSizeInBytes())),
m_regionBoundaries(data + (1+m_storeIdToGhId.getSizeInBytes()+m_regions.getSizeInBytes()+m_regionPtrs.getSizeInBytes())),
m_cells(data + (1+m_storeIdToGhId.getSizeInBytes()+m_regions.getSizeInBytes()+m_regionPtrs.getSizeInBytes()+m_regionBoundaries.getSizeInBytes())),
m_cellPtrs(data + (1+m_storeIdToGhId.getSizeInBytes()+m_regions.getSizeInBytes()+m_regionPtrs.getSizeInBytes()+m_regionBoundaries.getSizeInBytes()+m_cells.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_GEO_HIERARCHY_VERSION, data.at(0), "sserialize::Static::GeoHierarchy");
}

GeoHierarchy::~GeoHierarchy() {}

OffsetType GeoHierarchy::getSizeInBytes() const {
	return 1 + m_regionPtrs.getSizeInBytes() + m_regions.getSizeInBytes() +  m_cells.getSizeInBytes() + m_cellPtrs.getSizeInBytes();
}

uint32_t GeoHierarchy::cellSize() const {
	uint32_t rs  = m_cells.size();
	return (rs > 0 ? rs-1 : 0);
}

uint32_t GeoHierarchy::cellParentsBegin(uint32_t id) const {
	return cells().at(id, Cell::CD_PARENTS_BEGIN);
}

uint32_t GeoHierarchy::cellDirectParentsEnd(uint32_t id) const {
	return cellParentsBegin(id) + cells().at(id, Cell::CD_DIRECT_PARENTS_OFFSET);
}

uint32_t GeoHierarchy::cellParentsEnd(uint32_t id) const {
	return cells().at(id+1, Cell::CD_PARENTS_BEGIN);
}

uint32_t GeoHierarchy::cellPtrSize() const {
	return m_cellPtrs.size();
}

uint32_t GeoHierarchy::cellPtr(uint32_t pos) const {
	return m_cellPtrs.at(pos);
}

uint32_t GeoHierarchy::cellItemsPtr(uint32_t pos) const {
	return m_cells.at(pos, Cell::CD_ITEM_PTR);
}

uint32_t GeoHierarchy::cellItemsCount(uint32_t pos) const {
	return m_cells.at(pos, Cell::CD_ITEM_COUNT);
}

uint32_t GeoHierarchy::regionSize() const {
	uint32_t rs  = m_regions.size();
	return (rs > 0 ? rs-2 : 0); //we have to subtract 2 because of the rootRegion and the dummy region
}

uint32_t GeoHierarchy::regionCellIdxPtr(uint32_t pos) const {
	return m_regions.at(pos, Region::RD_CELL_LIST_PTR);
}

uint32_t GeoHierarchy::regionItemsPtr(uint32_t pos) const {
	return m_regions.at(pos, Region::RD_ITEMS_PTR);
}

uint32_t GeoHierarchy::regionItemsCount(uint32_t pos) const {
	return m_regions.at(pos, Region::RD_ITEMS_COUNT);
}

uint32_t GeoHierarchy::regionCellSumItemsCount(uint32_t /*pos*/) const {
	std::cerr << "GeoHierachy::regionCellSumItemsCount is not implremented yet" << std::endl;
	return 0xFFFFFFFF;
}

uint32_t GeoHierarchy::regionParentsBegin(uint32_t id) const {
	return m_regions.at(id, Region::RD_CHILDREN_BEGIN) + m_regions.at(id, Region::RD_PARENTS_OFFSET);
}

uint32_t GeoHierarchy::regionParentsEnd(uint32_t id) const {
	return m_regions.at(id+1, Region::RD_CHILDREN_BEGIN);
}

uint32_t GeoHierarchy::regionPtrSize() const {
	return m_regionPtrs.size();
}

uint32_t GeoHierarchy::regionPtr(uint32_t pos) const {
	return m_regionPtrs.at(pos);
}

uint32_t GeoHierarchy::storeIdToGhId(uint32_t storeId) const {
	return m_storeIdToGhId.at(storeId);
}

uint32_t GeoHierarchy::ghIdToStoreId(uint32_t regionId) const {
	return m_regions.at(regionId, Region::RD_STORE_ID);
}


sserialize::spatial::GeoRect GeoHierarchy::boundary(uint32_t pos) const {
	return m_regionBoundaries.at(pos);
}

std::ostream & GeoHierarchy::printStats(std::ostream & out) const {
	out << "sserialize::Static::spatial::GeoHierarchy::stats--BEGIN" << std::endl;
	out << "regions.size()=" << regionSize() << std::endl;
	out << "regionPtrs.size()=" << regionPtrSize() << std::endl;
	out << "cells.size()=" << cellSize() << std::endl;
	out << "cellPtrs.size()=" << cellPtrs().size() << std::endl;
	out << "total data size=" << getSizeInBytes() << std::endl;
	out << "regions data size=" << m_regions.getSizeInBytes() << std::endl;
	out << "region ptr data size=" << m_regionPtrs.getSizeInBytes() << std::endl;
	out << "cell data size=" << m_cells.getSizeInBytes() << std::endl;
	out << "cell ptr data size=" << m_cellPtrs.getSizeInBytes() << std::endl;
	out << "sserialize::Static::spatial::GeoHierarchy::stats--END" << std::endl;
	return out;
}

//TODO:implement sparse SubSet creation
SubSet GeoHierarchy::subSet(const sserialize::CellQueryResult& cqr, bool sparse) const {
	SubSet::Node * rootNode = 0;
	if (cqr.cellCount() > cellSize()*0.5) {
		std::vector<SubSet::Node*> nodes(regionSize()+1, 0);
		rootNode = createSubSet(cqr, &(nodes[0]), nodes.size());
	}
	else {
		std::unordered_map<uint32_t, SubSet::Node*> nodes;
		rootNode = createSubSet(cqr, nodes);
	}
	return SubSet(rootNode, cqr, sparse);
}

//TODO:implement sparse SubSet creation
FlatSubSet GeoHierarchy::flatSubSet(const sserialize::CellQueryResult& cqr, bool /*sparse*/) const {
	FlatSubSet subSet(false);
	//First iteration, count the number of cells
	uint32_t cellListSize = 0;
	std::unordered_map<uint32_t, uint32_t> regionInfo;
	for(sserialize::CellQueryResult::const_iterator it(cqr.cbegin()), end(cqr.cend()); it != end; ++it) {
		uint32_t cellId = it.cellId();
		uint32_t cpIt(cellParentsBegin(cellId));
		uint32_t cpEnd(cellParentsEnd(cellId));
		cellListSize += cpEnd - cpIt;
		for(; cpIt != cpEnd; ++cpIt) {
// 			regions.insert(cellPtr(cpIt));
		}
	}
	
	return subSet;
}

SubSet::Node * GeoHierarchy::createSubSet(const CellQueryResult & cqr, SubSet::Node* *nodes, uint32_t size) const {
	SubSet::Node * rootNode = new SubSet::Node(npos, 0);

	for(CellQueryResult::const_iterator it(cqr.cbegin()), end(cqr.cend()); it != end; ++it) {
		uint32_t cellId = it.cellId();
		uint32_t itemsInCell = (it.fullMatch() ? cellItemsCount(cellId) : it.idxSize());
		rootNode->maxItemsSize() += itemsInCell;
		rootNode->cellPositions().push_back(it.pos());
		for(uint32_t cPIt(cellParentsBegin(cellId)), cPEnd(cellParentsEnd(cellId)); cPIt != cPEnd; ++cPIt) {
			uint32_t cP = cellPtr(cPIt);
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
			uint32_t rPIt(regionParentsBegin(regionId)), rPEnd(regionParentsEnd(regionId));
			if (rPIt != rPEnd) {
				for(; rPIt != rPEnd; ++rPIt) {
					uint32_t rp = regionPtr(rPIt);
					if (nodes[rp]) {
						nodes[rp]->push_back(*it);
					}
					else {
						nodes[rp] = new SubSet::Node(rp, 0);
						nodes[rp]->push_back(*it);
					}
				}
			}
			else {
				rootNode->push_back(*it);
			}
		}
	}
	return rootNode;
}

SubSet::Node * GeoHierarchy::createSubSet(const CellQueryResult & cqr, std::unordered_map<uint32_t, SubSet::Node*> & nodes) const {
	SubSet::Node * rootNode = new SubSet::Node(npos, 0);
	
	for(CellQueryResult::const_iterator it(cqr.cbegin()), end(cqr.cend()); it != end; ++it) {
		uint32_t cellId = it.cellId();
		uint32_t itemsInCell = (it.fullMatch() ? cellItemsCount(cellId) : it.idxSize());
		rootNode->maxItemsSize() += itemsInCell;
		rootNode->cellPositions().push_back(it.pos());
		for(uint32_t cPIt(cellParentsBegin(cellId)), cPEnd(cellParentsEnd(cellId)); cPIt != cPEnd; ++cPIt) {
			uint32_t cP = cellPtr(cPIt);
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
		uint32_t rPIt(regionParentsBegin(regionId)), rPEnd(regionParentsEnd(regionId));
		if (rPIt != rPEnd) {
			for(; rPIt != rPEnd; ++rPIt) {
				uint32_t rp = regionPtr(rPIt);
			nodes[rp]->push_back(it->second);
			}
		}
		else {
			rootNode->push_back(it->second);
		}
	}
	return rootNode;
}


}//end namespace detail


GeoHierarchy::Cell GeoHierarchy::cell(uint32_t id) const {
	return Cell(id, m_priv);
}

GeoHierarchy::Region GeoHierarchy::region(uint32_t id) const {
	return Region(id, m_priv);
}

GeoHierarchy::Region GeoHierarchy::rootRegion() const {
	return region(regionSize());
}

bool GeoHierarchy::consistencyCheck(const sserialize::Static::ItemIndexStore & store) const {
	for(uint32_t i = 0, s = regionSize(); i < s; ++i) {
		Region r = region(i);
		for (uint32_t j = 0, sj = r.childrenSize(); j < sj; ++j) {
			if (i <= r.child(j)) {
				std::cout << "Region-DAG has unsorted ids" << std::endl;
			}
		}
	}
	for(uint32_t i = 0, s = regionSize(); i < s; ++i) {
		Region r = region(i);
		std::vector<ItemIndex> idcs;
		ItemIndex cellIdx = store.at(r.cellIndexPtr());
		for(uint32_t j = 0, sj = cellIdx.size(); j < sj; ++j) {
			sserialize::ItemIndex cellItemsIdx(store.at( this->cell(cellIdx.at(j)).itemPtr() ));
			idcs.push_back(cellItemsIdx);
		}
		ItemIndex mergedIdx = ItemIndex::unite(idcs);
		ItemIndex regionItems = store.at( r.itemsPtr() );
		if (mergedIdx != regionItems) {
			std::cout << "Merged cell idx does not match region index for region " << i << std::endl;
			return false;
		}
	}
	return true;
}

}}} //end namespace

std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Cell & c) {
	out << "sserialize::Static::spatial::GeoHierarchy::Cell[";
	out << ", itemPtr=" << c.itemPtr();
	out << ", itemsCount=" << c.itemCount();
	out << "]";
	return out;
}


std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Region & r) {
	out << "sserialize::Static::spatial::GeoHierarchy::Region[";
	out << r.boundary();
	out << ", cellIndexPtr=" << r.cellIndexPtr();
	out << ", childrenSize=" << r.childrenSize();
	out << ", itemsPtr=" << r.itemsPtr();
	out << ", itemsCount=" << r.itemsCount();
	out << "]";
	return out;
}
