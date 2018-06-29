#ifndef SSERIALIZE_SPATIAL_CELL_DISTANCE_H
#define SSERIALIZE_SPATIAL_CELL_DISTANCE_H
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/Static/Array.h>

namespace sserialize {
namespace spatial {
namespace interface {
	
class CellDistance {
public:
	CellDistance() {}
	virtual ~CellDistance();
	virtual double distance(uint32_t cellId1, uint32_t cellId2) const = 0;
	virtual double distance(const sserialize::spatial::GeoPoint & gp, uint32_t cellId) const = 0;
protected:
	static double distance(const sserialize::spatial::GeoPoint & gp1, const sserialize::spatial::GeoPoint & gp2);
};
	
}} //end namespace spatial::interface

namespace Static {
namespace spatial {
	
class CellDistanceByCellCenter: public sserialize::spatial::interface::CellDistance {
public:
	typedef sserialize::Static::Array<sserialize::Static::spatial::GeoPoint> CellCenters;
public:
	CellDistanceByCellCenter(const CellCenters & d);
	virtual ~CellDistanceByCellCenter();
	virtual double distance(uint32_t cellId1, uint32_t cellId2) const;
	virtual double distance(const sserialize::spatial::GeoPoint & gp, uint32_t cellId) const;
private:
	CellCenters m_cc;
};

}}//end namespace Static::spatial


}//end namespace sserialize::spatial::interface

#endif
