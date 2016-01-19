#ifndef SSERIALIZE_SPATIAL_LAT_LON_CALCULATIONS_H
#define SSERIALIZE_SPATIAL_LAT_LON_CALCULATIONS_H

namespace sserialize {
namespace spatial {

//most of the calulations are based on http://www.movable-type.co.uk/scripts/latlong.html

///Simple distance calulation between (lat0, lon0)->(lat1, lon1), use DistanceCalculator for more advanced calculations
///@return distance in meters @param earthRadius in meters
double distanceTo(double lat0, double lon0, double lat1, double lon1, double earthRadius = 6371000);

///initial bearing in degrees
double bearingTo(double lat0, double lon0, double lat1, double lon1);

///Cross-track distance: minimum distance between a point on the great-circle arc of p0->p1 and q
double crossTrackDistance(double lat0, double lon0, double lat1, double lon1, double latq, double lonq);

class CrossTrackDistanceCalculator final {
	double m_lat0;
	double m_lon0;
	double m_theta12;
public:
	CrossTrackDistanceCalculator(double lat0, double lon0, double lat1, double lon1);
	~CrossTrackDistanceCalculator() {}
	double operator()(double latq, double lonq) const;
};

}}

#endif