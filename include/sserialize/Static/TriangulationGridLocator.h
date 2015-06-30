#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GRID_LOCATOR_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GRID_LOCATOR_H
#include <sserialize/Static/Triangulation.h>
#include <sserialize/Static/RGeoGrid.h>

#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GRID_LOCATOR_VERSION 1

namespace sserialize {
namespace Static {
namespace spatial {
/**
  * {
  *   Version          u8
  *   Grid             sserialize::Static::spatial::RGeoGrid<uint32_t>
  *   Triangulation    sserialize::Static::spatial::Triangulation
  *
  * }
  *
  *
  */

class TriangulationGridLocator final {
public:
	typedef sserialize::Static::spatial::Triangulation Triangulation;
	typedef Triangulation::Face Face;
	typedef Triangulation::Point Point;
	static constexpr uint32_t NullFace = Triangulation::NullFace;
private:
	sserialize::Static::spatial::RGeoGrid<uint32_t> MyGrid;
private:
	sserialize::Static::spatial::RGeoGrid<uint32_t> m_grid;
	Triangulation m_trs;
public:
	TriangulationGridLocator();
	TriangulationGridLocator(const sserialize::UByteArrayAdapter & d);
	~TriangulationGridLocator();
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	bool gridContains(double lat, double lon) const;
	bool gridContains(const Point & p) const;
	bool contains(double lat, double lon) const;
	bool contains(const Point & p) const;
	uint32_t faceId(double lat, double lon) const;
	uint32_t faceId(const Point & p) const;
	Face face(double lat, double lon) const;
	Face face(const Point & p) const;
};

}}}//end namespace

#endif