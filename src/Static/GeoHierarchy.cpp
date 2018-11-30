#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/printers.h>
#include <sserialize/stats/statfuncs.h>


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

SubSet::SubSet(Node * root, const sserialize::Static::spatial::GeoHierarchy & gh, const CellQueryResult & cqr, bool sparse) :
m_gh(gh),
m_root(root),
m_cqr(cqr),
m_sparse(sparse)
{}

sserialize::ItemIndex SubSet::items(const NodePtr & node) const {

	struct MapFunc {
		const SubSet * subset;
		sserialize::ItemIndex operator()(uint32_t pos) const {
			return subset->cqr().items(pos);
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
		insertCellPositions(node, idcsPos);
		std::vector<uint32_t> tmp(idcsPos.cbegin(), idcsPos.cend());
		return treeReduceMap<std::vector<uint32_t>::const_iterator, sserialize::ItemIndex, const RedFunc &, const MapFunc &>(
					tmp.cbegin(), tmp.cend(), redfunc, mapfunc);
	}
	else {
		return treeReduceMap<SubSet::Node::CellPositionsContainer::const_iterator, sserialize::ItemIndex, const RedFunc &, const MapFunc &>(
					node->cellPositions().cbegin(), node->cellPositions().cend(), redfunc, mapfunc);
	}
}

ItemIndex SubSet::cells(const SubSet::NodePtr& node) const {
	if (m_sparse) {
		std::unordered_set<uint32_t> idcsPos;
		insertCellPositions(node, idcsPos);
		std::vector<uint32_t> tmp;
		tmp.reserve(idcsPos.size());
		for(uint32_t cellPos : idcsPos) {
			tmp.push_back(cqr().cellId(cellPos));
		}
		std::sort(tmp.begin(), tmp.end());
		return sserialize::ItemIndex(std::move(tmp));
	}
	else {
		std::vector<uint32_t> tmp;
		tmp.reserve(node->cellPositions().size());
		for(uint32_t cellPos : node->cellPositions()) {
			tmp.push_back(cqr().cellId(cellPos));
		}
		return sserialize::ItemIndex(std::move(tmp));
	}
}

sserialize::ItemIndex SubSet::topK(const NodePtr & node, uint32_t numItems) const {

	struct MapFunc {
		const SubSet * subset;
		sserialize::ItemIndex operator()(uint32_t pos) const {
			return subset->cqr().idx(pos);
		}
	} mapfunc;
	mapfunc.subset = this;

	struct RedFunc {
		uint32_t numItems;
		sserialize::ItemIndex operator()(const sserialize::ItemIndex & a, const sserialize::ItemIndex & b) const {
			return sserialize::ItemIndex::uniteK(a, b, numItems);
		}
	} redfunc;
	redfunc.numItems = numItems;


	if (m_sparse) {
		std::unordered_set<uint32_t> idcsPos;
		insertCellPositions(node, idcsPos);
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
	return m_gh;
}

uint32_t SubSet::storeId(const SubSet::NodePtr& node) const {
	return geoHierarchy().ghIdToStoreId(node->ghId());
}

SubSet::NodePtr SubSet::regionByStoreId(uint32_t storeId) const {
	NodePtr res;
	regionByGhId(root(), geoHierarchy().storeIdToGhId(storeId), res);
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

sserialize::spatial::GeoRect Cell::boundary() const {
	return m_db->cellBoundary(m_pos);
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
	return m_db->regionBoundary(m_pos);
}

uint32_t Region::cellIndexPtr() const {
	return m_db->regions().at(m_pos, RD_CELL_LIST_PTR);
}

uint32_t Region::exclusiveCellIndexPtr() const {
	return m_db->regionExclusiveCellIdxPtr(m_pos);
}

uint32_t Region::parentsBegin() const {
	return m_db->regionParentsBegin(m_pos);
}

uint32_t Region::parentsEnd() const {
	return  m_db->regionParentsEnd(m_pos);
}

uint32_t Region::parentsSize() const {
	return m_db->regionParentsSize(m_pos);
}

uint32_t Region::parent(uint32_t pos) const {
	uint32_t pB = parentsBegin();
	uint32_t pE = parentsEnd();
	if (pB + pos < pE) {
		return m_db->regionPtrs().at( pB + pos );
	}
	return GeoHierarchy::npos;
}

uint32_t Region::neighborsBegin() const {
	return m_db->regionNeighborsBegin(m_pos);
}

uint32_t Region::neighborsEnd() const {
	return m_db->regionNeighborsEnd(m_pos);
}

uint32_t Region::neighborsSize() const {
	return m_db->regionNeighborsSize(m_pos);
}

uint32_t Region::neighbor(uint32_t pos) const {
	uint32_t nB = neighborsBegin();
	uint32_t nE = neighborsEnd();
	if (nB + pos < nE) {
		return m_db->regionPtrs().at( nB + pos );
	}
	return GeoHierarchy::npos;
}

uint32_t Region::childrenBegin() const {
	return m_db->regionChildrenBegin(m_pos);
}

uint32_t Region::childrenEnd() const {
	return m_db->regionChildrenEnd(m_pos);
}

uint32_t Region::childrenSize() const {
	return m_db->regionChildrenSize(m_pos);
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
m_cellPtrs(data + (1+m_storeIdToGhId.getSizeInBytes()+m_regions.getSizeInBytes()+m_regionPtrs.getSizeInBytes()+m_regionBoundaries.getSizeInBytes()+m_cells.getSizeInBytes())),
m_cellBoundaries(data + (1+m_storeIdToGhId.getSizeInBytes()+m_regions.getSizeInBytes()+m_regionPtrs.getSizeInBytes()+m_regionBoundaries.getSizeInBytes()+m_cells.getSizeInBytes()+m_cellPtrs.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_GEO_HIERARCHY_VERSION, data.at(0), "sserialize::Static::GeoHierarchy");
}

GeoHierarchy::~GeoHierarchy() {}

OffsetType GeoHierarchy::getSizeInBytes() const {
	return 1 + m_storeIdToGhId.getSizeInBytes() +
			m_regions.getSizeInBytes() + m_regionPtrs.getSizeInBytes() + m_regionBoundaries.getSizeInBytes() +
			m_cells.getSizeInBytes() + m_cellPtrs.getSizeInBytes() + m_cellBoundaries.getSizeInBytes();
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

uint32_t GeoHierarchy::cellParentsSize(uint32_t id) const {
	return cellParentsEnd(id) - cellParentsBegin(id);
}

uint32_t GeoHierarchy::cellPtrsSize() const {
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

uint32_t GeoHierarchy::regionExclusiveCellIdxPtr(uint32_t pos) const {
	return m_regions.at(pos, Region::RD_EXCLUSIVE_CELL_LIST_PTR);
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

uint32_t GeoHierarchy::regionChildrenBegin(uint32_t id) const {
	return m_regions.at(id, Region::RD_CHILDREN_BEGIN);
}

uint32_t GeoHierarchy::regionChildrenEnd(uint32_t id) const {
	return regionParentsBegin(id);
}

uint32_t GeoHierarchy::regionChildrenSize(uint32_t id) const {
	return m_regions.at(id, Region::RD_PARENTS_OFFSET);
}

uint32_t GeoHierarchy::regionParentsBegin(uint32_t id) const {
	return regionChildrenBegin(id) + m_regions.at(id, Region::RD_PARENTS_OFFSET);
}

uint32_t GeoHierarchy::regionParentsEnd(uint32_t id) const {
	return regionNeighborsBegin(id);
}

uint32_t GeoHierarchy::regionParentsSize(uint32_t id) const {
	return m_regions.at(id, Region::RD_NEIGHBORS_OFFSET);
}

uint32_t GeoHierarchy::regionNeighborsBegin(uint32_t id) const {
	return regionParentsBegin(id) + m_regions.at(id, Region::RD_NEIGHBORS_OFFSET);
}

uint32_t GeoHierarchy::regionNeighborsEnd(uint32_t id) const {
	return regionChildrenBegin(id+1);
}

uint32_t GeoHierarchy::regionNeighborsSize(uint32_t id) const {
	return regionNeighborsEnd(id) - regionNeighborsBegin(id);
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


sserialize::spatial::GeoRect GeoHierarchy::regionBoundary(uint32_t pos) const {
	return m_regionBoundaries.at(pos);
}

sserialize::spatial::GeoRect GeoHierarchy::cellBoundary(uint32_t id) const {
	return m_cellBoundaries.at(id);
}

//TODO:implement sparse SubSet creation
SubSet GeoHierarchy::subSet(const sserialize::CellQueryResult& cqr, bool sparse, uint32_t threadCount) const {
	SubSet::Node * rootNode = 0;
	if (cqr.cellCount() > cellSize()*0.5) {
		std::vector<SubSet::Node*> nodes(regionSize()+1, 0);
		rootNode = createSubSet(cqr, &(nodes[0]), (uint32_t) nodes.size(), threadCount);
	}
	else {
		std::unordered_map<uint32_t, SubSet::Node*> nodes;
		rootNode = createSubSet(cqr, nodes);
	}
	sserialize::Static::spatial::GeoHierarchy gh(
		sserialize::RCPtrWrapper<sserialize::Static::spatial::detail::GeoHierarchy>(
			const_cast<sserialize::Static::spatial::detail::GeoHierarchy*>(this)
		)
	);
	return SubSet(rootNode, gh, cqr, sparse);
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

SubSetBase::Node * GeoHierarchy::createSubSet(const CellQueryResult & cqr, SubSet::Node* *nodes, uint32_t size, uint32_t /*threadCount*/) const {
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
			uint32_t regionId = (uint32_t) (it-nodes);
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

SubSetBase::Node * GeoHierarchy::createSubSet(const CellQueryResult & cqr, std::unordered_map<uint32_t, SubSet::Node*> & nodes) const {
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

const uint32_t GeoHierarchy::npos;

std::ostream & GeoHierarchy::printStats(std::ostream & out, const sserialize::Static::ItemIndexStore & store) const {
	std::vector<uint32_t> cellItemSizes(cellSize(), 0);
	std::vector<uint32_t> cellAncestorCount(cellSize(), 0);
	std::vector<uint32_t> cellDepths(cellSize(), 0);
	
	{
		struct MyCellDeptFunc {
			std::vector<uint32_t> & cellDepths;
			const sserialize::Static::ItemIndexStore & store;
			const sserialize::Static::spatial::GeoHierarchy * ghPtr;
			void calc(const Region & r, uint32_t depth) {
				uint32_t cellIdxPtr = r.cellIndexPtr();
				sserialize::ItemIndex cellIdx = store.at(cellIdxPtr);
				for(uint32_t x : cellIdx) {
					uint32_t & y = cellDepths.at(x);
					y = std::max<uint32_t>(y, depth);
				}
				for(uint32_t i(0), s(r.childrenSize()); i < s; ++i) {
					calc(ghPtr->region(r.child(i)), depth+1);
				}
			}
		};
		MyCellDeptFunc mcdf = { .cellDepths = cellDepths, .store = store, .ghPtr = this};
		mcdf.calc(rootRegion(), 0);
	}
	
	uint32_t largestCellSize = 0;
	uint32_t largestCellId = 0;
	uint32_t smallestCellSize = 0xFFFFFFFF;
	uint32_t smallestCellId = 0;
	for(uint32_t i(0), s(cellSize()); i < s; ++i) {
		uint32_t cellItemCount = cellItemsCount(i);
		cellItemSizes[i] = cellItemCount;
		if (largestCellSize < cellItemCount) {
			largestCellSize = cellItemCount;
			largestCellId = i;
		}
		if (smallestCellSize > cellItemCount) {
			smallestCellSize = cellItemCount;
			smallestCellId = i;
		}
		cellAncestorCount[i] = cellParentsSize(i);
	}
	
	std::sort(cellDepths.begin(), cellDepths.end());
	std::sort(cellItemSizes.begin(), cellItemSizes.end());
	std::sort(cellAncestorCount.begin(), cellAncestorCount.end());
	double cs_mean = sserialize::statistics::mean(cellItemSizes.cbegin(), cellItemSizes.cend(), (double)0);
	double cs_stddev = sserialize::statistics::stddev(cellItemSizes.cbegin(), cellItemSizes.cend(), (double)0);
	double cd_mean = sserialize::statistics::mean(cellDepths.cbegin(), cellDepths.cend(), (double)0);
	double cd_stddev= sserialize::statistics::stddev(cellDepths.cbegin(), cellDepths.cend(), (double)0);
	double ca_mean = sserialize::statistics::mean(cellAncestorCount.cbegin(), cellAncestorCount.cend(), (double)0);
	double ca_stddev= sserialize::statistics::stddev(cellAncestorCount.cbegin(), cellAncestorCount.cend(), (double)0);
	out << "sserialize::Static::spatial::GeoHierarchy::stats--BEGIN" << std::endl;
	out << "regions.size()=" << regionSize() << std::endl;
	out << "regionPtrs.size()=" << regionPtrSize() << std::endl;
	out << "cells.size()=" << cellSize() << std::endl;
	out << "cellPtrs.size()=" << cellPtrs().size() << std::endl;
	out << "total data size=" << getSizeInBytes() << std::endl;
	out << "regions data size=" << regions().getSizeInBytes() << std::endl;
	out << "region ptr data size=" << regionPtrs().getSizeInBytes() << std::endl;
	out << "cell data size=" << cells().getSizeInBytes() << std::endl;
	out << "cell ptr data size=" << cellPtrs().getSizeInBytes() << std::endl;
	out << "Cell size info:\n";
	out << "\tmedian: " << cellItemSizes.at(cellItemSizes.size()/2) << "\n";
	out << "\tmin: " << cellItemSizes.front() << "\n";
	out << "\tmax: " << cellItemSizes.back() << "\n";
	out << "\tmean: " << cs_mean << "\n";
	out << "\tstddev: " << cs_stddev << "\n";
	out << "\tid of largest: " << largestCellId << "\n";
	out << "\tid of smallest: " << smallestCellId << "\n";
	out << "Cell depth info:\n";
	out << "\tmedian: " << cellDepths.at(cellDepths.size()/2) << "\n";
	out << "\tmin: " << cellDepths.front() << "\n";
	out << "\tmax: " << cellDepths.back() << "\n";
	out << "\tmean: " << cd_mean << "\n";
	out << "\tstddef: " << cd_stddev << "\n";
	out << "Cell ancestor info:\n";
	out << "\tmedian: " << cellAncestorCount.at(cellAncestorCount.size()/2) << "\n";
	out << "\tmin: " << cellAncestorCount.front() << "\n";
	out << "\tmax: " << cellAncestorCount.back() << "\n";
	out << "\tmean: " << ca_mean << "\n";
	out << "\tstddef: " << ca_stddev << "\n";
	out << "sserialize::Static::spatial::GeoHierarchy::stats--END" << std::endl;
	return out;
}

GeoHierarchy::Cell GeoHierarchy::cell(uint32_t id) const {
	return Cell(id, m_priv);
}

GeoHierarchy::Region GeoHierarchy::region(uint32_t id) const {
	if (id == npos) {
		return rootRegion();
	}
	else {
		return Region(id, m_priv);
	}
}

GeoHierarchy::Region GeoHierarchy::rootRegion() const {
	return region(regionSize());
}

bool GeoHierarchy::consistencyCheck(const sserialize::Static::ItemIndexStore & store) const {
	bool allOk = true;
	for(uint32_t i = 0, s = regionSize(); i < s; ++i) {
		Region r = region(i);
		for (uint32_t j = 0, sj = r.childrenSize(); j < sj; ++j) {
			if (i <= r.child(j)) {
				std::cout << "Region-DAG has unsorted ids" << std::endl;
				allOk = false;
				i = s;
				break;
			}
		}
	}
	for(uint32_t i(0), s(cellSize()); i < s; ++i) {
		if (cellDirectParentsEnd(i) - cellParentsBegin(i) < 1) {
			std::cerr << "Cell " << i << " has no direct parents" << std::endl;
			allOk = false;
			break;
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
			allOk = false;
			break;
		}
	}
	return allOk;
}

sserialize::ItemIndex GeoHierarchy::intersectingCells(const sserialize::Static::ItemIndexStore& idxStore, const sserialize::spatial::GeoRect & rect, uint32_t threadCount) const {
	std::deque<uint32_t> queue;
	std::vector<uint32_t> intersectingCells;
	std::unordered_set<uint32_t> visitedRegions;
	Region r(rootRegion());
	for(uint32_t i(0), s(r.childrenSize()); i < s; ++i) {
		uint32_t childId = r.child(i);
		if (rect.overlap(regionBoundary(childId))) {
			queue.push_back(childId);
			visitedRegions.insert(childId);
		}
	}
	while (queue.size()) {
		r = region(queue.front());
		queue.pop_front();
		if (rect.contains(r.boundary())) {
			//checking the itemsCount of the region does only work if the hierarchy was created with a full region item index
			//so instead we have to check the cellcount
			uint32_t cIdxPtr = r.cellIndexPtr();
			if (idxStore.idxSize(cIdxPtr)) {
				sserialize::ItemIndex idx(idxStore.at(cIdxPtr));
				intersectingCells.insert(intersectingCells.end(), idx.cbegin(), idx.cend());
			}
		}
		else {
			for(uint32_t i(0), s(r.childrenSize()); i < s; ++i) {
				uint32_t childId = r.child(i);
				if (!visitedRegions.count(childId) && rect.overlap(regionBoundary(childId))) {
					queue.push_back(childId);
					visitedRegions.insert(childId);
				}
			}
			//check cells that are not part of children regions
			uint32_t exclusiveCellIndexPtr = r.exclusiveCellIndexPtr();
			if (idxStore.idxSize(exclusiveCellIndexPtr)) {
				sserialize::ItemIndex idx(idxStore.at(exclusiveCellIndexPtr));
				for(uint32_t cellId : idx) {
					if (rect.overlap(cellBoundary(cellId))) {
						intersectingCells.push_back(cellId);
					}
				}
			}
		}
	}
	std::sort(intersectingCells.begin(), intersectingCells.end());
	intersectingCells.resize(std::unique(intersectingCells.begin(), intersectingCells.end())-intersectingCells.begin());
	return sserialize::ItemIndex(std::move(intersectingCells));
}


GeoHierarchy::SubSet
GeoHierarchy::subSet(const sserialize::CellQueryResult & cqr, bool sparse, uint32_t threadCount) const {
	return m_priv->subSet(cqr, sparse, threadCount);
}

GeoHierarchy::FlatSubSet
GeoHierarchy::flatSubSet(const sserialize::CellQueryResult & cqr, bool sparse) const {
	return m_priv->flatSubSet(cqr, sparse);
}

GeoHierarchyCellInfo::GeoHierarchyCellInfo(const sserialize::Static::spatial::GeoHierarchy & gh) :
m_gh(gh)
{}

GeoHierarchyCellInfo::SizeType
GeoHierarchyCellInfo::cellSize() const {
	return m_gh.cellSize();
}

sserialize::spatial::GeoRect
GeoHierarchyCellInfo::cellBoundary(CellId cellId) const {
	return m_gh.cellBoundary(cellId);
}

GeoHierarchyCellInfo::SizeType
GeoHierarchyCellInfo::cellItemsCount(CellId cellId) const {
	return m_gh.cellItemsCount(cellId);
}

GeoHierarchyCellInfo::IndexId
GeoHierarchyCellInfo::cellItemsPtr(CellId cellId) const {
	return m_gh.cellItemsPtr(cellId);
}

sserialize::RCPtrWrapper<interface::CQRCellInfoIface>
GeoHierarchyCellInfo::makeRc(const sserialize::Static::spatial::GeoHierarchy & gh) {
	return sserialize::RCPtrWrapper<interface::CQRCellInfoIface>( new GeoHierarchyCellInfo(gh) );
}

}}} //end namespace

std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Cell & c) {
	out << "sserialize::Static::spatial::GeoHierarchy::Cell[";
	out << "itemPtr=" << c.itemPtr();
	out << ", itemsCount=" << c.itemCount();
	if (c.parentsSize()) {
		out << ", parents:";
		char sep = '(';
		for(uint32_t i(0), s(c.parentsSize()); i != s; ++i) {
			out << sep << c.parent(i);
			sep = ',';
		}
		out << ")";
		
	}
	out << "]";
	return out;
}


std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Region & r) {
	out << "sserialize::Static::spatial::GeoHierarchy::Region[";
	out << r.boundary();
	out << ", cellIndexPtr=" << r.cellIndexPtr();
	out << ", parentsSize=" << r.parentsSize();
	out << ", childrenSize=" << r.childrenSize();
	out << ", neighborSize=" << r.neighborsSize();
	out << ", itemsPtr=" << r.itemsPtr();
	out << ", itemsCount=" << r.itemsCount();
	out << ", storeId=" << r.storeId();
	out << ", gId=" << r.ghId();
	out << "]";
	return out;
}
