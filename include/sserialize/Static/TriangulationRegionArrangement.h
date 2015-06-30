#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_REGION_ARRANGEMENT_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_REGION_ARRANGEMENT_H
#include <sserialize/Static/TriangulationGridLocator.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/CompactUintArray.h>

#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_REGION_ARRANGEMENT_VERSION 1

namespace sserialize {
namespace Static {
namespace spatial {

/**
  * {
  *   Version               u8
  *   Grid                  TriangulationGridLocator
  *   BaseCellIndexPtrs     BoundedCompactUintArray
  *   RefinedCellToBaseCell BoundedCompactUintArray
  *   FaceIdToRefinedCellId BoundedCompactUintArray
  * }
  * external: ItemIndexStore holding the BaseCell->Regions mapping
  *
  */

class TriangulationRegionArrangement final {
public:
	typedef TriangulationGridLocator::Triangulation Triangulation;
	typedef Triangulation::Point Point;
	static constexpr uint32_t NullCellId = 0xFFFFFFFF;
private:
	TriangulationGridLocator m_grid;
	BoundedCompactUintArray m_baseCellIdToIndexPtr;
	BoundedCompactUintArray m_refinedToBaseCellId;
	BoundedCompactUintArray m_faceIdToRefinedCellId;
	ItemIndexStore m_idxStore;
public:
	TriangulationRegionArrangement();
	TriangulationRegionArrangement(const sserialize::UByteArrayAdapter & d, const sserialize::Static::ItemIndexStore & idxStore);
	~TriangulationRegionArrangement();
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t baseCellId(uint32_t cellId) const;
	uint32_t cellId(double lat, double lon) const;
	uint32_t cellId(const Point & p) const;
	sserialize::ItemIndex regions(uint32_t cellId) const;
};


}}}//end namespace

#endif