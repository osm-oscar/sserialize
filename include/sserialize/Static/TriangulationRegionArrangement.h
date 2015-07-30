#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_REGION_ARRANGEMENT_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_REGION_ARRANGEMENT_H
#include <sserialize/Static/TriangulationGridLocator.h>
#include <sserialize/containers/CompactUintArray.h>

#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_REGION_ARRANGEMENT_VERSION 1

namespace sserialize {
namespace Static {
namespace spatial {

/**
  * {
  *   Version               u8
  *   Grid                  TriangulationGridLocator
  *   RegionListIndexPtrs   BoundedCompactUintArray
  *   RefinedCellToBaseCell BoundedCompactUintArray
  *   FaceIdToRefinedCellId BoundedCompactUintArray
  * }
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
public:
	TriangulationRegionArrangement();
	TriangulationRegionArrangement(const sserialize::UByteArrayAdapter & d);
	~TriangulationRegionArrangement();
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t baseCellId(uint32_t cellId) const;
	uint32_t cellId(double lat, double lon) const;
	uint32_t cellId(const Point & p) const;
	uint32_t regionListPtr(uint32_t cellId) const;
	inline uint32_t cellCount() const { return m_refinedToBaseCellId.size(); }
};


}}}//end namespace

#endif