#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRegLine.h>

namespace sserialize {
namespace spatial {

bool GeoHierarchy::append(sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory& idxFactory) const {
	bool allOk = true;
	dest.putUint8(1); //version
	sserialize::Static::DequeCreator<UByteArrayAdapter> rdc(dest);
	for(uint32_t i = 0, s = m_regions.size(); i < s; ++i) {
		const Region & r = m_regions[i];
		bool ok = true;
		rdc.beginRawPut();
		uint32_t cellListIndexPtr = idxFactory.addIndex(r.cells, &ok);
		rdc.rawPut().putVlPackedUint32(cellListIndexPtr);
		rdc.rawPut().putUint8(r.type);
		rdc.rawPut().putVlPackedUint32( r.parent );
		rdc.rawPut().putVlPackedUint32(r.id);
		ItemIndexPrivateRegLine::create(r.children, rdc.rawPut(), -1, true);
		rdc.endRawPut();
		allOk = allOk && ok;
	}
	rdc.flush();
	
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
	
	return allOk;
}

}} //end namespace
