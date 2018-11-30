#ifndef SSERIALIZE_SPATIAL_HCQR_H
#define SSERIALIZE_SPATIAL_HCQR_H
#include <sserialize/spatial/CellQueryResult.h>

namespace sserialize {
namespace spatial {

class HCQRImp;

class HCQR {
public:
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
	using CellInfo = sserialize::CellQueryResult::CellInfo;
public:
	HCQR();
	HCQR(const HCQR & hcqr);
	HCQR(const CellQueryResult & cqr);
public:
	const CellInfo & cellInfo() const;
	const ItemIndexStore & store() const;
public:
	uint32_t maxItems() const;
	sserialize::ItemIndex::Types defaultIndexType() const;
public:
	bool operator!=(const HCQR & other) const;
	bool operator==(const HCQR & other) const;
public:
	///unification
	HCQR operator+(const HCQR & other) const;
	///difference
	HCQR operator-(const HCQR & other) const;
	///intersection
	HCQR operator/(const HCQR & other) const;
public:
	HCQR allToFull() const;
public:
	ItemIndex flaten() const;
	ItemIndex topK(uint32_t numItems) const;
public:
	void dump(std::ostream & out) const;
	void dump() const;
protected:
	const RCPtrWrapper<HCQRImp> & priv() const;
	RCPtrWrapper<HCQRImp> & priv();
private:
	RCPtrWrapper<HCQRImp> m_priv;
};

}} //end namespace sserialize::spatial

#endif 
