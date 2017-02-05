#ifndef SSERIALIZE_SPATIAL_HCQR_H
#define SSERIALIZE_SPATIAL_HCQR_H
#include <sserialize/spatial/CellQueryResult.h>

namespace sserialize {
namespace spatial {

class HCQRImp;

class HCQR {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
public:
	HCQR();
	HCQR(const CellQueryResult & cqr);
public:
	HCQR operator+(const HCQR & other) const;
	HCQR operator-(const HCQR & other) const;
	HCQR operator/(const HCQR & other) const;
public:
	uint32_t maxItems() const;
	sserialize::ItemIndex::Types defaultIndexType() const;
public:
	HCQR allToFull() const;
public:
	bool operator!=(const HCQR & other) const;
	bool operator==(const HCQR & other) const;
public:
	ItemIndex flaten() const;
	ItemIndex topK(uint32_t numItems) const;
public:
	void dump(std::ostream & out) const;
	void dump() const;
public:
	const GeoHierarchy & gh() const;
	const ItemIndexStore & store() const;
protected:
	const RCPtrWrapper<HCQRImp> & priv() const;
	RCPtrWrapper<HCQRImp> & priv();
private:
	RCPtrWrapper<HCQRImp> m_priv;
};

}} //end namespace sserialize::spatial

#endif