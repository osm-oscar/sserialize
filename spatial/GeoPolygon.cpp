#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/utility/utilfuncs.h>
#define EPSILON 0.000001


namespace sserialize {
namespace spatial {

GeoPolygon::GeoPolygon() : GeoWay<GeoPoint, std::vector<GeoPoint> >() {}
GeoPolygon::GeoPolygon(const std::vector<Point> & points) : GeoWay<GeoPoint, std::vector<GeoPoint> >(points) {}
GeoPolygon::~GeoPolygon() {}
bool GeoPolygon::intersects(const GeoRect & boundary) const {
	return collidesWithRect(boundary);
}

UByteArrayAdapter & GeoPolygon::serializeWithTypeInfo(UByteArrayAdapter & destination) const {
	destination << static_cast<uint8_t>( GS_POLYGON );
	return serialize(destination);
}

UByteArrayAdapter & GeoPolygon::serialize(UByteArrayAdapter & destination) const {
	return GeoWay<GeoPoint, std::vector<GeoPoint> >::serialize(destination);
}

//http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
bool GeoPolygon::test(const Point & p) const {
	if (!points().size() || !boundary().contains(p.lat, p.lon))
		return false;
	int nvert = points().size();
	double testx = p.lat;
	double testy = p.lon;
	int i, j, c = 0;
	for (i = 0, j = nvert-1; i < nvert; j = i++) {
		const GeoPoint & iP = points()[i];
		const GeoPoint & jP = points()[j];
		double vertx_i = iP.lat;
		double verty_i = iP.lon;
		double vertx_j = jP.lat;
		double verty_j = jP.lon;
		
		if ( ((verty_i>testy) != (verty_j>testy)) &&
			(testx < (vertx_j-vertx_i) * (testy-verty_i) / (verty_j-verty_i) + vertx_i) ) {
			c = !c;
		}
	}
	return c;
}

bool GeoPolygon::test(const std::deque<Point> & ps) const {
	for(size_t i=0; i< ps.size(); i++)
		if (test(ps[i]))
			return true;

	return false;
}

bool GeoPolygon::test(const std::vector<Point> & ps) const {
	for(size_t i=0; i< ps.size(); i++)
		if (test(ps[i]))
			return true;

	return false;
}

bool GeoPolygon::collidesWithRect(const GeoRect & rect) const {
	if (!boundary().overlap(rect))
		return false;
		
	return collidesWithPolygon( fromRect(rect) );
}

bool GeoPolygon::collidesWithPolygon(const GeoPolygon & poly) const {
	if (!boundary().overlap(poly.boundary()))
		return false;

	if (test(poly.points())) { //check if at least one vertex poly lies within us
		return true;
	}
	else if (poly.test(points())) { //check if at least one own vertex lies within poly
		return true;
	}
	else { //check if any lines intersect
		for(size_t i=0; i < poly.points().size(); i++) {
			if (intersectsWithLineSegment(poly.points()[i], poly.points()[(i+1)%poly.points().size()]))
				return true;
		}
	}
	return false;
}

inline bool GeoPolygon::intersectsWithLineSegment(const Point & p1, const Point & p2) const {
	size_t pointSize = points().size();
	for(size_t i = 0; i < pointSize; i++) {
		if (intersectLineSegments(p1, p2, points()[i], points()[(i+1)%pointSize])) {
			return true;
		}
	}
	return false;
}

inline bool GeoPolygon::intersectLineSegments(const Point & p , const Point & q, const Point & r, const Point & s) const {
	double tl1, tl2;
	double t1_denom = (q.lon-p.lon)*(s.lat-r.lat)+(q.lat-p.lat)*(r.lon-s.lon);
	if (std::abs(t1_denom) <= 0.000001)
		return false;
	double t1_nom = (r.lon-p.lon)*(s.lat-r.lat)+(r.lat-p.lat)*(r.lon-s.lon);
	
	if (sserialize::sgn(t1_nom)*sserialize::sgn(t1_denom) < 0)
		return false;
	tl1 = t1_nom/t1_denom;
	if (tl1 > 1)
		return false;
	tl2 = (tl1*(q.lat-p.lat)-r.lat+p.lat)/(s.lat-r.lat);
	return (0.0 <= tl2 && 1.0 >= tl2);
}

GeoPolygon GeoPolygon::fromRect(const GeoRect & rect) {
	std::vector<GeoPoint> points;
	points.push_back( Point(rect.lat()[0], rect.lon()[0]) );
	points.push_back( Point(rect.lat()[1], rect.lon()[0]) );
	points.push_back( Point(rect.lat()[1], rect.lon()[1]) );
	points.push_back( Point(rect.lat()[0], rect.lon()[1]) );
	return GeoPolygon(points);
}


}}