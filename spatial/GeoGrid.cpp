#include <sserialize/spatial/GeoGrid.h>
#include <sserialize/utility/pack_unpack_functions.h>

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

GeoGrid::GeoGrid(const UByteArrayAdapter& d) :
m_rect(d)
{
	sserialize::UByteArrayAdapter::OffsetType off = sserialize::SerializationInfo<sserialize::spatial::GeoRect>::sizeInBytes(m_rect);
	int tmp;
	m_latcount = d.getVlPackedUint32(off, &tmp);
	off += tmp;
	m_loncount = d.getVlPackedUint32(off);
	m_latStep = (m_rect.lat()[1]-m_rect.lat()[0])/m_latcount;
	m_lonStep = (m_rect.lon()[1]-m_rect.lon()[0])/m_loncount;
}



UByteArrayAdapter::OffsetType GeoGrid::getSizeInBytes() const {
	return sserialize::SerializationInfo<sserialize::spatial::GeoRect>::sizeInBytes(m_rect) + sserialize::psize_vu32(m_latcount) + sserialize::psize_vu32(m_loncount);
}

GeoRect GeoGrid::cellBoundary(uint32_t lat, uint32_t lon) const {
	return GeoRect(m_rect.lat()[0]+lat*m_latStep,
							m_rect.lat()[0]+(lat+1)*m_latStep,
							m_rect.lon()[0]+lon*m_lonStep,
							m_rect.lon()[0]+(lon+1)*m_lonStep);
}

GeoRect GeoGrid::multiCellBoundary(uint32_t latmin, uint32_t lonmin, uint32_t latmax, uint32_t lonmax) const {
	return GeoRect(m_rect.lat()[0] + latmin * m_latStep,
							m_rect.lat()[0]+(latmax+1)*m_latStep,
							m_rect.lon()[0]+ lonmin*m_lonStep,
							m_rect.lon()[0]+(lonmax+1)*m_lonStep);
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

GeoGrid::GridBin GeoGrid::select(uint32_t tile) const {
	return (tile < m_latcount*m_loncount ? GridBin(tile%m_latcount, tile/m_latcount, tile) : GridBin());
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
