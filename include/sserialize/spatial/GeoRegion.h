#ifndef SSERIALIZE_SPATIAL_GEO_REGION_H
#define SSERIALIZE_SPATIAL_GEO_REGION_H
#include <sserialize/spatial/GeoPoint.h>


namespace sserialize {
namespace spatial {

class GeoRegion: public GeoShape {
public:
	GeoRegion() {}
	virtual ~GeoRegion() {}
	virtual bool contains(const GeoPoint & p) const = 0;
	///@return true if the line p1->p2 intersects this region
	virtual bool intersects(const GeoPoint & p1, const GeoPoint & p2) const = 0;
	virtual bool intersects(const GeoRegion & other) const = 0;
};


}}//end namespace


#endif