#include <sserialize/spatial/LatLonCalculations.h>
#include <cmath>
#include <algorithm>

namespace sserialize {
namespace spatial {

inline double sqr(double a) { return a*a;}
inline double toRadian(double deg) { return (deg*M_PI)/180;}
inline double toDegree(double radian) { return (radian*180)/M_PI; }

double bearingTo(double lat0, double lon0, double lat1, double lon1) {
	double ph1(toRadian(lat0)), ph2(toRadian(lat1));
	double deltaLambda = toRadian(lon1-lon0);
	
	double y = ::sin(deltaLambda) * ::cos(ph2);
	double x = ::cos(ph1)* ::sin(ph2) - ::sin(ph1) * ::cos(ph2)* ::cos(deltaLambda);
	double theta = ::atan2(y, x);

	return ::fmod(toDegree(theta)+360, 360.0);
}

void destinationPoint(double latStart, double lonStart, double bearing, double distance, double& latRes, double& lonRes, double earthRadius) {
	double delta = distance / earthRadius; // angular distance in radians
	double theta = toRadian(bearing);

	double phi1 = toRadian(latStart);
	double lambda1 = toRadian(lonStart);

	double phi2 = std::asin( std::sin(phi1)*std::cos(delta) +
						std::cos(phi1)*std::sin(delta)*std::cos(theta) );
	double lambda2 = lambda1 + std::atan2(std::sin(theta)*std::sin(delta)*std::cos(phi1),
								std::cos(delta)-std::sin(phi1)*std::sin(phi2));

	latRes = toDegree( phi2 );
	lonRes = ::fmod(toDegree(lambda2)+540.0, 360)-180; // normalise to −180…+180°
}

void midPoint(double lat0, double lon0, double lat1, double lon1, double & latRes, double & lonRes) {
	double phi1 = toRadian(lat0);
	double lambda1 = toRadian(lon0);
	double phi2 = toRadian(lat1);
	double deltaLambda = toRadian(lon1-lon0);

	double Bx = std::cos(phi2) * std::cos(deltaLambda);
	double By = std::cos(phi2) * std::sin(deltaLambda);

	double phi3 = std::atan2(std::sin(phi1)+std::sin(phi2),
				std::sqrt( (std::cos(phi1)+Bx)*(std::cos(phi1)+Bx) + By*By) );
	double lambda3 = lambda1 + std::atan2(By, std::cos(phi1) + Bx);

	latRes = toDegree(phi3);
	lonRes = ::fmod(toDegree(lambda3)+540.0, 360.0)-180.0; // normalise to −180…+180°
}

void rhumbMidPoint(double lat0, double lon0, double lat1, double lon1, double& latRes, double& lonRes) {
	double phi1 = toRadian(lat0);
	double lambda1 = toRadian(lon0);
	double phi2 = toRadian(lat1);
	double lambda2 = toRadian(lon1);

	if (std::abs(lambda2-lambda1) > M_PI) {
		lambda1 += 2*M_PI; // crossing anti-meridian
	}

	double phi3 = (phi1+phi2)/2;
	double f1 = std::tan(M_PI/4 + phi1/2);
	double f2 = std::tan(M_PI/4 + phi2/2);
	double f3 = std::tan(M_PI/4 + phi3/2);
	double lambda3 = ( (lambda2-lambda1)*std::log(f3) + lambda1*std::log(f2) - lambda2*std::log(f1) ) / std::log(f2/f1);

	if (!std::isfinite(lambda3)) {
		lambda3 = (lambda1+lambda2)/2; // parallel of latitude
	}

	latRes = toDegree(phi3);
	lonRes = ::fmod(toDegree(lambda3)+540.0, 360)-180; // normalise to −180…+180°
}

double distanceTo(double lat0, double lon0, double lat1, double lon1, double earthRadius) {
	double ph1 = toRadian(lat0);
	double ph2 = toRadian(lat1);
	double deltaPh = toRadian(lat1-lat0);
	double deltaLambda = toRadian(lon1-lon0);

	double a = sqr(::sin(deltaPh/2)) + ::cos(ph1) * ::cos(ph2) * sqr(sin(deltaLambda/2));
	double c = 2 * ::atan2(::sqrt(a), ::sqrt(1-a));

	return earthRadius * c;
}

//pathStart=(lat0, lon0), pathEnd=(lat1, lon1), this=(latq, lonq)
double crossTrackDistance(double lat0, double lon0, double lat1, double lon1, double latq, double lonq) {
	double radius(SSERIALIZE_DEFAULT_EARTH_RADIUS);

	double delta13 = distanceTo(lat0, lon0, latq, lonq, radius)/radius;
	double theta13 = toRadian( bearingTo(lat0, lon0, latq, lonq) );
	double theta12 = toRadian( bearingTo(lat0, lon0, lat1, lon1) );

	double dxt = ::asin( ::sin(delta13) * ::sin(theta13-theta12) ) * radius;
	return dxt;
}

double alongTrackDistance(double lat0, double lon0, double lat1, double lon1, double latq, double lonq) {
	double radius(SSERIALIZE_DEFAULT_EARTH_RADIUS);

	double delta13 = distanceTo(lat0, lon0, latq, lonq, radius)/radius;
	double theta13 = toRadian( bearingTo(lat0, lon0, latq, lonq) );
	double theta12 = toRadian( bearingTo(lat0, lon0, lat1, lon1) );

	double dxt = ::asin( ::sin(delta13) * ::sin(theta13-theta12) );
	
	double dAt = ::acos(::cos(delta13)/::cos(dxt)) * radius;
	
	return dAt;
}

double distance(double lat0, double lon0, double lat1, double lon1, double latq, double lonq) {
	double earthRadius = SSERIALIZE_DEFAULT_EARTH_RADIUS;
	double dist01 = distanceTo(lat0, lon0, lat1, lon1, earthRadius);
	double atd = alongTrackDistance(lat0, lon0, lat1, lon1, lat1, lonq);

	double dist0q = distanceTo(lat0, lon0, latq, lonq, earthRadius);

	double delta13 = dist0q/earthRadius;
	double theta13 = toRadian( bearingTo(lat0, lon0, latq, lonq) );
	double theta12 = toRadian( bearingTo(lat0, lon0, lat1, lon1) );

	double dxt = ::asin( ::sin(delta13) * ::sin(theta13-theta12) );
	
	double dAt = ::acos(::cos(delta13)/::cos(dxt)) * earthRadius;

	if (atd < 0.0) {
		return ::fabs(dist0q);
	}
	else if (atd > dist01) {
		return ::fabs(distanceTo(lat1, lon1, latq, lonq));
	}
	else {
		return ::fabs( dxt*earthRadius );
	}
}

CrossTrackDistanceCalculator::CrossTrackDistanceCalculator(double lat0, double lon0, double lat1, double lon1) :
m_lat0(lat0),
m_lon0(lon0),
m_theta12(toRadian( bearingTo(lat0, lon0, lat1, lon1) ))
{}

double CrossTrackDistanceCalculator::operator()(double latq, double lonq) const {
	double radius(6371e3);

	double delta13 = distanceTo(m_lat0, m_lon0, latq, lonq, radius)/radius;
	double theta13 = toRadian( bearingTo(m_lat0, m_lon0, latq, lonq) );

	double dxt = ::asin( ::sin(delta13) * ::sin(theta13-m_theta12) ) * radius;
	return dxt;
}


}}