#include <sserialize/spatial/CellDistance.h>
#include <sserialize/spatial/LatLonCalculations.h>

namespace sserialize {
namespace spatial {
namespace interface {
	
CellDistance::~CellDistance() {}
	
double CellDistance::distance(const sserialize::spatial::GeoPoint & gp1, const sserialize::spatial::GeoPoint & gp2) {
	return std::abs<double>( sserialize::spatial::distanceTo(gp1.lat(), gp1.lon(), gp2.lat(), gp2.lon()) );
}
	
}}//end namespace spatial::interface

	
namespace Static {
namespace spatial {
	
CellDistanceByCellCenter::CellDistanceByCellCenter(const CellCenters & d) :
m_cc(d)
{}

CellDistanceByCellCenter::~CellDistanceByCellCenter()
{}

double CellDistanceByCellCenter::distance(uint32_t cellId1, uint32_t cellId2) const {
	return CellDistance::distance(m_cc.at(cellId1), m_cc.at(cellId2));
}

double CellDistanceByCellCenter::distance(const sserialize::spatial::GeoPoint & gp, uint32_t cellId) const {
	return CellDistance::distance(gp, m_cc.at(cellId));
}



}}//end namespace Static::spatial
	
}//end namespace sserialize
