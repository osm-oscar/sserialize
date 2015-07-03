#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_H
#include <sserialize/Static/TriangulationGridLocator.h>
#include <sserialize/utility/CompactUintArray.h>

#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_VERSION 1

namespace sserialize {
namespace Static {
namespace spatial {

/** A triangulation based locator usable with sserialize::Static::spatial::GeoHierarchy
  * {
  *   Version                  u8
  *   CellCount                u32
  *   Grid                     TriangulationGridLocator
  *   FaceIdToRefinedCellId    BoundedCompactUintArray
  * }
  *
  * By definition:
  * Triangles that are in no cell of the Geohierarchy get a cellId of RefinedCellToBaseCell.size()
  * This has to be converted to NullCellId upon returning
  *
  */

class TriangulationGeoHierarchyArrangement final {
public:
	typedef TriangulationGridLocator::Triangulation Triangulation;
	typedef Triangulation::Point Point;
	static constexpr uint32_t NullCellId = 0xFFFFFFFF;
private:
	uint32_t m_cellCount;
	TriangulationGridLocator m_grid;
	BoundedCompactUintArray m_faceIdToRefinedCellId;
public:
	TriangulationGeoHierarchyArrangement();
	TriangulationGeoHierarchyArrangement(const sserialize::UByteArrayAdapter & d);
	~TriangulationGeoHierarchyArrangement();
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	inline uint32_t cellCount() const { return m_cellCount; }
	inline const TriangulationGridLocator & grid() const { return m_grid; }
	inline const Triangulation & tds() const { return m_grid.tds(); }
	uint32_t cellId(double lat, double lon) const;
	uint32_t cellId(const Point & p) const;
};


}}}//end namespace

#endif