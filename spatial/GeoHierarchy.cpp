#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRegLine.h>
#include <sserialize/Static/GeoHierarchy.h>

namespace sserialize {
namespace spatial {

bool GeoHierarchy::append(sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory& idxFactory) const {
	bool allOk = true;
	dest.putUint8(3); //version

	std::vector<uint32_t> maxValues(sserialize::Static::spatial::GeoHierarchy::Region::RD__ENTRY_SIZE, 0);
	uint32_t & mrdCellListIndexPtr = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR];
	uint32_t & mrdChildrenBegin = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN];
	uint32_t & mrdId= maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_ID];
	uint32_t & mrdParentsOffset = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET];
	uint32_t & mrdType= maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE];
	
	uint32_t curPtrOffset = 0;
	
	std::vector<uint32_t> indexPtrs(m_regions.size(), 0);
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions[i];
		bool ok = true;
		uint32_t cellListIndexPtr = idxFactory.addIndex(r.cells, &ok);
		mrdCellListIndexPtr = std::max<uint32_t>(mrdCellListIndexPtr, cellListIndexPtr);
		mrdType = std::max<uint32_t>(mrdType, r.type);
		mrdId = std::max<uint32_t>(mrdId, r.id);
		mrdParentsOffset = std::max<uint32_t>(mrdParentsOffset, r.parents.size());
		curPtrOffset += r.children.size() + r.parents.size();
		indexPtrs[i] = cellListIndexPtr;
		allOk = allOk && ok;
	}
	mrdChildrenBegin = curPtrOffset;
	
	std::vector<uint8_t> bitConfig;
	for(uint32_t i = 0, s = maxValues.size(); i < s; ++i) {
		bitConfig.push_back( CompactUintArray::minStorageBits(maxValues[i]) );
	}
	curPtrOffset = 0;
	
	{
		std::vector<uint8_t> tmp;
		sserialize::UByteArrayAdapter tmpDest(&tmp, false);
		MultiVarBitArrayCreator mvaCreator(bitConfig, tmpDest);
		mvaCreator.reserve(m_regions.size());
		for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
			const Region & r = m_regions[i];
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR, indexPtrs[i]);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE, r.type);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_ID, r.id);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET, r.parents.size());
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN, curPtrOffset);
			curPtrOffset += r.children.size() + r.parents.size();
		}
		
		mvaCreator.flush();
		dest.put(tmp);
	}
	
	std::vector<uint32_t> ptrOffsetArray;
	ptrOffsetArray.reserve(curPtrOffset);
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		ptrOffsetArray.insert(ptrOffsetArray.end(), m_regions[i].children.begin(), m_regions[i].children.end());
		ptrOffsetArray.insert(ptrOffsetArray.end(), m_regions[i].parents.begin(), m_regions[i].parents.end());
	}
	
	allOk = allOk && (BoundedCompactUintArray::create(ptrOffsetArray, dest) > 0);
	
	//and do the same for the cells
	
	maxValues.resize(sserialize::Static::spatial::GeoHierarchy::Cell::CD__ENTRY_SIZE, 0);
	indexPtrs.clear();
	indexPtrs.resize(m_cells.size(), 0);
	uint32_t & mcdItemPtr = maxValues[sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_PTR];
	uint32_t & mcdParentBegin = maxValues[sserialize::Static::spatial::GeoHierarchy::Cell::CD_PARENTS_BEGIN];
	
	curPtrOffset = 0;
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells[i];
		bool ok = true;
		uint32_t itemPtr = idxFactory.addIndex(c.items, &ok);
		mcdItemPtr = std::max<uint32_t>(mcdItemPtr, itemPtr);
		curPtrOffset += c.parents.size();
		indexPtrs[i] = itemPtr;
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
		mvaCreator.reserve(m_cells.size());
		for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
			const Cell & c = m_cells[i];
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Cell::CD_ITEM_PTR, indexPtrs[i]);
			mvaCreator.set(i, sserialize::Static::spatial::GeoHierarchy::Cell::CD_PARENTS_BEGIN, curPtrOffset);
			curPtrOffset += c.parents.size();
		}
		
		mvaCreator.flush();
		dest.put(tmp);
	}
	
	ptrOffsetArray.clear();
	ptrOffsetArray.reserve(curPtrOffset);
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		ptrOffsetArray.insert(ptrOffsetArray.end(), m_cells[i].parents.begin(), m_cells[i].parents.end());
	}
	
	allOk = allOk && (BoundedCompactUintArray::create(ptrOffsetArray, dest) > 0);
	
	return allOk;
}

}} //end namespace
