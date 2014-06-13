#ifndef SSERIALIZE_SPATIAL_GEO_GRID_H
#define SSERIALIZE_SPATIAL_GEO_GRID_H
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/spatial/GeoPoint.h>
#include <stdint.h>

namespace sserialize {
namespace spatial {

class GeoGrid {
public:
	struct GridBin {
		GridBin() : x(0xFFFFFFFF), y(0xFFFFFFFF), tile(0xFFFFFFFF) {}
		GridBin(uint32_t x, uint32_t y, uint32_t tile) : x(x), y(y), tile(tile) {}
		uint32_t x;
		uint32_t y;
		uint32_t tile;
		inline bool valid() const { return tile != 0xFFFFFFFF;}
	};
private:
	GeoRect m_rect;
	uint32_t m_latcount;
	uint32_t m_loncount;
	double m_latStep;
	double m_lonStep;
protected:
	void setGridInfo(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount);
	
public:
	GeoGrid() : m_latcount(0), m_loncount(0), m_latStep(0), m_lonStep(0) {}
	GeoGrid(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount);
	virtual ~GeoGrid() {}
	inline uint32_t tileCount() const { return m_latcount*m_loncount;}
	
	inline const GeoRect & rect() const { return m_rect; }
	inline GeoRect & rect() { return m_rect; }
	inline uint32_t latCount() const { return m_latcount; }
	inline uint32_t lonCount() const { return m_loncount; }
	inline double latStep() const { return m_latStep; }
	inline double lonStep() const { return m_lonStep; }
	
	GeoRect cellBoundary(uint32_t lat, uint32_t lon) const;
	inline GeoRect cellBoundary(const GridBin & gridBin) const { return cellBoundary(gridBin.x, gridBin.y);}
	///inclusive xmin inclusive xmax => multiCellBoundary(x,y,x,y) == cellBoundary(x,y)
	GeoRect multiCellBoundary(uint32_t latmin, uint32_t lonmin, uint32_t latmax, uint32_t lonmax) const;
	
	inline bool contains(const double lat, const double lon) const { return m_rect.contains(lat, lon); }
	inline bool contains(const GeoPoint & point) const { return contains(point.lat(), point.lon()); }
	inline GridBin select(const spatial::GeoPoint & point) const { return select(point.lat(), point.lon()); }
	
	GridBin select(double lat, double lon) const;
	
	/** This does NOT! check if the given coords exisit */
	inline uint32_t selectBin(uint32_t lat, uint32_t lon) const { return lon*m_latcount+lat; }
	
	GridBin select(uint32_t tile) const;
	
	/** @return all GridBins intersecting rect */
	std::vector<GridBin> select(GeoRect rect);
	
	bool select(GeoRect rect, GeoGrid::GridBin & bottomLeft,GeoGrid::GridBin & topRight) const;
	
	/** @param enclosed: tiles that are completelty within rect, @param intersected: tiles that intersect, but are not enclosed */
	void select(const GeoRect & rect, std::vector<GridBin> & enclosed, std::vector<GridBin> & intersected) const;
	
	/** @param enclosed: tiles that are completelty within rect, @param intersected: tiles that intersect, but are not enclosed */
	void select(const GeoRect & rect, std::vector<uint32_t> & enclosed, std::vector<uint32_t> & intersected) const;
};

}}

#endif