#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_H
#include <sserialize/Static/TriangulationGridLocator.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/spatial/TreedCQR.h>

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
private:
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
	uint32_t cellIdFromFaceId(uint32_t faceId) const;
	///start needs to be within the triangulation
	sserialize::ItemIndex cellsBetween(const Point & start, const Point & end, double radius) const;
	///at least one point needs to be within the triangulation
	sserialize::ItemIndex cellsAlongPath(double radius, const Point * begin, const Point * end) const;
	inline sserialize::ItemIndex cellsAlongPath(double radius, const std::vector<Point>::const_iterator & begin, const std::vector<Point>::const_iterator & end) const {
		const Point * myBegin = &(*begin);
		const Point * myEnd = &(*end);
		return this->cellsAlongPath(radius, myBegin, myEnd);
	}
	inline sserialize::ItemIndex cellsAlongPath(double radius, const std::vector<sserialize::spatial::GeoPoint>::iterator & begin, const std::vector<sserialize::spatial::GeoPoint>::iterator & end) const {
		const sserialize::spatial::GeoPoint * myBegin = &(*begin);
		const sserialize::spatial::GeoPoint * myEnd = &(*end);
		return this->cellsAlongPath(radius, myBegin, myEnd);
	}
	template<typename T_GEOPOINT_ITERATOR>
	inline sserialize::ItemIndex cellsAlongPath(double radius, const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const {
		std::vector<sserialize::spatial::GeoPoint> tmp(begin, end);
		const sserialize::spatial::GeoPoint * myBegin = &(*tmp.begin());
		const sserialize::spatial::GeoPoint * myEnd = &(*tmp.end());
		return cellsAlongPath(radius, myBegin, myEnd);
	}
};


}}}//end namespace

#endif