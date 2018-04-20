#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_H
#include <sserialize/Static/TriangulationGridLocator.h>
#include <sserialize/Static/TRACFGraph.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/spatial/TreedCQR.h>

#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_VERSION 2

namespace sserialize {
namespace Static {
namespace spatial {

/** A triangulation based locator usable with sserialize::Static::spatial::GeoHierarchy
  * {
  *   Version                  u8
  *   Grid                     TriangulationGridLocator
  *   FaceIdToRefinedCellId    BoundedCompactUintArray
  *   RefinedCellIdToFaceId    BoundedCompactUintArray
  * }
  *
  * Changelog:
  * v2: add cell->face mapping, remove cellCount
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
	typedef TRACFGraph<TriangulationGeoHierarchyArrangement> CFGraph;
	static constexpr uint32_t NullCellId = 0xFFFFFFFF;
private:
	TriangulationGridLocator m_grid;
	BoundedCompactUintArray m_faceIdToRefinedCellId;
	BoundedCompactUintArray m_refinedCellIdToFaceId;
public:
	TriangulationGeoHierarchyArrangement();
	TriangulationGeoHierarchyArrangement(const sserialize::UByteArrayAdapter & d);
	~TriangulationGeoHierarchyArrangement();
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	inline uint32_t cellCount() const { return m_refinedCellIdToFaceId.size(); }
	inline const TriangulationGridLocator & grid() const { return m_grid; }
	inline const Triangulation & tds() const { return m_grid.tds(); }
	uint32_t cellId(double lat, double lon) const;
	uint32_t cellId(const Point & p) const;
	uint32_t cellIdFromFaceId(uint32_t faceId) const;
	///returns the id of a face that is part of the cell with id=cellId
	uint32_t faceIdFromCellId(uint32_t cellId) const;
	CFGraph cfGraph(uint32_t cellId) const;
	///start needs to be within the triangulation
	sserialize::ItemIndex cellsBetween(const Point & start, const Point & end, double radius) const;
	///at least one point needs to be within the triangulation, if radius==0.0 then a faster triangulation walk will be used
	sserialize::ItemIndex cellsAlongPath(double radius, const Point * begin, const Point * end) const;
	sserialize::ItemIndex trianglesAlongPath(const Point * begin, const Point * end) const;
	
	sserialize::ItemIndex cellsAlongPath(double radius, const std::vector<Point>::const_iterator & begin, const std::vector<Point>::const_iterator & end) const;
	sserialize::ItemIndex cellsAlongPath(double radius, const std::vector<sserialize::spatial::GeoPoint>::iterator & begin, const std::vector<sserialize::spatial::GeoPoint>::iterator & end) const;
	template<typename T_GEOPOINT_ITERATOR>
	inline sserialize::ItemIndex cellsAlongPath(double radius, const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const;
	
	sserialize::ItemIndex trianglesAlongPath(const std::vector<Point>::const_iterator & begin, const std::vector<Point>::const_iterator & end) const;
	sserialize::ItemIndex trianglesAlongPath(const std::vector<sserialize::spatial::GeoPoint>::iterator & begin, const std::vector<sserialize::spatial::GeoPoint>::iterator & end) const;
	template<typename T_GEOPOINT_ITERATOR>
	inline sserialize::ItemIndex trianglesAlongPath(const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const;
};


template<typename T_GEOPOINT_ITERATOR>
sserialize::ItemIndex TriangulationGeoHierarchyArrangement::cellsAlongPath(double /*radius*/, const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const {
	std::vector<sserialize::spatial::GeoPoint> tmp(begin, end);
	const sserialize::spatial::GeoPoint * myBegin = &(*tmp.begin());
	const sserialize::spatial::GeoPoint * myEnd = &(*tmp.end());
	return trianglesAlongPath(myBegin, myEnd);
}

template<typename T_GEOPOINT_ITERATOR>
sserialize::ItemIndex TriangulationGeoHierarchyArrangement::trianglesAlongPath(const T_GEOPOINT_ITERATOR& begin, const T_GEOPOINT_ITERATOR& end) const {
	std::vector<sserialize::spatial::GeoPoint> tmp(begin, end);
	const sserialize::spatial::GeoPoint * myBegin = &(*tmp.begin());
	const sserialize::spatial::GeoPoint * myEnd = &(*tmp.end());
	return trianglesAlongPath(myBegin, myEnd);
}

}}}//end namespace

#endif
