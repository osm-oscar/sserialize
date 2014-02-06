#include <sserialize/spatial/GeoMultiPolygon.h>
#include <numeric>
#include <sserialize/Static/Deque.h>
#include <sserialize/spatial/GeoRect.h>


namespace sserialize {
namespace spatial {
// 
// bool GeoMultiPolygon::collidesWithMultiPolygon(const GeoMultiPolygon & multiPoly) const {
// 	if (!m_outerBoundary.overlap(multiPoly.m_outerBoundary))
// 		return false;
// 	throw sserialize::UnimplementedFunctionException("sserialize::spatial::GeoMultiPolygon::collidesWithMultiPolygon");
// 	return false;
// }
// 
// bool GeoMultiPolygon::collidesWithPolygon(const GeoPolygon & poly) const {
// 	if (!m_outerBoundary.overlap(poly.boundary()))
// 		return false;
// 	
// 	bool collides = false;
// 	for(PolygonList::const_iterator it(outerPolygons().begin()), end(outerPolygons().end()); it != end; ++it) {
// 		if (it->intersects(poly)) {
// 			collides = true;
// 			break;
// 		}
// 	}
// 	//now check if the test polygon is fully contained in any of our outer polygons:
// 	if (collides) {
// 		for(PolygonList::const_iterator it(innerPolygons().begin()), end(innerPolygons().end()); it != end; ++it) {
// 			if (it->encloses(poly)) {
// 				collides = false;
// 				break;
// 			}
// 		}
// 	}
// 	return collides;
// }
// 
// bool GeoMultiPolygon::collidesWithWay(const GeoWay & way) const {
// 	if (!way.boundary().overlap(m_outerBoundary) || !way.points().size())
// 		return false;
// 
// 	if (way.points().size() == 1) {
// 		return contains(way.points().front());
// 	}
// 	//check if any point of the way is contained
// 	for(GeoWay::PointsContainer::const_iterator it(way.points().begin()), end(way.points().end()); it != end; ++it) {
// 		if (contains(*it))
// 			return true;
// 	}
// 	//check for an intersection with any way segment
// 	for(GeoWay::PointsContainer::const_iterator prev(way.points().begin()), it(way.points().begin()+1), end(way.points().end()); it != end; ++prev, ++it) {
// 		if (intersects(*prev, *it))
// 			return true;
// 	}
// 	return false;
// }
// 
// GeoMultiPolygon::GeoMultiPolygon() {}
// 
// 
// GeoMultiPolygon::GeoMultiPolygon(GeoMultiPolygon && other) :
// m_innerPolygons(other.m_innerPolygons),
// m_outerPolygons(other.m_outerPolygons),
// m_innerBoundary(other.m_innerBoundary),
// m_outerBoundary(other.m_outerBoundary),
// m_size(other.m_size)
// {}
// 
// GeoMultiPolygon::GeoMultiPolygon(const GeoMultiPolygon & other) :
// m_innerPolygons(other.m_innerPolygons),
// m_outerPolygons(other.m_outerPolygons),
// m_innerBoundary(other.m_innerBoundary),
// m_outerBoundary(other.m_outerBoundary),
// m_size(other.m_size)
// {}
// 
// GeoMultiPolygon::~GeoMultiPolygon() {}
// 
// 
// GeoMultiPolygon & GeoMultiPolygon::operator=(GeoMultiPolygon && other) {
// 	std::swap(m_innerPolygons, other.m_innerPolygons);
// 	std::swap(m_outerPolygons, other.m_outerPolygons);
// 	m_innerBoundary = other.m_innerBoundary;
// 	m_outerBoundary = other.m_outerBoundary;
// 	m_size = other.m_size;
// 	return *this;
// }
// 
// GeoMultiPolygon & GeoMultiPolygon::operator=(const sserialize::spatial::GeoMultiPolygon& other) {
// 	m_innerPolygons = other.m_innerPolygons;
// 	m_outerPolygons = other.m_outerPolygons;
// 	m_innerBoundary = other.m_innerBoundary;
// 	m_outerBoundary = other.m_outerBoundary;
// 	m_size = other.m_size;
// 	return *this;
// }
// 
// GeoShapeType GeoMultiPolygon::type() const {
// 	return GS_MULTI_POLYGON;
// }
// 
// uint32_t GeoMultiPolygon::size() const {
// 	return m_size;
// }
// 
// GeoRect GeoMultiPolygon::boundary() const {
// 	return outerPolygonsBoundary();
// }
// 
// bool GeoMultiPolygon::intersects(const GeoRect & boundary) const {
// 	if (this->outerPolygonsBoundary().overlap(boundary)) {
// 		bool intersects = false;
// 		for(PolygonList::const_iterator it(outerPolygons().begin()), end(outerPolygons().end()); it != end; ++it) {
// 			if (it->intersects(boundary)) {
// 				intersects = true;
// 				break;
// 			}
// 		}
// 		if (intersects && this->innerPolygonsBoundary().overlap(boundary)) {
// 			GeoPolygon boundaryPoly(GeoPolygon::fromRect(boundary));
// 			for(PolygonList::const_iterator it(innerPolygons().begin()), end(innerPolygons().end()); it != end; ++it) {
// 				if (it->encloses(boundaryPoly)) {
// 					intersects = false;
// 					break;
// 				}
// 			}
// 		}
// 		return intersects;
// 	}
// 	return false;
// }
// 
// bool GeoMultiPolygon::contains(const GeoPoint & p) const {
// 	if (!outerPolygonsBoundary().contains(p.lat(), p.lon()))
// 		return false;
// 	bool contained = false;
// 	for(PolygonList::const_iterator it(outerPolygons().begin()), end(outerPolygons().end()); it != end; ++it) {
// 		if (it->contains(p)) {
// 			contained = true;
// 			break;
// 		}
// 	}
// 	if (contained && innerPolygonsBoundary().contains(p.lat(), p.lon())) {
// 		for(PolygonList::const_iterator it(innerPolygons().begin()), end(innerPolygons().end()); it != end; ++it) {
// 			if (it->contains(p)) {
// 				contained = false;
// 				break;
// 			}
// 		}
// 	}
// 	return contained;
// }
// 
// bool GeoMultiPolygon::intersects(const GeoPoint & p1, const GeoPoint & p2) const {
// 	if (!outerPolygonsBoundary().contains(p1.lat(), p1.lon()) && !outerPolygonsBoundary().contains(p2.lat(), p2.lon()))
// 		return false;
// 	if (contains(p1) || contains(p2))
// 		return true;
// 	//check for intersection with any polygon
// 	for(PolygonList::const_iterator it(outerPolygons().begin()), end(outerPolygons().end()); it != end; ++it) {
// 		if (it->intersects(p1, p2)) {
// 			return true;
// 		}
// 	}
// 	//Check for intersection with inner polygons. i.e. if both points lie within a star-shaped inner polygon
// 	for(PolygonList::const_iterator it(innerPolygons().begin()), end(innerPolygons().end()); it != end; ++it) {
// 		if (it->intersects(p1, p2)) {
// 			return true;
// 		}
// 	}
// 	return false;
// }
// 
// bool GeoMultiPolygon::intersects(const GeoRegion & other) const {
// 	if (other.type() <= type()) {
// 		switch (other.type()) {
// 		case GS_WAY:
// 			return collidesWithWay(*static_cast<const GeoWay*>(&other));
// 		case GS_POLYGON:
// 			return collidesWithPolygon(*static_cast<const GeoPolygon*>(&other));
// 		case GS_MULTI_POLYGON:
// 			return collidesWithMultiPolygon(*static_cast<const GeoMultiPolygon*>(&other));
// 		default:
// 			sserialize::UnimplementedFunctionException("Unable to do intersection with unknown type");
// 			return false;
// 		}
// 	}
// 	else {
// 		return other.intersects(*this);
// 	}
// }
// 
// template<typename TIterator>
// GeoRect calcBoundary(TIterator it,  TIterator end) {
// 	if (it != end) {
// 		it->recalculateBoundary();
// 		GeoRect b = it->boundary();
// 		for(++it; it != end; ++it) {
// 			it->recalculateBoundary();
// 			b.enlarge(it->boundary());
// 		}
// 		return b;
// 	}
// 	return GeoRect();
// }
// 
// void GeoMultiPolygon::recalculateBoundary() {
// 	m_innerBoundary = calcBoundary(m_innerPolygons.begin(), m_innerPolygons.end());
// 	m_outerBoundary = calcBoundary(m_outerPolygons.begin(), m_outerPolygons.end());
// 	auto c = [](uint32_t v, const GeoPolygon & a) {return v+a.size();};
// 	m_size = std::accumulate(m_innerPolygons.begin(), m_innerPolygons.end(), static_cast<uint32_t>(0), c);
// 	m_size += std::accumulate(m_outerPolygons.begin(), m_outerPolygons.end(), m_size, c);
// }
// 	
// const GeoRect & GeoMultiPolygon::innerPolygonsBoundary() const { return m_innerBoundary; }
// const GeoRect & GeoMultiPolygon::outerPolygonsBoundary() const { return m_outerBoundary; }
// 
// const GeoMultiPolygon::PolygonList & GeoMultiPolygon::innerPolygons() const { return m_innerPolygons; }
// GeoMultiPolygon::PolygonList & GeoMultiPolygon::innerPolygons() { return m_innerPolygons; }
// 
// const GeoMultiPolygon::PolygonList & GeoMultiPolygon::outerPolygons() const { return m_outerPolygons; }
// GeoMultiPolygon::PolygonList & GeoMultiPolygon::outerPolygons() { return m_outerPolygons; }
// 

namespace detail {

template<>
sserialize::UByteArrayAdapter & GeoMultiPolygon< std::vector<sserialize::spatial::GeoPolygon>, sserialize::spatial::GeoPolygon >::append(sserialize::UByteArrayAdapter & destination) const {
	destination.putVlPackedUint32(m_size);
	destination << m_outerBoundary;
	destination << m_innerBoundary;
	destination << m_outerPolygons;
	destination << m_innerPolygons;
	return destination;
}

}//end namespace detail
// 
}}