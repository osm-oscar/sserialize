#ifndef SSERIALIZE_SPATIAL_HCQR_IMP_H
#define SSERIALIZE_SPATIAL_HCQR_IMP_H
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>

namespace sserialize {
namespace spatial {


/* we need a mapping from region -> (full region, full cells, partial cells)
 * It has to support fast query 
 * 
 */

class HCQRImp {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
public:
	HCQRImp();
	HCQRImp(const CellQueryResult & cqr);
	virtual ~HCQRImp();
public:
	bool operator!=(const HCQRImp * other) const;
	bool operator==(const HCQRImp * other) const;
public:
	uint32_t maxItems() const;
	sserialize::ItemIndex::Types defaultIndexType() const;
public:
	HCQRImp * unite(const HCQRImp * other) const;
	HCQRImp * difference(const HCQRImp * other) const;
	HCQRImp * intersect(const HCQRImp * other) const;
	HCQRImp * allToFull() const;
public:
	ItemIndex flaten() const;
	ItemIndex topK(uint32_t numItems) const;
public:
	void dump(std::ostream & out) const;
	void dump() const;
public:
	const GeoHierarchy & gh() const;
	const ItemIndexStore & store() const;
private:
	ItemIndexStore m_store;
	GeoHierarchy m_gh;
	
};

}} //end namespace sserialize::spatial

#endif