#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_REGION_ARRANGEMENT_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_REGION_ARRANGEMENT_H
#include <sserialize/Static/TriangulationGridLocator.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/Static/ItemIndexStore.h>

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
	using cellid_type = uint32_t;
	using indexid_type = sserialize::Static::ItemIndexStore::IdType;
	static constexpr cellid_type NullCellId = std::numeric_limits<cellid_type>::max();
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
	cellid_type baseCellId(cellid_type cellId) const;
	cellid_type cellId(double lat, double lon) const;
	cellid_type cellId(const Point & p) const;
	indexid_type regionListPtr(uint32_t cellId) const;
	inline auto cellCount() const { return m_refinedToBaseCellId.size(); }
};


}}}//end namespace

#endif