#include <sserialize/spatial/DistanceCalculator.h>
#include <cmath>

namespace sserialize {
namespace spatial {
namespace detail {

inline double sqr(double a) { return a*a;}

double EuclideanDistanceCalculator::calc(const double lat0, const double lon0, const double lat1, const double lon1) const {
	return std::sqrt( sqr(lat0-lat1) + sqr(lon0-lon1) );
}

GeodesicDistanceCalculator::GeodesicDistanceCalculator(double a, double f) {
	geod_init(&m_geodParams, a, f);
}

double GeodesicDistanceCalculator::calc(const double lat0, const double lon0, const double lat1, const double lon1) const {
	double s12;
	geod_inverse(&m_geodParams, lat0, lon0, lat1, lon1, &s12, 0, 0);
	return s12;
}

}}}