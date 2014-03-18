#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/utility/log.h>

namespace sserialize {
namespace spatial {

void GeoHierarchy::depths(std::vector<uint32_t> & d, uint32_t me) const {
	for(uint32_t child : m_regions.at(me).children) {
		if (d[child] <= d[me]) {
			d[child] = d[me] + 1;
			depths(d, child);
		}
	}
}

void GeoHierarchy::createRootRegion() {
	m_rootRegion.children.clear();
	m_rootRegion.type = sserialize::spatial::GS_NONE;
	m_rootRegion.id = 0;
	m_rootRegion.parents.clear();
	m_rootRegion.children.clear();
	m_rootRegion.cells.clear();
	
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions[i];
		if (!r.parents.size()) {
			m_rootRegion.children.push_back(i);
		}
	}
	m_rootRegion.cells.reserve(m_cells.size());
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		m_rootRegion.cells.push_back(i);
	}
}

bool GeoHierarchy::checkConsistency() {
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		if (r.children.size() && !std::is_sorted(r.children.cbegin(), r.children.cend())) {
			std::cout << "Region " << i << " has unsorted children" << std::endl;
			return false;
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		if (r.parents.size() && !std::is_sorted(r.parents.cbegin(), r.parents.cend())) {
			std::cout << "Region " << i << " has unsorted parents" << std::endl;
			return false;
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		if (haveCommonValueOrdered(r.parents, r.children)) {
			std::cout << "Region " << i << " has common parent/child: ";
			sserialize::print(std::cout, sserialize::intersect< std::vector<uint32_t> >(r.parents, r.children));
			std::cout << std::endl;
			return false;
		}
	}
	{
		std::vector<uint32_t> regionsWithoutParentChildren;
		for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
			const Region & r = m_regions.at(i);
			if (r.parents.size() == 0 && r.children.size() == 0) {
				regionsWithoutParentChildren.push_back(i);
			}
		}
		std::vector<uint32_t> invalidRegions;
		std::set_difference(regionsWithoutParentChildren.cbegin(), regionsWithoutParentChildren.cend(),
							m_rootRegion.children.cbegin(), m_rootRegion.children.cend(),
							std::back_insert_iterator< std::vector<uint32_t> >(invalidRegions));
		for(uint32_t i : invalidRegions) {
			std::cout << "Region " << i << " has no children and no parents" << std::endl;
		}
		if (invalidRegions.size())
			return false;
	}

	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(const auto x : r.children) {
			const Region & cr = m_regions.at(x);
			if (cr.parents.size() && !std::binary_search(cr.parents.cbegin(), cr.parents.cend(), i)) {
				std::cout << "Region " << x << " has missing parent pointer to " << i << std::endl;
				return false;
			}
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(const auto x : r.children) {
			const Region & cr = m_regions.at(x);
			if (cr.children.size() && std::binary_search(cr.children.cbegin(), cr.children.cend(), i)) {
				std::cout << "Region " << x << " has parent " << i << " as child" << std::endl;
				return false;
			}
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(const auto x : r.children) {
			const Region & cr = m_regions.at(x);
			if (!r.boundary.contains(cr.boundary)) {
				if (!sserialize::subset_of(cr.cells.cbegin(), cr.cells.cend(), r.cells.cbegin(), r.cells.cend())) {
					std::cout << "Region " << i << " does not span child " << x << std::endl;
					return false;
				}
				else {
					std::cout << "Region " << i << " with child " << x << " were not correctly sampled" << std::endl;
				}
			}
		}
	}
	
	//cells
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells.at(i);
		if (!std::is_sorted(c.items.cbegin(), c.items.cend())) {
			std::cout << "Cell " << i << " has unsorted items" << std::endl;
			return false;
		}
	}
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells.at(i);
		if (!std::is_sorted(c.parents.cbegin(), c.parents.cend())) {
			std::cout << "Cell " << i << " has unsorted parents" << std::endl;
			return false;
		}
	}
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		for(const auto x : r.cells) {
			const Cell & c = m_cells.at(x);
			if (!std::binary_search(c.parents.cbegin(), c.parents.cend(), i)) {
				std::cout << "Cell " << x << " has missing parent " << i << std::endl;
				return false;
			}
		}
	}
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells.at(i);
		for(const auto x : c.parents) {
			const Region & r = m_regions.at(x);
			if (!std::binary_search(r.cells.cbegin(), r.cells.cend(), i)) {
				std::cout << "Region " << x << " has missing cell " << i << std::endl;
				return false;
			}
		}
	}
	return true;
}

void GeoHierarchy::printStats(std::ostream & out) const {
	std::vector<Region>::const_iterator maxCellSplitIt = std::max_element(m_regions.cbegin(), m_regions.cend(), [] (const Region & x, const Region & y) {return x.cells.size() < y.cells.size();});
	std::vector<Region>::const_iterator maxChildrenIt = std::max_element(m_regions.cbegin(), m_regions.cend(), [] (const Region & x, const Region & y) {return x.children.size() < y.children.size();});
	std::vector<Region>::const_iterator maxParentsIt = std::max_element(m_regions.cbegin(), m_regions.cend(), [] (const Region & x, const Region & y) {return x.parents.size() < y.parents.size();});
	out << "sserialize::spatial::GeoHierarchy--stats-BEGIN" << std::endl;
	out << "max cellsplit at " << maxCellSplitIt-m_regions.cbegin() << " with " << maxCellSplitIt->cells.size() << std::endl;
	out << "max children at " << maxChildrenIt-m_regions.cbegin() << " with " << maxChildrenIt->children.size() << std::endl;
	out << "max parents at " << maxParentsIt-m_regions.cbegin() << " with " << maxParentsIt->parents.size() << std::endl;
	out << "sserialize::spatial::GeoHierarchy--stats-END" << std::endl;
}

UByteArrayAdapter GeoHierarchy::append(sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory& idxFactory, bool fullItemsIndex) const {
	bool allOk = true;
	UByteArrayAdapter::OffsetType beginOffset = dest.tellPutPtr();
	dest.putUint8(3); //version

	std::vector<uint32_t> maxValues(sserialize::Static::spatial::GeoHierarchy::Region::RD__ENTRY_SIZE, 0);
	uint32_t & mrdCellListIndexPtr = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR];
	uint32_t & mrdChildrenBegin = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN];
	uint32_t & mrdId= maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_ID];
	uint32_t & mrdParentsOffset = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET];
	uint32_t & mrdType= maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE];
	
	uint32_t curPtrOffset = 0;

	std::vector<uint32_t> cellListIndexPtrs(m_regions.size(), 0);
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions[i];
		bool ok = true;
		uint32_t cellListIndexPtr = idxFactory.addIndex(r.cells, &ok);
		mrdCellListIndexPtr = std::max<uint32_t>(mrdCellListIndexPtr, cellListIndexPtr);
		mrdType = std::max<uint32_t>(mrdType, r.type);
		mrdId = std::max<uint32_t>(mrdId, r.id);
		mrdParentsOffset = std::max<uint32_t>(mrdParentsOffset, r.children.size());
		curPtrOffset += r.children.size() + r.parents.size();
		cellListIndexPtrs[i] = cellListIndexPtr;
		allOk = allOk && ok;
	}
	curPtrOffset += m_rootRegion.children.size();
	mrdParentsOffset = std::max<uint32_t>(mrdParentsOffset, m_rootRegion.children.size());
	mrdChildrenBegin = curPtrOffset;

	std::vector<uint32_t> fullItemIndexPtrIds;
	//check for fullItemsPtr
	if (fullItemsIndex) {
		fullItemIndexPtrIds = createFullRegionItemIndex(idxFactory);
		std::vector<uint32_t>::const_iterator maxElem = std::max_element(fullItemIndexPtrIds.cbegin(), fullItemIndexPtrIds.cend());
		maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_PTR] = *maxElem;
	}
	else {
		fullItemIndexPtrIds.resize(m_regions.size(), 0);
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
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE, r.type);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_ID, r.id);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN, curPtrOffset);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET, r.children.size());
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_ITEMS_PTR, fullItemIndexPtrIds[i]);
			curPtrOffset += r.children.size() + r.parents.size();
		}
		//append the root region region
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR, 0);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE, sserialize::spatial::GS_NONE);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_ID, m_rootRegion.id);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN, curPtrOffset);
		mvaCreator.set(m_regions.size(), sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET, m_rootRegion.children.size());
		curPtrOffset += m_rootRegion.children.size();
		
		//append the dummy region which is needed for the offset array
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR, 0);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE, sserialize::spatial::GS_NONE);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_ID, 0);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN, curPtrOffset);
		mvaCreator.set(m_regions.size()+1, sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET, 0);
		
		mvaCreator.flush();
		dest.put(tmp);
	}
	
	std::vector<uint32_t> ptrOffsetArray;
	ptrOffsetArray.reserve(curPtrOffset);
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		ptrOffsetArray.insert(ptrOffsetArray.end(), m_regions[i].children.cbegin(), m_regions[i].children.cend());
		ptrOffsetArray.insert(ptrOffsetArray.end(), m_regions[i].parents.cbegin(), m_regions[i].parents.cend());
	}
	//append the root region
	ptrOffsetArray.insert(ptrOffsetArray.end(), m_rootRegion.children.cbegin(), m_rootRegion.children.cend());
	
	allOk = allOk && (BoundedCompactUintArray::create(ptrOffsetArray, dest) > 0);
	//append the RegionRects
	{
		sserialize::Static::DequeCreator<sserialize::spatial::GeoRect> bCreator(dest);
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
	uint32_t & mcdParentBegin = maxValues[sserialize::Static::spatial::GeoHierarchy::Cell::CD_PARENTS_BEGIN];
	
	curPtrOffset = 0;
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells[i];
		bool ok = true;
		uint32_t itemPtr = idxFactory.addIndex(c.items, &ok);
		mcdItemPtr = std::max<uint32_t>(mcdItemPtr, itemPtr);
		curPtrOffset += c.parents.size();
		cellItemsIndexPtrs[i] = itemPtr;
		allOk = allOk && ok;
	}
	mcdParentBegin = curPtrOffset;
	
	bitConfig.clear();
	for(uint32_t i = 0, s = maxValues.size(); i < s; ++i) {
		bitConfig.push_back( CompactUintArray::minStorageBits(maxValues[i]) );
	}
	curPtrOffset = 0;
	
	{
		std::vector<uint8_t> tmp;
		sserialize::UByteArrayAdapter tmpDest(&tmp, false);
		MultiVarBitArrayCreator mvaCreator(bitConfig, tmpDest);
		mvaCreator.reserve(m_cells.size()+1);
		for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
			const Cell & c = m_cells[i];
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_PTR, cellItemsIndexPtrs[i]);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Cell::CD_PARENTS_BEGIN, curPtrOffset);
			curPtrOffset += c.parents.size();
		}
		mvaCreator.set(m_cells.size(), sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_PTR, 0);
		mvaCreator.set(m_cells.size(), sserialize::Static::spatial::GeoHierarchy::Cell::CD_PARENTS_BEGIN, curPtrOffset);
		
		mvaCreator.flush();
		dest.put(tmp);
	}
	
	ptrOffsetArray.clear();
	ptrOffsetArray.reserve(curPtrOffset);
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		ptrOffsetArray.insert(ptrOffsetArray.end(), m_cells[i].parents.begin(), m_cells[i].parents.end());
	}
	
	allOk = allOk && (BoundedCompactUintArray::create(ptrOffsetArray, dest) > 0);
	
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

bool regionEqTest(uint32_t i, const GeoHierarchy::Region & r, const sserialize::Static::spatial::GeoHierarchy::Region & sr) {
	bool ok = true;
	if (r.children.size() != sr.childrenSize()) {
		std::cout << "Childern.size of region " << i << " differ" << std::endl;
		ok = false;
	}
	if (r.parents.size() != sr.parentsSize()) {
		std::cout << "parent.size of region " << i << " differ" << std::endl;
		ok = false;
	}
	if (r.id != sr.id()) {
		std::cout << "id of region " << i << " differs" << std::endl;
		ok = false;
	}
	if (r.type != sr.type()) {
		std::cout << "type of region " << i << " differs" << std::endl;
		ok = false;
	}
	for(uint32_t j = 0, js = r.children.size(); j < js; ++j) {
		if (r.children.at(j) != sr.child(j)) {
			std::cout << "child " << j << " of region " << i << " differs. Want: " << r.children.at(j) << ", have: " << sr.child(j) << std::endl;
			ok = false;
		}
	}
	for(uint32_t j = 0, js = r.parents.size(); j < js; ++j) {
		if (r.parents.at(j) != sr.parent(j)) {
			std::cout << "parent " << j << " of region " << i << " differs" << std::endl;
			ok = false;
		}
	}
	return ok;
}

bool GeoHierarchy::testEquality(const sserialize::Static::spatial::GeoHierarchy & sgh) const {
	if (m_regions.size() != sgh.regionSize()) {
		std::cout << "Region size missmatch" << std::endl;
		return false;
	}
	if (m_cells.size() != sgh.cellSize()) {
		std::cout << "Cell size missmatch" << std::endl;
		return false;
	}
	
	{
		if (!regionEqTest(std::numeric_limits<uint32_t>::max(), m_rootRegion, sgh.rootRegion())) {
			std::cout << "Root region missmatch" << std::endl;
			return false;
		}
	}
	
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions.at(i);
		sserialize::Static::spatial::GeoHierarchy::Region sr = sgh.region(i);
		if (!regionEqTest(i, r, sr))
			return false;
	}
	//cells
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		bool ok = true;
		const Cell & c = m_cells.at(i);
		sserialize::Static::spatial::GeoHierarchy::Cell sc = sgh.cell(i);
		if (c.parents.size() != sc.parentsSize()) {
			std::cout << "parent.size of cell " << i << " differ" << std::endl;
			ok = false;
		}
		for(uint32_t j = 0, js = c.parents.size(); j < js; ++j) {
			if (c.parents.at(j) != sc.parent(j)) {
				std::cout << "parent " << j << " of cell " << i << " differs" << std::endl;
				ok = false;
			}
		}
		if (!ok)
			return false;
	}
	return true;
}

std::vector<uint32_t> GeoHierarchy::getRegionsInLevelOrder() const {
	if (!m_regions.size()) {
		return std::vector<uint32_t>();
	}
	std::vector<uint32_t> d(m_regions.size(), 0);
	for(uint32_t child : m_rootRegion.children) {
		d[child] = 1;
	}
	for(uint32_t child : m_rootRegion.children) {
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

std::vector<uint32_t> treeMerge(const std::vector< const std::vector<uint32_t>* > & v, uint32_t begin, uint32_t end) {
	if (begin ==  end)
		return *v[begin];
	else {
		std::vector<uint32_t> tmp;
		if (end-begin == 1) {
			sserialize::mergeSortedContainer(tmp, *v[begin], *v[end]);
		}
		else {
			sserialize::mergeSortedContainer(tmp, treeMerge(v, begin, begin+(end-begin)/2), treeMerge(v, begin+(end-begin)/2+1, end));
		}
		return tmp;
	}
}

std::vector<uint32_t> GeoHierarchy::createFullRegionItemIndex(sserialize::ItemIndexFactory& idxFactory) const {
	std::vector<uint32_t> res;
	res.resize(m_regions.size(), 0);
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions[i];
		std::vector< const std::vector<uint32_t> * > tmp;
		tmp.reserve(r.cells.size());
		for(uint32_t c : r.cells) {
			tmp.push_back(&cell(c).items);
		}
		std::vector<uint32_t> items;
		if (tmp.size()) {
			items = treeMerge(tmp, 0, tmp.size()-1);
		}
		res.at(i) = idxFactory.addIndex(items);
	}
	return res;
}

}} //end namespace
