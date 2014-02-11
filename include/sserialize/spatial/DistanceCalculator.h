#ifndef SSERIALIZE_DISTANCE_CALCULATOR_H
#define SSERIALIZE_DISTANCE_CALCULATOR_H
#include <memory>
#include <sserialize/vendor/geodesic.h>

namespace sserialize {
namespace spatial {

namespace detail {

class DistanceCalculator {
public:
	DistanceCalculator() {}
	virtual ~DistanceCalculator() {}
	virtual double calc(const double lat0, const double lon0, const double lat1, const double lon1) const = 0;
};

class EuclideanDistanceCalculator: public DistanceCalculator {
public:
	EuclideanDistanceCalculator() {}
	virtual ~EuclideanDistanceCalculator() {}
	virtual double calc(const double lat0, const double lon0, const double lat1, const double lon1) const;
};

class GeodesicDistanceCalculator: public DistanceCalculator {
private:
	struct geod_geodesic m_geodParams;
public:
	///Default ctor with WGS84 parameters as defined in inverse.c from GeoGraphicLib
	GeodesicDistanceCalculator(double a = 6378137, double f = 1.0/298.257223563);
	virtual ~GeodesicDistanceCalculator() {}
	virtual double calc(const double lat0, const double lon0, const double lat1, const double lon1) const;
};

}//end namespace detail

class DistanceCalculator {
private:
	std::shared_ptr<detail::DistanceCalculator> m_priv;
public:
	DistanceCalculator() : m_priv(new detail::EuclideanDistanceCalculator()) {}
	DistanceCalculator(const std::shared_ptr<detail::DistanceCalculator> & d) : m_priv(d) {}
	virtual ~DistanceCalculator() {}
	inline double calc(const double lat0, const double lon0, const double lat1, const double lon1) const {
		return m_priv->calc(lat0, lon0, lat1, lon1);
	}
};


}}//end namespace

#endif