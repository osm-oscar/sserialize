#ifndef SSERIALIZE_SPATIAL_LAT_LON_CALCULATIONS_H
#define SSERIALIZE_SPATIAL_LAT_LON_CALCULATIONS_H
#define SSERIALIZE_DEFAULT_EARTH_RADIUS 6371000

namespace sserialize {
namespace spatial {

//most of the calulations are based on http://www.movable-type.co.uk/scripts/latlong.html

///Simple distance calulation between (lat0, lon0)->(lat1, lon1), use DistanceCalculator for more advanced calculations
///@return distance in meters (can be negative) @param earthRadius in meters
double distanceTo(double lat0, double lon0, double lat1, double lon1, double earthRadius = SSERIALIZE_DEFAULT_EARTH_RADIUS);


void destinationPoint(double latStart, double lonStart, double bearing, double distance, double & latRes, double & lonRes, double earthRadius = SSERIALIZE_DEFAULT_EARTH_RADIUS);

///initial bearing in degrees (0, 360)
double bearingTo(double lat0, double lon0, double lat1, double lon1);

///midPoint
void midPoint(double lat0, double lon0, double lat1, double lon1, double & latRes, double & lonRes);

///midpoint on the line from p0->p1 along a rhumb point (straight line on mercator projection)
void rhumbMidPoint(double lat0, double lon0, double lat1, double lon1, double & latRes, double & lonRes);

///Cross-track distance: minimum distance between a point on the great-circle defined by p0->p1 and q (this is NOT the distance from p0p1 to q)
double crossTrackDistance(double lat0, double lon0, double lat1, double lon1, double latq, double lonq);

///along-track distance: distance from p0 to the point p_c that is closest to q following the great-circle arc defined by p0->p1
double alongTrackDistance(double lat0, double lon0, double lat1, double lon1, double latq, double lonq);

///shortest distance from q to the great-circle arc defined by p1->p0, always >= 0.0
double distance(double lat0, double lon0, double lat1, double lon1, double latq, double lonq);

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