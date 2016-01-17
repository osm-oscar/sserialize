#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/utility/log.h>
#include <sserialize/iterator/RangeGenerator.h>

namespace sserialize {
namespace spatial {

namespace detail {
namespace geohierarchy {

CellList::CellList() :
m_cellIdData(sserialize::MM_FAST_FILEBASED),
m_cellItems(sserialize::MM_FILEBASED),
m_d(sserialize::MM_PROGRAM_MEMORY)
{}


CellList::CellList(const sserialize::MMVector<uint32_t> & cellIdData, const sserialize::MMVector<uint32_t> & cellItems, const sserialize::MMVector<Cell> & cells) :
m_cellIdData(cellIdData),
m_cellItems(cellItems),
m_d(cells)
{}

CellList::CellList(sserialize::MMVector<uint32_t> && cellIdData, sserialize::MMVector<uint32_t> && cellItems, sserialize::MMVector<Cell> && cells) :
m_cellIdData(std::move(cellIdData)),
m_cellItems(std::move(cellItems)),
m_d(std::move(cells))
{}

CellList::CellList(const CellList & other) :
m_cellIdData(other.m_cellIdData),
m_cellItems(other.m_cellItems),
m_d(other.m_d)
{}

CellList::CellList(const CellList && other) :
m_cellIdData(std::move(other.m_cellIdData)),
m_cellItems(std::move(other.m_cellItems)),
m_d(std::move(other.m_d))
{}

CellList::~CellList() {}


CellList & CellList::operator=(CellList && other) {
	m_cellIdData = std::move(other.m_cellIdData);
	m_cellItems = std::move(other.m_cellItems);
	m_d = std::move(other.m_d);
	return *this;
}

CellList & CellList::operator=(const CellList & other) {
	m_cellIdData = other.m_cellIdData;
	m_cellItems = other.m_cellItems;
	m_d = other.m_d;
	return *this;
}

void CellList::swap(CellList & other) {
	using std::swap;
	swap(m_cellIdData, other.m_cellIdData);
	swap(m_cellItems, other.m_cellItems);
	swap(m_d, other.m_d);
}

void RegionList::clear() {
	m_data.clear();
	m_regions.clear();
}

RegionList::Region::Region() :
m_d(0),
m_off(0),
m_childrenSize(0),
m_parentsSize(0),
m_neighborsSize(0),
m_cellsSize(0),
ghId(0),
storeId(0),
type(sserialize::spatial::GS_NONE)
{}

RegionList::Region::Region(const RegionList::Region & other) :
m_d(other.m_d),
m_off(other.m_off),
m_childrenSize(other.m_childrenSize),
m_parentsSize(other.m_parentsSize),
m_neighborsSize(other.m_neighborsSize),
m_cellsSize(other.m_cellsSize),
ghId(other.ghId),
storeId(other.storeId),
type(sserialize::spatial::GS_NONE),
boundary(other.boundary)
{}

RegionList::Region::Region(DataContainer * d, uint64_t off, uint32_t childrenSize, uint32_t parentsSize, uint32_t cellsSize, uint32_t neighborsSize) :
m_d(d),
m_off(off),
m_childrenSize(childrenSize),
m_parentsSize(parentsSize),
m_neighborsSize(neighborsSize),
m_cellsSize(cellsSize),
ghId(0),
storeId(0),
type(sserialize::spatial::GS_NONE)
{}

void RegionList::Region::swap(RegionList::Region & other) {
	using std::swap;
	swap(m_d, other.m_d);
	swap(m_off, other.m_off);
	swap(m_childrenSize, other.m_childrenSize);
	swap(m_parentsSize, other.m_parentsSize);
	swap(m_neighborsSize, other.m_neighborsSize);
	swap(m_cellsSize, other.m_cellsSize);
	swap(type, other.type);
	swap(storeId, other.storeId);
	swap(ghId, other.ghId);
	swap(boundary, other.boundary);
}

}}//end namespace detail::geohierarchy

void GeoHierarchy::depths(std::vector<uint32_t> & d, uint32_t me) const {
	for(uint32_t child : m_regions.at(me).children()) {
		if (d[child] <= d[me]) {
			d[child] = d[me] + 1;
			depths(d, child);
		}
	}
}

void GeoHierarchy::splitCellParents(uint32_t cellId, std::vector<uint32_t> & directParents, std::vector<uint32_t> & remainingParents) const {
	directParents.clear();
	remainingParents.clear();
	std::unordered_set<uint32_t> removedParents;
	const Cell & cell = m_cells[cellId];
	for(auto cPIt(cell.parentsBegin()), cPEnd(cell.parentsEnd()); cPIt != cPEnd; ++cPIt) {
		if (removedParents.count(*cPIt)) {
			remainingParents.push_back(*cPIt);
		}
		else {
			directParents.push_back(*cPIt);
			getAncestors(*cPIt, removedParents);
		}
	}
}

void GeoHierarchy::createRootRegion() {
	if (m_rootRegion.cellsSize()) {
		throw std::runtime_error("GeoHierarchy::createRootRegion can only be called once");
	}
	
	m_rootRegion.type = sserialize::spatial::GS_NONE;
	m_rootRegion.ghId = m_regions.size();
	m_rootRegion.storeId = 0;
	m_rootRegion.boundary = GeoRect();
	m_rootRegion.m_d = &(m_regions.m_data);
	m_rootRegion.m_off = m_regions.m_data.size();
	m_rootRegion.m_childrenSize = 0;
	m_rootRegion.m_parentsSize = 0;
	m_rootRegion.m_neighborsSize = 0;
	m_rootRegion.m_cellsSize = m_cells.size();
	
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions[i];
		if (!r.parentsSize()) {
			m_regions.m_data.push_back(r.ghId);
			++m_rootRegion.m_childrenSize;
		}
		m_rootRegion.boundary.enlarge(r.boundary);
	}
	m_regions.m_data.push_back(sserialize::RangeGenerator<uint32_t>::begin(0, m_cells.size()), sserialize::RangeGenerator<uint32_t>::end(0, m_cells.size()));
}

bool GeoHierarchy::checkConsistency() {
	for(uint32_t i(0), s(m_regions.size()); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(auto rnIt(r.neighborsBegin()), rnEnd(r.neighborsEnd()); rnIt != rnEnd; ++rnIt) {
			const Region & rn = m_regions.at(*rnIt);
			if (rn.neighborsEnd() == std::find(rn.neighborsBegin(), rn.neighborsEnd(), i)) {
				std::cout << "Region " << i << " has a non-matched neighbor pointer to region " << *rnIt <<  std::endl;
				return false;
			}
		}
	}
	
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(uint32_t x : r.cells()) {
			if (!r.boundary.contains(m_cells.at(x).boundary())) {
				std::cout << "Region " << i << " has cells with non-contained boundary" << std::endl;
				return false;
			}
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		if (r.childrenSize() && !std::is_sorted(r.childrenBegin(), r.childrenEnd())) {
			std::cout << "Region " << i << " has unsorted children" << std::endl;
			return false;
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		if (r.parentsSize() && !std::is_sorted(r.parentsBegin(), r.parentsEnd())) {
			std::cout << "Region " << i << " has unsorted parents" << std::endl;
			return false;
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		if (haveCommonValueOrdered(r.parents(), r.children())) {
			std::cout << "Region " << i << " has common parent/child: ";
			sserialize::print(std::cout, sserialize::intersect< Region::ConstDataContainerWrapper, Region::ConstDataContainerWrapper, std::vector<uint32_t> >(r.parents(), r.children()));
			std::cout << std::endl;
			return false;
		}
	}
	{
		std::vector<uint32_t> regionsWithoutParentChildren;
		for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
			const Region & r = m_regions.at(i);
			if (r.parentsSize() == 0 && r.childrenSize() == 0) {
				regionsWithoutParentChildren.push_back(i);
			}
		}
		std::vector<uint32_t> invalidRegions;
		std::set_difference(regionsWithoutParentChildren.cbegin(), regionsWithoutParentChildren.cend(),
							m_rootRegion.childrenBegin(), m_rootRegion.childrenEnd(),
							std::back_insert_iterator< std::vector<uint32_t> >(invalidRegions));
		for(uint32_t i : invalidRegions) {
			std::cout << "Region " << i << " has no children and no parents" << std::endl;
		}
		if (invalidRegions.size())
			return false;
	}

	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(const auto x : r.children()) {
			const Region & cr = m_regions.at(x);
			if (cr.parentsSize() && !std::binary_search(cr.parentsBegin(), cr.parentsEnd(), i)) {
				std::cout << "Region " << x << " has missing parent pointer to " << i << std::endl;
				return false;
			}
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(const auto x : r.children()) {
			const Region & cr = m_regions.at(x);
			if (cr.childrenSize() && std::binary_search(cr.childrenBegin(), cr.childrenEnd(), i)) {
				std::cout << "Region " << x << " has parent " << i << " as child" << std::endl;
				return false;
			}
		}
	}
	uint32_t numIncorrectlySampledRegions = 0;
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(const auto x : r.children()) {
			const Region & cr = m_regions.at(x);
			if (!r.boundary.contains(cr.boundary)) {
				if (!sserialize::subset_of(cr.cellsBegin(), cr.cellsEnd(), r.cellsBegin(), r.cellsEnd())) {
					std::cout << "Region " << i << " does not span child " << x << std::endl;
					return false;
				}
				else {
					++numIncorrectlySampledRegions;
// 					std::cout << "Region " << i << " with child " << x << " were not correctly sampled" << std::endl;
				}
			}
		}
	}
	if (numIncorrectlySampledRegions) {
		std::cout << "There are " << numIncorrectlySampledRegions << " incorrectly sampled regions-child relations" << std::endl;
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(const auto x : r.children()) {
			if (i <= x) {
				const Region & cr = m_regions.at(x);
				std::cout << "Region " << i << " with child " << x << " have incorrect id order. storeId=" << r.storeId << ", ghId=" << r.ghId;
				std::cout << ", cr.storeId=" << cr.storeId << ", cr.ghId=" << cr.ghId << std::endl;
				std::cout << "Region cells:" << r.cells() << std::endl;
				std::cout << "Child-Region cells: " << cr.cells() << std::endl;
				return false;
			}
		}
	}
	
	//cells
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells.at(i);
		if (!std::is_sorted(c.itemsBegin(), c.itemsEnd())) {
			std::cout << "Cell " << i << " has unsorted items" << std::endl;
			return false;
		}
	}
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells.at(i);
		if (!std::is_sorted(c.parentsBegin(), c.parentsEnd())) {
			std::cout << "Cell " << i << " has unsorted parents" << std::endl;
			return false;
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(const auto x : r.cells()) {
			const Cell & c = m_cells.at(x);
			if (!std::binary_search(c.parentsBegin(), c.parentsEnd(), i)) {
				std::cout << "Cell " << x << " has missing parent " << i << std::endl;
				return false;
			}
		}
	}
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells.at(i);
		for(auto cPIt(c.parentsBegin()), cPEnd(c.parentsEnd()); cPIt != cPEnd; ++cPIt) {
			const Region & r = m_regions.at(*cPIt);
			if (!std::binary_search(r.cellsBegin(), r.cellsEnd(), i)) {
				std::cout << "Region " << *cPIt << " has missing cell " << i << std::endl;
				return false;
			}
		}
	}
	return true;
}

void GeoHierarchy::printStats(std::ostream & out) const {
	out << "sserialize::spatial::GeoHierarchy--stats-BEGIN" << std::endl;
	if (!m_regions.size()) {
		std::cout << "Hierarchy is empty" << std::endl;
	}
	else {
		RegionList::const_iterator maxCellSplitIt = std::max_element(m_regions.cbegin(), m_regions.cend(), [] (const Region & x, const Region & y) {return x.cellsSize() < y.cellsSize();});
		RegionList::const_iterator maxChildrenIt = std::max_element(m_regions.cbegin(), m_regions.cend(), [] (const Region & x, const Region & y) {return x.childrenSize() < y.childrenSize();});
		RegionList::const_iterator maxParentsIt = std::max_element(m_regions.cbegin(), m_regions.cend(), [] (const Region & x, const Region & y) {return x.parentsSize() < y.parentsSize();});
		out << "max cellsplit at " << maxCellSplitIt-m_regions.cbegin() << " with " << maxCellSplitIt->cellsSize() << std::endl;
		out << "max children at " << maxChildrenIt-m_regions.cbegin() << " with " << maxChildrenIt->childrenSize() << std::endl;
		out << "max parents at " << maxParentsIt-m_regions.cbegin() << " with " << maxParentsIt->parentsSize() << std::endl;
		out << "#cells: " << m_cells.size() << std::endl;
		out << "#items in all cells: " << m_cells.cellItemList().size() << std::endl;
		out << "#child ptrs: " << m_regions.m_data.size() << std::endl;
	}
	out << "sserialize::spatial::GeoHierarchy--stats-END" << std::endl;
}

UByteArrayAdapter GeoHierarchy::append(sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory& idxFactory, bool fullItemsIndex) const {
	bool allOk = true;
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(11); //version
	{ //put the storeIdToGhId map:
		std::vector<uint32_t> tmp(m_regions.size(), 0);
		for(uint32_t ghId(0), s(m_regions.size()); ghId < s; ++ghId) {
			tmp.at(m_regions.at(ghId).storeId) = ghId;
		}
		dest << tmp;
	}

	std::vector<uint32_t> maxValues(sserialize::Static::spatial::GeoHierarchy::Region::RD__ENTRY_SIZE, 0);
	uint32_t & mrdCellListIndexPtr = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR];
	uint32_t & mrdExclusiveCellListIndexPtr = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_EXCLUSIVE_CELL_LIST_PTR];
	uint32_t & mrdId= maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_STORE_ID];
	uint32_t & mrdChildrenBegin = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN];
	uint32_t & mrdParentsOffset = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET];
	uint32_t & mrdNeighborsOffset = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_NEIGHBORS_OFFSET];
	uint32_t & mrdType= maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE];
	
	uint32_t curPtrOffset = 0;

	std::vector<uint32_t> cellListIndexPtrs(m_regions.size(), 0);
	std::vector<uint32_t> exclusiveCellListIndexPtrs(m_regions.size(), 0);
	uint32_t rootRegionCellListIndexPtr;
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions[i];
		bool ok = true;
		uint32_t cellListIndexPtr = idxFactory.addIndex(r.cells());
		uint32_t exclusiveCellListIndexPtr;
		if (r.childrenSize()) {
			//get the region exclusive cells
			//children have a smaller id, thus checking if a cell has a parent smaller than the region id suffices to find region exclusive cells
			//furthermore cell-parents are sorted, thus it's enough to check if the first parent is the region or not
			std::vector<uint32_t> tmp;
			for(uint32_t cellId : r.cells()) {
				if (*(cell(cellId).parentsBegin()) == i) {
					tmp.push_back(cellId);
				}
			}
			exclusiveCellListIndexPtr = idxFactory.addIndex(tmp);
		}
		else {
			exclusiveCellListIndexPtr = cellListIndexPtr;
		}
		mrdCellListIndexPtr = std::max<uint32_t>(mrdCellListIndexPtr, cellListIndexPtr);
		mrdExclusiveCellListIndexPtr = std::max<uint32_t>(mrdExclusiveCellListIndexPtr, exclusiveCellListIndexPtr);
		mrdType = std::max<uint32_t>(mrdType, r.type);
		mrdId = std::max<uint32_t>(mrdId, r.storeId);
		mrdParentsOffset = std::max<uint32_t>(mrdParentsOffset, r.childrenSize());
		mrdNeighborsOffset = std::max<uint32_t>(mrdNeighborsOffset, r.parentsSize());
		curPtrOffset += r.childrenSize() + r.parentsSize() + r.neighborsSize();
		cellListIndexPtrs[i] = cellListIndexPtr;
		exclusiveCellListIndexPtrs[i] = exclusiveCellListIndexPtr;
		allOk = allOk && ok;
	}
	rootRegionCellListIndexPtr = idxFactory.addIndex(m_rootRegion.cells());
	mrdCellListIndexPtr = std::max<uint32_t>(mrdCellListIndexPtr, rootRegionCellListIndexPtr);
	curPtrOffset += m_rootRegion.childrenSize();
	mrdParentsOffset = std::max<uint32_t>(mrdParentsOffset, m_rootRegion.childrenSize());
	mrdChildrenBegin = curPtrOffset;

	std::vector< std::pair<uint32_t, uint32_t> > fullItemIndexInfo;
	//check for fullItemsPtr
	if (fullItemsIndex) {
		fullItemIndexInfo = createFullRegionItemIndex(idxFactory);
		uint32_t & mrdFullItemsPtr = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_PTR];
		uint32_t & mrdFullItemsCount = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_COUNT];
		for(std::vector< std::pair<uint32_t, uint32_t> >::const_iterator it(fullItemIndexInfo.cbegin()), end(fullItemIndexInfo.cend()); it != end; ++it) {
			mrdFullItemsPtr = std::max(mrdFullItemsPtr, it->first);
			mrdFullItemsCount = std::max(mrdFullItemsCount, it->second);
		}
	}
	else {
		fullItemIndexInfo.resize(m_regions.size()+1, std::pair<uint32_t, uint32_t>(0,0));
	}
	
	std::vector<uint8_t> bitConfig;
	for(uint32_t i = 0, s = maxValues.size(); i < s; ++i) {
		bitConfig.push_back( CompactUintArray::minStorageBits(maxValues[i]) );
	}
	curPtrOffset = 0;
	
	{
		std::vector<uint8_t> tmp;
		sserialize::UByteArrayAdapter tmpDest(&tmp, false);
		MultiVarBitArrayCreator mvaCreator(bitConfig, tmpDest);
		mvaCreator.reserve(m_regions.size()+2);
		for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
			const Region & r = m_regions[i];
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR, cellListIndexPtrs[i]);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_EXCLUSIVE_CELL_LIST_PTR, exclusiveCellListIndexPtrs[i]);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE, r.type);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_STORE_ID, r.storeId);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN, curPtrOffset);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET, r.childrenSize());
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_NEIGHBORS_OFFSET, r.parentsSize());
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_PTR, fullItemIndexInfo[i].first);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_COUNT, fullItemIndexInfo[i].second);
			curPtrOffset += r.childrenSize() + r.parentsSize() + r.neighborsSize();
		}
		//append the root region region
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_PTR, fullItemIndexInfo.back().first);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_COUNT, fullItemIndexInfo.back().second);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR, rootRegionCellListIndexPtr);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_EXCLUSIVE_CELL_LIST_PTR, 0);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE, sserialize::spatial::GS_NONE);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_STORE_ID, m_rootRegion.storeId);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN, curPtrOffset);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET, m_rootRegion.childrenSize());
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_NEIGHBORS_OFFSET, 0);
		curPtrOffset += m_rootRegion.childrenSize();
		
		//append the dummy region which is needed for the offset array
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_PTR, 0);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_COUNT, 0);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR, 0);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_EXCLUSIVE_CELL_LIST_PTR, 0);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE, sserialize::spatial::GS_NONE);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_STORE_ID, 0);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN, curPtrOffset);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET, 0);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_NEIGHBORS_OFFSET, 0);
		
		mvaCreator.flush();
		dest.putData(tmp);
	}
	
	sserialize::MMVector<uint32_t> ptrOffsetArray(sserialize::MM_FILEBASED);
	ptrOffsetArray.reserve(curPtrOffset);
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		ptrOffsetArray.push_back(m_regions[i].childrenBegin(), m_regions[i].childrenEnd());
		ptrOffsetArray.push_back(m_regions[i].parentsBegin(), m_regions[i].parentsEnd());
		ptrOffsetArray.push_back(m_regions[i].neighborsBegin(), m_regions[i].neighborsEnd());
	}
	//append the root region
	ptrOffsetArray.push_back(m_rootRegion.childrenBegin(), m_rootRegion.childrenEnd());
	
	allOk = allOk && (BoundedCompactUintArray::create(ptrOffsetArray, dest) > 0);
	//append the RegionRects
	{
		sserialize::Static::ArrayCreator<sserialize::spatial::GeoRect> bCreator(dest);
		for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
			bCreator.put(m_regions.at(i).boundary);
		}
		bCreator.put(m_rootRegion.boundary);
		bCreator.flush();
	}
	//and do the same for the cells
	
	maxValues.resize(sserialize::Static::spatial::GeoHierarchy::Cell::CD__ENTRY_SIZE, 0);
	cellListIndexPtrs.clear();
	cellListIndexPtrs.resize(m_cells.size(), 0);
	std::vector<uint32_t> & cellItemsIndexPtrs = cellListIndexPtrs;
	
	uint32_t & mcdItemPtr = maxValues[sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_PTR];
	uint32_t & mcdItemCount = maxValues[sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_COUNT];
	uint32_t & mcdParentBegin = maxValues[sserialize::Static::spatial::GeoHierarchy::Cell::CD_PARENTS_BEGIN];
	uint32_t & mcdDirectParentsCount = maxValues[sserialize::Static::spatial::GeoHierarchy::Cell::CD_DIRECT_PARENTS_OFFSET];
	
	std::vector<uint32_t> cellDirectParents, cellRemainingParents;
	
	curPtrOffset = 0;
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells[i];
		splitCellParents(i, cellDirectParents, cellRemainingParents);
		uint32_t itemPtr = idxFactory.addIndex(c.items());
		mcdItemPtr = std::max<uint32_t>(mcdItemPtr, itemPtr);
		mcdItemCount = std::max<uint32_t>(mcdItemCount, c.itemsSize());
		mcdDirectParentsCount = std::max<uint32_t>(mcdDirectParentsCount, cellDirectParents.size());
		curPtrOffset += c.parentsSize();
		cellItemsIndexPtrs[i] = itemPtr;
	}
	mcdParentBegin = curPtrOffset;
	
	bitConfig.clear();
	for(uint32_t i = 0, s = maxValues.size(); i < s; ++i) {
		bitConfig.push_back( CompactUintArray::minStorageBits(maxValues[i]) );
	}
	ptrOffsetArray.clear();
	ptrOffsetArray.reserve(curPtrOffset);
	curPtrOffset = 0;
	
	{
		std::vector<uint8_t> tmp;
		sserialize::UByteArrayAdapter tmpDest(&tmp, false);
		MultiVarBitArrayCreator mvaCreator(bitConfig, tmpDest);
		mvaCreator.reserve(m_cells.size()+1);
		for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
			const Cell & c = m_cells[i];
			splitCellParents(i, cellDirectParents, cellRemainingParents);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_PTR, cellItemsIndexPtrs[i]);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_COUNT, c.itemsSize());
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Cell::CD_PARENTS_BEGIN, curPtrOffset);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Cell::CD_DIRECT_PARENTS_OFFSET, cellDirectParents.size());
			
			ptrOffsetArray.push_back(cellDirectParents.cbegin(), cellDirectParents.cend());
			ptrOffsetArray.push_back(cellRemainingParents.cbegin(), cellRemainingParents.cend());
			curPtrOffset += c.parentsSize();
		}
		mvaCreator.set(m_cells.size(), sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_PTR, 0);
		mvaCreator.set(m_cells.size(), sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_COUNT, 0);
		mvaCreator.set(m_cells.size(), sserialize::Static::spatial::GeoHierarchy::Cell::CD_PARENTS_BEGIN, curPtrOffset);
		mvaCreator.set(m_cells.size(), sserialize::Static::spatial::GeoHierarchy::Cell::CD_DIRECT_PARENTS_OFFSET, 0);
		
		mvaCreator.flush();
		dest.putData(tmp);
	}

	allOk = allOk && (BoundedCompactUintArray::create(ptrOffsetArray, dest) > 0);
	
	//append the cell boundaries
	{
		sserialize::Static::ArrayCreator<sserialize::spatial::GeoRect> bCreator(dest);
		for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
			bCreator.put(m_cells.at(i).boundary());
		}
		bCreator.flush();
	}
	
	if (!allOk) {
		return UByteArrayAdapter();
	}
	else {
		UByteArrayAdapter ret(dest);
		ret.setPutPtr(beginOffset);
		ret.shrinkToPutPtr();
		return ret;
	}
}

bool GeoHierarchy::regionEqTest(uint32_t i, const GeoHierarchy::Region & r, const sserialize::Static::spatial::GeoHierarchy::Region & sr, const sserialize::Static::ItemIndexStore * idxStore) const {
	bool ok = true;
	if (r.childrenSize() != sr.childrenSize()) {
		std::cout << "Childern.size of region " << i << " differ" << std::endl;
		ok = false;
	}
	if (r.parentsSize() != sr.parentsSize()) {
		std::cout << "parent.size of region " << i << " differ" << std::endl;
		ok = false;
	}
	if (r.neighborsSize() != sr.neighborsSize()) {
		std::cout << "neighbor.size of region " << i << " differ" << std::endl;
		ok = false;
	}
	if (r.storeId != sr.storeId()) {
		std::cout << "storeId of region " << i << " differs. Should=" << r.storeId << ", IS=" << sr.storeId() << std::endl;
		ok = false;
	}
	if (r.ghId != sr.ghId()) {
		std::cout << "ghId of region " << i << " differs. Should=" << r.ghId << ", IS=" << sr.ghId() << std::endl;
		ok = false;
	}
	if (r.type != sr.type()) {
		std::cout << "type of region " << i << " differs" << std::endl;
		ok = false;
	}
	for(uint32_t j = 0, js = r.childrenSize(); j < js; ++j) {
		if (r.child(j) != sr.child(j)) {
			std::cout << "child " << j << " of region " << i << " differs. Want: " << r.child(j) << ", have: " << sr.child(j) << std::endl;
			ok = false;
		}
	}
	for(uint32_t j = 0, js = r.parentsSize(); j < js; ++j) {
		if (r.parent(j) != sr.parent(j)) {
			std::cout << "parent " << j << " of region " << i << " differs" << std::endl;
			ok = false;
		}
	}
	for(uint32_t j = 0, js = r.neighborsSize(); j < js; ++j) {
		if (r.neighbor(j) != sr.neighbor(j)) {
			std::cout << "neighbor " << j << " of region " << i << " differs" << std::endl;
			ok = false;
		}
	}
	
	if (idxStore && idxStore->at(sr.cellIndexPtr()) != r.cells()) {
		std::cout << "cell-list of region " << i << " differs" << std::endl;
		return false;
	}
	if (idxStore) {
		std::set<uint32_t> tmp;
		for(uint32_t cellId : r.cells()) {
			const Cell & c = m_cells.at(cellId);
			tmp.insert(c.itemsBegin(), c.itemsEnd());
		}
		if (idxStore->at(sr.itemsPtr()) != tmp) {
			std::cout << "Items of region " << i << " differs" << std::endl;
			ok = false;
		}
	}
	
	if (r.boundary != sr.boundary()) {
		std::cout << "Boundary of region " << i << " differs" << std::endl;
		ok = false;
	}
	
	return ok;
}

bool GeoHierarchy::testEquality(const sserialize::Static::spatial::GeoHierarchy& sgh, const sserialize::Static::ItemIndexStore* idxStore) const {
	if (m_regions.size() != sgh.regionSize()) {
		std::cout << "Region size missmatch" << std::endl;
		return false;
	}
	if (m_cells.size() != sgh.cellSize()) {
		std::cout << "Cell size missmatch" << std::endl;
		return false;
	}
	bool allOk = true;
	
	{
		if (!regionEqTest(std::numeric_limits<uint32_t>::max(), m_rootRegion, sgh.rootRegion(), idxStore)) {
			std::cout << "Root region missmatch" << std::endl;
			allOk = false;
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		sserialize::Static::spatial::GeoHierarchy::Region sr = sgh.region(i);
		if (!regionEqTest(i, r, sr, idxStore)) {
			allOk = false;
			break;
		}
	}
	//cells
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		bool ok = true;
		const Cell & c = m_cells.at(i);
		sserialize::Static::spatial::GeoHierarchy::Cell sc = sgh.cell(i);
		if (c.parentsSize() != sc.parentsSize()) {
			std::cout << "parent.size of cell " << i << " differ" << std::endl;
			ok = false;
		}
		std::vector<uint32_t> tmp(c.parentsSize());
		for(uint32_t j = 0, js = c.parentsSize(); j < js; ++j) {
			tmp[j] = sc.parent(j);
		}
		std::sort(tmp.begin(), tmp.end());
		for(uint32_t j = 0, js = c.parentsSize(); j < js; ++j) {
			if (c.parentAt(j) != tmp.at(j)) {
				std::cout << "parent " << j << " of cell " << i << " differs" << std::endl;
				ok = false;
			}
		}
		if (idxStore && idxStore->at(sc.itemPtr()) != c.items()) {
			std::cout << "ItemIndex of cell " << i << " differs" << std::endl;
			ok = false;
		}
		if (c.boundary() != sc.boundary()) {
			std::cout << "Boundary of cell " << i << " differs" << std::endl;
			ok = false;
		}

		if (!ok) {
			allOk = false;
			break;
		}
	}
	return allOk;
}

std::vector<uint32_t> GeoHierarchy::getRegionsInLevelOrder() const {
	if (!m_regions.size()) {
		return std::vector<uint32_t>();
	}
	std::vector<uint32_t> d(m_regions.size(), 0);
	for(uint32_t child : m_rootRegion.children()) {
		d[child] = 1;
	}
	for(uint32_t child : m_rootRegion.children()) {
		depths(d, child);
	}
	std::vector<uint32_t>::const_iterator maxElem = std::max_element(d.cbegin(), d.cend());
	std::vector< std::vector<uint32_t> > regionsInLevels(*maxElem+1);
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		regionsInLevels.at(d[i]).push_back(i);
	}
	d.clear();
	for(std::vector< std::vector<uint32_t> >::iterator it(regionsInLevels.begin()), end(regionsInLevels.end()); it != end; ++it) {
		d.insert(d.end(), it->cbegin(), it->cend());
	}
	return d;
}

std::vector<uint32_t> treeMerge(const std::vector< CFLArray<uint32_t> > & v, uint32_t begin, uint32_t end) {
	if (begin == end)
		return std::vector<uint32_t>(v[begin].cbegin(), v[begin].cend());
	else {
		std::vector<uint32_t> tmp;
		if (end-begin == 1) {
			sserialize::mergeSortedContainer(tmp, v[begin], v[end]);
		}
		else {
			sserialize::mergeSortedContainer(tmp, treeMerge(v, begin, begin+(end-begin)/2), treeMerge(v, begin+(end-begin)/2+1, end));
		}
		return tmp;
	}
}

std::vector< std::pair<uint32_t, uint32_t> > GeoHierarchy::createFullRegionItemIndex(sserialize::ItemIndexFactory& idxFactory) const {
	std::vector< std::pair<uint32_t, uint32_t> > res;
	std::vector< CFLArray<uint32_t> > tmp;
	std::vector<uint32_t> items;
	res.resize(m_regions.size()+1, std::pair<uint32_t, uint32_t>(0,0));
	//if i == m_regions.size() then calculate the index for the rootRegion
	for(uint32_t i = 0, s = m_regions.size(); i <= s; ++i) {
		const Region * r = (i < s ? &m_regions[i] : &m_rootRegion);
		tmp.clear();
		tmp.reserve(r->cellsSize());
		for(uint32_t c : r->cells()) {
			tmp.push_back(cell(c).items());
		}
		items.clear();
		if (tmp.size()) {
			items = treeMerge(tmp, 0, tmp.size()-1);
		}
		res.at(i) = std::pair<uint32_t, uint32_t>(idxFactory.addIndex(items), items.size());
	}
	return res;
}

void GeoHierarchy::compactify(bool compactifyCells, bool compactifyRegions) {
	if (compactifyCells) {
		std::unordered_set<uint32_t> removedParents;
		std::vector<uint32_t> keptParents;
		for(Cell & cell : m_cells) {
			removedParents.clear();
			keptParents.clear();
			for(uint32_t cellParent : cell.parents()) {
				if (removedParents.count(cellParent))
					continue;
				keptParents.push_back(cellParent);
				getAncestors(cellParent, removedParents);
			}
			uint32_t * pIt = cell.parentsBegin();
			for(uint32_t x : keptParents) {
				*pIt = x;
				++*pIt;
			}
			cell.m_parentsSize = pIt - cell.parentsBegin();
		}
	}
	if (compactifyRegions) {
		std::vector<uint32_t> regionsInLevelOrder( getRegionsInLevelOrder() );
		//TODO: do what?
	}
}

}} //end namespace
