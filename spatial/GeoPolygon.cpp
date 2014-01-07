#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/utility/utilfuncs.h>
#define EPSILON 0.000001


namespace sserialize {
namespace spatial {

bool GeoPolygon::collidesWithPolygon(const GeoPolygon & poly) const {
	if (!boundary().overlap(poly.boundary()))
		return false;

	if (contains(poly.points().begin(), poly.points().end())) { //check if at least one vertex poly lies within us
		return true;
	}
	else if (poly.contains(points().begin(), points().end())) { //check if at least one own vertex lies within poly
		return true;
	}
	else { //check if any lines intersect
		for(std::size_t i=0, s = poly.points().size(); i < s; i++) {
			if (intersects(poly.points()[i], poly.points()[(i+1)%s]))
				return true;
		}
	}
	return false;
}


bool GeoPolygon::collidesWithWay(const GeoWay & way) const {
	if (!boundary().overlap(way.boundary()))
		return false;

	if (contains(way.points().begin(), way.points().end())) { //check if at least one vertex poly lies within us
		return true;
	}
	else if (way.size() > 1) { //check if any lines intersect
		for(GeoWay::ConstPointsIterator it(way.cbegin()), jt(way.cbegin()+1), end(way.cend()); jt != end; ++it, ++jt) {
			for(std::size_t i = 0, s = points().size(); i < s; ++i) {
				if (sserialize::spatial::GeoPoint::intersect(*it, *jt, points()[i], points()[(i+1)%s])) {
					return true;
				}
			}
		}
	}
	return false;
}

GeoPolygon::GeoPolygon() :
GeoWay()
{}

GeoPolygon::GeoPolygon(const std::vector<Point> & points) :
GeoWay(points)
{}

GeoPolygon::GeoPolygon(std::vector<Point> && points) :
GeoWay(points)
{}

GeoPolygon::GeoPolygon(GeoPolygon && other) :
GeoWay(other)
{}

GeoPolygon::GeoPolygon(const GeoPolygon & other) :
GeoWay(other)
{}

GeoPolygon::~GeoPolygon()
{}

GeoPolygon & GeoPolygon::operator=(GeoPolygon && other) {
	swap(other);
	return *this;
}

GeoPolygon & GeoPolygon::operator=(const sserialize::spatial::GeoPolygon & other) {
	GeoWay::operator=(other);
	return *this;
}

void GeoPolygon::swap(GeoPolygon & other) {
	GeoWay::swap(other);
}

GeoShapeType GeoPolygon::type() const {
	return GS_POLYGON;
}

//http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
bool GeoPolygon::contains(const GeoPoint & p) const {
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

bool GeoPolygon::intersects(const sserialize::spatial::GeoRect & rect) const {
	if (!boundary().overlap(rect))
		return false;
		
	return collidesWithPolygon( fromRect(rect) );
}

bool GeoPolygon::intersects(const GeoPoint & p1, const GeoPoint & p2) const {
	if (!myBoundary().overlap( sserialize::spatial::GeoRect(p1.lat, p2.lat, p1.lon, p2.lon) ) ) {
		return false;
	}

	if (contains(p1) || contains(p2)) {
		return true;
	}

	for(std::size_t i = 0, s = points().size(); i < s; i++) {
		if (sserialize::spatial::GeoPoint::intersect(p1, p2, points()[i], points()[(i+1)%s])) {
			return true;
		}
	}
	return false;
}

bool GeoPolygon::intersects(const GeoRegion & other) const {
	if (other.type() == sserialize::spatial::GS_POLYGON) {
		return collidesWithPolygon( *static_cast<const sserialize::spatial::GeoPolygon*>(&other) );
	}
	else if (other.type() == sserialize::spatial::GS_WAY) {
		return collidesWithWay( *static_cast<const sserialize::spatial::GeoWay*>(&other) );
	}
	else {
		return other.intersects(*this);
	}
}

UByteArrayAdapter & GeoPolygon::append(UByteArrayAdapter & destination) const {
	return destination << *this;
}

sserialize::spatial::GeoShape * GeoPolygon::copy() const {
	return new sserialize::spatial::GeoPolygon(*this);
}

GeoPolygon GeoPolygon::fromRect(const GeoRect & rect) {
	std::vector<GeoPoint> points;
	points.push_back( Point(rect.lat()[0], rect.lon()[0]) );
	points.push_back( Point(rect.lat()[1], rect.lon()[0]) );
	points.push_back( Point(rect.lat()[1], rect.lon()[1]) );
	points.push_back( Point(rect.lat()[0], rect.lon()[1]) );
	return GeoPolygon(points);
}


bool GeoPolygon::encloses(const sserialize::spatial::GeoPolygon & other) const {
	return encloses(*static_cast<const sserialize::spatial::GeoWay*>(&other) );
}

bool GeoPolygon::encloses(const sserialize::spatial::GeoWay & other) const {
	if (!other.boundary().overlap(this->boundary())) {
		return false;
	}
	//if all points are within my self and nothing intersects, then the poly is fully contained
	for (std::size_t i = 0, s = other.points().size(); i < s; ++i) {
		if (!contains(other.points()[i])) {
			return false;
		}
	}
	//all points are within, check for cut. TODO: improve this by haveing more info about the polygon, if this would be convex, then we would be done here
	
	for(GeoWay::PointsContainer::const_iterator prev(other.points().begin()), it(other.points().begin()+1), end(other.points().end()); it != end; ++it) {
		if (intersects(*prev, *it))
			return false;
	}
	return true;
}

}}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPolygon & p) {
	const sserialize::spatial::GeoWay * gw = &p;
	return destination << *gw;
}