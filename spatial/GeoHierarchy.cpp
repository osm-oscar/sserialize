#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRegLine.h>
#include <sserialize/Static/GeoHierarchy.h>

namespace sserialize {
namespace spatial {

bool GeoHierarchy::append(sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory& idxFactory) const {
	bool allOk = true;
	dest.putUint8(2); //version
	sserialize::Static::DequeCreator<UByteArrayAdapter> cdc(dest);
	for(uint32_t i = 0, s = m_cells.size(); i < s; ++i) {
		const Cell & c = m_cells[i];
		bool ok = true;
		cdc.beginRawPut();
		idxFactory.addIndex(c.items, &ok);
		uint32_t itemIndexPtr = ItemIndexPrivateRegLine::create(c.parents, cdc.rawPut(), -1, true);
		cdc.rawPut().putVlPackedUint32(itemIndexPtr);
		ItemIndexPrivateRegLine::create(c.parents, cdc.rawPut(), -1, true);
		cdc.endRawPut();
		allOk = allOk && ok;
	}
	cdc.flush();
	
// 	std::vector< std::pair<uint32_t, uint32_t> > childrenBeginParentsOffsets;
	
	
	std::vector<uint32_t> maxValues(sserialize::Static::spatial::GeoHierarchy::Region::RD__ENTRY_SIZE, 0);
	uint32_t & mvCellListIndexPtr = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_CELL_LIST_PTR];
	uint32_t & mvChildrenBegin = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_CHILDREN_BEGIN];
	uint32_t & mvId= maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_ID];
	uint32_t & mvParentsOffset = maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_PARENTS_OFFSET];
	uint32_t & mvType= maxValues[sserialize::Static::spatial::GeoHierarchy::Region::RD_TYPE];
	
	uint32_t curPtrOffset = 0;
	
	std::vector<uint32_t> indexPtrs(m_regions.size(), 0);
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions[i];
		bool ok = true;
		uint32_t cellListIndexPtr = idxFactory.addIndex(r.cells, &ok);
		mvCellListIndexPtr = std::max<uint32_t>(mvCellListIndexPtr, cellListIndexPtr);
		mvType = std::max<uint32_t>(mvType, r.type);
		mvId = std::max<uint32_t>(mvId, r.id);
		mvParentsOffset = std::max<uint32_t>(mvParentsOffset, r.parents.size());
		mvChildrenBegin = std::max<uint32_t>(mvChildrenBegin, curPtrOffset);
		curPtrOffset += r.children.size() + r.parents.size();
		indexPtrs[i] = cellListIndexPtr;
		allOk = allOk && ok;
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
	
	return allOk;
}

}} //end namespace
