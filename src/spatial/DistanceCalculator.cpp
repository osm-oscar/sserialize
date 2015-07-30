#include <sserialize/spatial/DistanceCalculator.h>
#include <sserialize/spatial/LatLonCalculations.h>
#include <cmath>

namespace sserialize {
namespace spatial {
namespace detail {

inline double sqr(double a) { return a*a;}
inline double toRadian(double deg) { return (deg*M_PI)/180;}


double EuclideanDistanceCalculator::calc(const double lat0, const double lon0, const double lat1, const double lon1) const {
	return ::sqrt( sqr(lat0-lat1) + sqr(lon0-lon1) );
}

GeodesicDistanceCalculator::GeodesicDistanceCalculator(double a, double f) {
	geod_init(&m_geodParams, a, f);
}

double GeodesicDistanceCalculator::calc(const double lat0, const double lon0, const double lat1, const double lon1) const {
	double s12;
	geod_inverse(&m_geodParams, lat0, lon0, lat1, lon1, &s12, 0, 0);
	return s12;
}

//port from http://www.movable-type.co.uk/scripts/latlong.html
double HaversineDistanceCaluclator::calc(const double lat0, const double lon0, const double lat1, const double lon1) const {
	return distanceTo(lat0, lon0, lat1, lon1, m_earthRadius);
}


}

DistanceCalculator::DistanceCalculator(sserialize::spatial::DistanceCalculator::DistanceCalculatorTypes type) {
	switch (type) {
	case DCT_EUCLIDEAN:
		m_priv.reset(new detail::EuclideanDistanceCalculator());
		break;
	case DCT_GEODESIC_ACCURATE:
		m_priv.reset(new detail::GeodesicDistanceCalculator());
		break;
	case DCT_GEODESIC_FAST:
		m_priv.reset(new detail::HaversineDistanceCaluclator());
		break;
	default:
		break;
	}
}

}}

