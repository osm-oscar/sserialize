#include <sserialize/spatial/GeoWay.h>

namespace sserialize {
namespace spatial {


GeoWay::GeoWay() {}
GeoWay::GeoWay(const PointsContainer & points) : m_points(points) {
	updateBoundaryRect();
}
GeoWay::~GeoWay() {}
GeoShapeType GeoWay::type() const { return GS_WAY; }

uint32_t GeoWay::size() const { return m_points.size();}

bool GeoWay::intersects(const GeoRect & rect) const {
	if (!m_boundary.overlap(rect))
		return false;
	for(size_t i = 0; i < points().size(); ++i) {
		Point p (points().at(i));
		if (rect.contains(p.lat, p.lon))
			return true;
	}
	return false;
}

bool GeoWay::contains(const GeoPoint & p) const {
	if (m_boundary.contains(p.lat, p.lon)) {
		for(typename PointsContainer::const_iterator it(m_points.begin()), end(m_points.end()); it != end; ++it) {
			if (p == *it)
				return true;
		}
	}
	return false;
}

///@return true if the line p1->p2 intersects this region
bool GeoWay::intersects(const GeoPoint & p1, const GeoPoint & p2) const {
	if (m_boundary.contains(p1.lat, p1.lon) || m_boundary.contains(p2.lat, p2.lon)) {
		for(std::size_t i(0), j(1), end(m_points.size()); j < end; ++i, ++j) {
			if (sserialize::spatial::GeoPoint::intersect(m_points[i], m_points[j], p1, p2)) {
				return true;
			}
		}
	}
	return false;
}

bool GeoWay::intersects(const GeoRegion & other) const {
	if (other.type() != sserialize::spatial::GS_WAY) {
		return other.intersects(*this);
	}
	const GeoWay * o = static_cast<const sserialize::spatial::GeoWay* >(&other);
	if (o->m_boundary.overlap(m_boundary)) {
		for(std::size_t i(0), j(1), myEnd(m_points.size()); j < myEnd; ++i, ++j) {
			for(std::size_t k(0), l(1), oEnd(o->m_points.size()); l < oEnd; ++k, ++l) {
				if (sserialize::spatial::GeoPoint::intersect(m_points[i], m_points[j], o->m_points[k], o->m_points[l])) {
					return true;
				}
			}
		}
	}
	return false;
}


void GeoWay::updateBoundaryRect() {
	if (!points().size())
		return;

	double minLat = points().at(0).lat;
	double minLon = points().at(0).lon;
	double maxLat = points().at(0).lat;
	double maxLon = points().at(0).lon;

	for(size_t i = 1; i < points().size(); i++) {
		minLat = std::min(points().at(i).lat, minLat);
		minLon = std::min(points().at(i).lon, minLon);
		maxLat = std::max(points().at(i).lat, maxLat);
		maxLon = std::max(points().at(i).lon, maxLon);
	}
	
	m_boundary.lat()[0] = minLat;
	m_boundary.lat()[1] = maxLat;
	
	m_boundary.lon()[0] = minLon;
	m_boundary.lon()[1] = maxLon;
}

GeoRect GeoWay::boundary() const {
	return m_boundary;
}

UByteArrayAdapter & GeoWay::serializeWithTypeInfo(UByteArrayAdapter & destination) const {
	destination << static_cast<uint8_t>( GS_WAY );
	return serialize(destination);
}

UByteArrayAdapter & GeoWay::serialize(UByteArrayAdapter & destination) const {
	destination.putVlPackedUint32(points().size());
	sserialize::spatial::GeoPoint bottomLeft(m_boundary.lat()[0], m_boundary.lon()[0]);
	sserialize::spatial::GeoPoint topRight(m_boundary.lat()[1], m_boundary.lon()[1]);
	destination << bottomLeft << topRight;
	for(size_t i = 0; i < points().size(); ++i) {
		destination << points().at(i);
	}
	return destination;
}

sserialize::spatial::GeoShape * GeoWay::copy() const {
	return new sserialize::spatial::GeoWay(*this);
}

}}//end namespace

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoWay & p) {
	return p.serialize(destination);
}
