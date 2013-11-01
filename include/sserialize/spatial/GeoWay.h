#ifndef SSERIALIZE_SPATIAL_GEO_WAY_H
#define SSERIALIZE_SPATIAL_GEO_WAY_H
#include "GeoShape.h"
#include "GeoPoint.h"

namespace sserialize {
namespace spatial {

template<typename Point = GeoPoint, typename PointsContainer = std::vector<Point> >
class GeoWay:public GeoShape {
private:
	PointsContainer m_points;
	GeoRect m_boundary;
public:
	GeoWay() {}
	GeoWay(const PointsContainer & points) : m_points(points) {
		updateBoundaryRect();
	}
	virtual ~GeoWay() {}
	virtual uint32_t size() const { return m_points.size();}
	virtual bool intersects(const GeoRect & boundary) const {
		return collidesWithRect(boundary);
	}
	/** you need to update the boundary rect if you changed anything here! */
	PointsContainer & points() { return m_points; }
	const PointsContainer & points() const { return m_points; }
	
	void updateBoundaryRect() {
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
	
	virtual GeoRect boundary() const {
		return m_boundary;
	}

	virtual UByteArrayAdapter & serializeWithTypeInfo(UByteArrayAdapter & destination) const {
		destination << static_cast<uint8_t>( GS_WAY );
		return serialize(destination);
	}

	UByteArrayAdapter & serialize(UByteArrayAdapter & destination) const {
		destination.putVlPackedUint32(points().size());
		sserialize::spatial::GeoPoint bottomLeft(m_boundary.lat()[0], m_boundary.lon()[0]);
		sserialize::spatial::GeoPoint topRight(m_boundary.lat()[1], m_boundary.lon()[1]);
		destination << bottomLeft << topRight;
		for(size_t i = 0; i < points().size(); ++i) {
			destination << points().at(i);
		}
		return destination;
	}
	
	Point getBottomLeft() const {
		if (points().size() == 0) 
			return Point();
		return Point(m_boundary.lat()[0], m_boundary.lon()[0]);
	}
	Point getTopRight() const {
		if (points().size() == 0) 
			return Point();
		return Point(m_boundary.lat()[1], m_boundary.lon()[1]);
	}

	bool collidesWithRect(const GeoRect & rect) const {
		if (!m_boundary.overlap(rect))
			return false;
		for(size_t i = 0; i < points().size(); ++i) {
			Point p (points().at(i));
			if (rect.contains(p.lat, p.lon))
				return true;
		}
		return false;
	}
};

}}//end namespace

template<typename Point, typename PointsContainer>
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoWay<Point, PointsContainer> & p) {
	return p.serialize(destination);
}


#endif