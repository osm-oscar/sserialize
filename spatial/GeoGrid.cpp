#include <sserialize/spatial/GeoGrid.h>

namespace sserialize {
namespace spatial {

void GeoGrid::setGridInfo(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount) {
	m_rect = rect;
	m_latcount = latcount;
	m_loncount = loncount;
	m_latStep = (m_rect.lat()[1]-m_rect.lat()[0])/m_latcount;
	m_lonStep = (m_rect.lon()[1]-m_rect.lon()[0])/m_loncount;
}
	

GeoGrid::GeoGrid(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount) :
m_rect(rect),
m_latcount(latcount),
m_loncount(loncount)
{
	m_latStep = (m_rect.lat()[1]-m_rect.lat()[0])/latcount;
	m_lonStep = (m_rect.lon()[1]-m_rect.lon()[0])/loncount;
}
GeoRect GeoGrid::cellBoundary(uint32_t x, uint32_t y) const {
	return GeoRect(m_rect.lat()[0]+x*m_latStep,
							m_rect.lat()[0]+(x+1)*m_latStep,
							m_rect.lon()[0]+y*m_lonStep,
							m_rect.lon()[0]+(y+1)*m_lonStep);
}
///inclusive xmin inclusive xmax => multiCellBoundary(x,y,x,y) == cellBoundary(x,y)
GeoRect GeoGrid::multiCellBoundary(uint32_t xmin, uint32_t ymin, uint32_t xmax, uint32_t ymax) const {
	return GeoRect(m_rect.lat()[0] + xmin * m_latStep,
							m_rect.lat()[0]+(xmax+1)*m_latStep,
							m_rect.lon()[0]+ ymin*m_lonStep,
							m_rect.lon()[0]+(ymax+1)*m_lonStep);
}

GeoGrid::GridBin GeoGrid::select(double lat, double lon) const {
	if (!m_rect.contains(lat, lon))
		return GridBin();
	double correctedLat = lat-m_rect.lat()[0];
	double correctedLon = lon-m_rect.lon()[0];
	uint32_t x = correctedLat/m_latStep;
	if (x == m_latcount)
		x = m_latcount-1;
	uint32_t y = correctedLon/m_lonStep;
	if (y == m_loncount)
		y = m_loncount-1;
	return GridBin(x,y, selectBin(x,y));
}

/** This does NOT! check if the given coords exisit */
uint32_t GeoGrid::selectBin(uint32_t x, uint32_t y) const {
	return y*m_latcount+x;
}

std::vector<GeoGrid::GridBin> GeoGrid::select(GeoRect rect) {
	if (!rect.clip(m_rect))
		return std::vector<GridBin>();
	GeoGrid::GridBin blBin = GeoGrid::select(rect.lat()[0], rect.lon()[0]);
	GeoGrid::GridBin trBin = GeoGrid::select(rect.lat()[1], rect.lon()[1]);
	if (!blBin.valid() || !trBin.valid())
		return std::vector<GridBin>();
	unsigned int xbinStart = blBin.x;
	unsigned int ybinStart = blBin.y;
	unsigned int xbinEnd = trBin.x;
	unsigned int ybinEnd = trBin.y;
	if (xbinStart == xbinEnd && ybinStart == ybinEnd) { //one cell
		return std::vector<GridBin>(1, trBin);
	}
	else { //polygon spans multiple cells
		std::vector< GridBin > cells;
		cells.reserve((xbinEnd-xbinStart+1)*(ybinEnd-ybinStart+1));
		for(unsigned int i = xbinStart; i <= xbinEnd; i++) {
			for(unsigned int j = ybinStart; j <= ybinEnd; j++) {
				cells.push_back( GridBin(i,j, selectBin(i,j)) );
			}
		}
		return cells;
	}
}

bool GeoGrid::select(GeoRect rect, GeoGrid::GridBin & bottomLeft,GeoGrid::GridBin & topRight) const {
	if (!rect.clip(m_rect))
		return false;
	bottomLeft = GeoGrid::select(rect.lat()[0], rect.lon()[0]);
	topRight = GeoGrid::select(rect.lat()[1], rect.lon()[1]);
	return (!bottomLeft.valid() || !topRight.valid());
}

void GeoGrid::select(const GeoRect & rect, std::vector<GridBin> & enclosed, std::vector<GridBin> & intersected) const {
	GeoGrid::GridBin blBin, trBin;
	select(rect, blBin, trBin);
	unsigned int xbinStart = blBin.x;
	unsigned int ybinStart = blBin.y;
	unsigned int xbinEnd = trBin.x;
	unsigned int ybinEnd = trBin.y;
	if (xbinStart == xbinEnd && ybinStart == ybinEnd) { //one cell
		intersected.push_back(trBin);
		return;
	}
	else { //polygon spans multiple cells
		//enclosed cells
		for(unsigned int i = xbinStart-1; i <= xbinEnd-1; i++) {
			for(unsigned int j = ybinStart-1; j <= ybinEnd-1; j++) {
				enclosed.push_back( GridBin(i,j, selectBin(i,j)) );
			}
		}
		
		//intersected cells, first top and bottom rows
		for(size_t i = xbinStart; i <= xbinEnd; i++) {
			intersected.push_back( GridBin(i,ybinStart, selectBin(i,ybinStart)) );
		}
		if (ybinStart != ybinEnd) {
			for(size_t i = xbinStart; i <= xbinEnd; i++) {
				intersected.push_back( GridBin(i,ybinEnd, selectBin(i,ybinEnd)) );
			}
		}
		
		//now the vertical rows
		if (ybinStart+1 < ybinEnd) {
			//left vertical line
			for(size_t j = ybinStart+1; j <= ybinEnd-1; ++j) {
				intersected.push_back( GridBin(xbinStart,j, selectBin(xbinStart,j)) );
			}
			if (xbinStart != xbinEnd) { //right vertical line
				for(size_t j = ybinStart+1; j <= ybinEnd-1; ++j) {
					intersected.push_back( GridBin(xbinEnd,j, selectBin(xbinEnd,j)) );
				}
			}
		}
	}
}

void GeoGrid::select(const GeoRect & rect, std::vector<uint32_t> & enclosed, std::vector<uint32_t> & intersected) const {
	GeoGrid::GridBin blBin, trBin;
	select(rect, blBin, trBin);
	unsigned int xbinStart = blBin.x;
	unsigned int ybinStart = blBin.y;
	unsigned int xbinEnd = trBin.x;
	unsigned int ybinEnd = trBin.y;
	if (xbinStart == xbinEnd && ybinStart == ybinEnd) { //one cell
		intersected.push_back(trBin.tile);
		return;
	}
	else { //polygon spans multiple cells
		//enclosed cells
		for(unsigned int i = xbinStart+1; i < xbinEnd; i++) {
			for(unsigned int j = ybinStart+1; j < ybinEnd; j++) {
				enclosed.push_back( selectBin(i,j) );
			}
		}
		
		//intersected cells, first top and bottom rows
		for(size_t i = xbinStart; i <= xbinEnd; i++) {
			intersected.push_back( selectBin(i,ybinStart) );
		}
		if (ybinStart != ybinEnd) {
			for(size_t i = xbinStart; i <= xbinEnd; i++) {
				intersected.push_back(selectBin(i,ybinEnd));
			}
		}
		
		//now the vertical rows
		if (ybinStart+1 < ybinEnd) {
			//left vertical line
			for(size_t j = ybinStart+1; j < ybinEnd; ++j) {
				intersected.push_back(selectBin(xbinStart,j));
			}
			if (xbinStart != xbinEnd) { //right vertical line
				for(size_t j = ybinStart+1; j < ybinEnd; ++j) {
					intersected.push_back(selectBin(xbinEnd,j));
				}
			}
		}
	}
}

}}
