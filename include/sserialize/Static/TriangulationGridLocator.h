#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GRID_LOCATOR_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GRID_LOCATOR_H
#include <sserialize/Static/Triangulation.h>
#include <sserialize/Static/RGeoGrid.h>

#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GRID_LOCATOR_VERSION 2

namespace sserialize {
namespace Static {
namespace spatial {
/**
  * {
  *   Version          u8
  *   Triangulation    sserialize::Static::spatial::Triangulation
  *   Grid             sserialize::Static::spatial::RGeoGrid<sserialize::Static::spatial::Triangulation::FaceId>
  *
  * }
  *
  * Grid: grid-cell -> FaceId
  *
  */

class TriangulationGridLocator final {
public:
	using Triangulation = sserialize::Static::spatial::Triangulation;
	using Face = Triangulation::Face;
	using Point = Triangulation::Point;
	using FaceId = Triangulation::FaceId;
	using VertexId = Triangulation::VertexId;
	using SizeType = Triangulation::SizeType;
	using Grid = sserialize::Static::spatial::RGeoGrid<sserialize::Static::spatial::Triangulation::FaceId>;
	static constexpr FaceId NullFace = Triangulation::NullFace;
private:
	Triangulation m_trs;
	Grid m_grid;
public:
	TriangulationGridLocator();
	TriangulationGridLocator(const sserialize::UByteArrayAdapter & d);
	~TriangulationGridLocator();
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	inline const Grid & grid() const { return m_grid; }
	inline const Triangulation & tds() const { return m_trs; }
	FaceId faceHint(double lat, double lon) const;
	FaceId faceHint(const Point & p) const;
	bool gridContains(double lat, double lon) const;
	bool gridContains(const Point & p) const;
	bool contains(double lat, double lon) const;
	bool contains(const Point & p) const;
	FaceId faceId(double lat, double lon) const;
	FaceId faceId(const Point & p) const;
	Face face(double lat, double lon) const;
	Face face(const Point & p) const;
};

}}}//end namespace

#endif
