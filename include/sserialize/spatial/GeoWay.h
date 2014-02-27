#ifndef SSERIALIZE_SPATIAL_GEO_WAY_H
#define SSERIALIZE_SPATIAL_GEO_WAY_H
#include <sserialize/spatial/GeoRegion.h>
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace spatial {

namespace detail {

template<typename TPointsContainer>
class GeoWay: public GeoRegion {
public:
	typedef sserialize::spatial::GeoPoint Point;
	typedef TPointsContainer PointsContainer;
	typedef typename TPointsContainer::const_iterator const_iterator;
	typedef typename TPointsContainer::iterator iterator;
	typedef GeoRegion MyBaseClass;
	typedef GeoWay<TPointsContainer> MyType;
private:
	GeoRect m_boundary;
	TPointsContainer m_points;
protected:
	inline const GeoRect & myBoundary() const { return m_boundary; }
public:
	GeoWay();
	GeoWay(const GeoRect & boundary, const TPointsContainer & points);
	GeoWay(const sserialize::UByteArrayAdapter & d);
	GeoWay(const TPointsContainer & points);
	GeoWay(const GeoWay & other);
	GeoWay(TPointsContainer && points);
	GeoWay(GeoWay && other);
	virtual ~GeoWay();
	GeoWay & operator=(GeoWay && other);
	GeoWay & operator=(const GeoWay & other);
	void swap(GeoWay & other);
	virtual void recalculateBoundary();
	/** you need to update the boundary rect if you changed anything here! */
	inline TPointsContainer & points() { return m_points; }
	inline const TPointsContainer & points() const { return m_points; }
	inline const_iterator cbegin() const { return points().cbegin(); }
	inline const_iterator cend() const { return points().cend(); }
	inline iterator begin() { return points().begin(); }
	inline iterator end() { return points().end(); }
	
	virtual GeoShapeType type() const { return sserialize::spatial::GS_WAY; }
	virtual uint32_t size() const;
	virtual GeoRect boundary() const;
	
	virtual bool contains(const GeoPoint & p) const;
	virtual bool intersects(const sserialize::spatial::GeoRect & rect) const;
	///@return true if the line p1->p2 intersects this region
	virtual bool intersects(const GeoPoint & p1, const GeoPoint & p2) const;
	virtual bool intersects(const GeoRegion & other) const;
	virtual double distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const;
	virtual UByteArrayAdapter & append(UByteArrayAdapter & /*destination*/) const {
		throw sserialize::UnimplementedFunctionException("sserialize::spatial::GeoWay<PointsContainer>::append");
	}
	
	virtual sserialize::spatial::GeoShape * copy() const { return new GeoWay(*this); }
};

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay() {}

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay(const GeoRect & boundary, const TPointsContainer & points) :
m_boundary(boundary),
m_points(points)
{}

	
template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay(const TPointsContainer & points) : m_points(points) {
	recalculateBoundary();
}

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay(const GeoWay & other) :
m_boundary(other.m_boundary),
m_points(other.m_points)
{}

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay(TPointsContainer && points) :
m_points(points)
{
	recalculateBoundary();
}

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay(GeoWay<TPointsContainer> && other) :
m_boundary(other.m_boundary),
m_points(other.m_points)
{}

template<typename TPointsContainer>
GeoWay<TPointsContainer>::~GeoWay() {}

template<typename TPointsContainer>
GeoWay<TPointsContainer> & GeoWay<TPointsContainer>::operator=(GeoWay<TPointsContainer> && other) {
	m_points.swap(other.m_points);
	m_boundary = other.m_boundary;
	return *this;
}

template<typename TPointsContainer>
GeoWay<TPointsContainer> & GeoWay<TPointsContainer>::operator=(const sserialize::spatial::detail::GeoWay<TPointsContainer> & other) {
	m_points = other.m_points;
	m_boundary = other.m_boundary;
	return *this;
}

template<typename TPointsContainer>
void GeoWay<TPointsContainer>::swap(GeoWay<TPointsContainer> & other) {
	std::swap(m_points, other.m_points);
	std::swap(m_boundary, other.m_boundary);
}

template<typename TPointsContainer>
uint32_t GeoWay<TPointsContainer>::size() const { return m_points.size();}

template<typename TPointsContainer>
bool GeoWay<TPointsContainer>::intersects(const GeoRect & rect) const {
	if (!m_boundary.overlap(rect))
		return false;
	for(const_iterator it(cbegin()), end(cend()); it != end; ++it) {
		Point p (*it);
		if (rect.contains(p.lat(), p.lon()))
			return true;
	}
	return false;
}

template<typename TPointsContainer>
bool GeoWay<TPointsContainer>::contains(const GeoPoint & p) const {
	if (m_boundary.contains(p.lat(), p.lon())) {
		for(const_iterator it(cbegin()), end(cend()); it != end; ++it) {
			if (p == *it)
				return true;
		}
	}
	return false;
}

template<typename TPointsContainer>
bool GeoWay<TPointsContainer>::intersects(const GeoPoint & p1, const GeoPoint & p2) const {
	if (m_boundary.contains(p1.lat(), p1.lon()) || m_boundary.contains(p2.lat(), p2.lon())) {
		const_iterator it(cbegin());
		++it;
		for(const_iterator prev(cbegin()), end(cend()); it != end; ++it, ++prev) {
			if (sserialize::spatial::GeoPoint::intersect(*prev, *it, p1, p2)) {
				return true;
			}
		}
	}
	return false;
}

template<typename TPointsContainer>
bool GeoWay<TPointsContainer>::intersects(const GeoRegion & other) const {

	const GeoWay * o = dynamic_cast<const sserialize::spatial::detail::GeoWay<TPointsContainer>* >(&other);

	if (!o) {
		if (other.type() > type()) {
			return other.intersects(*this);
		}
		return false;
	}
	
	if (o->m_boundary.overlap(m_boundary)) {
		const_iterator it(cbegin());
		++it;
		for(const_iterator prev(cbegin()), end(cend()); it != end; ++it, ++prev) {
			const_iterator oIt(o->cbegin());
			++oIt;
			for(const_iterator oPrev(cbegin()), end(cend()); it != end; ++it, ++prev) {
				if (sserialize::spatial::GeoPoint::intersect(*prev, *it, *oPrev, *oIt)) {
					return true;
				}
			}
		}
	}
	return false;
}

template<typename TPointsContainer>
double GeoWay<TPointsContainer>::distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const {
	GeoShapeType gt = type();
	if (gt <= GS_WAY) {
		if (gt == GS_POINT) {
			const sserialize::spatial::GeoPoint & op = *static_cast<const sserialize::spatial::GeoPoint*>(&other);
			double d = std::numeric_limits<double>::max();
			for(const_iterator it(cbegin()), end(cend()); it != end; ++it) {
				typename TPointsContainer::const_reference itP = *it;
				d = std::min<double>(d, distanceCalculator.calc(itP.lat(), itP.lon(), op.lat(), op.lon()));
			}
			return d;
		}
		else if (gt == GS_WAY) {
			const MyType & ow = *static_cast<const MyType*>(&other);
			double d = std::numeric_limits<double>::max();
			for(const_iterator it(cbegin()), end(cend()); it != end; ++it) {
				typename TPointsContainer::const_reference itP = *it;
				for (const_iterator oIt(ow.cbegin()), oEnd(ow.cend()); oIt != oEnd; ++oIt) {
					typename TPointsContainer::const_reference oItP = *oIt; 
					d = std::min<double>(d, distanceCalculator.calc(itP.lat(), itP.lon(), oItP.lat(), oItP.lon()));
				}
			}
			return d;
		}
	}
	else {
		return other.distance(*this, distanceCalculator);
	}
	return std::numeric_limits<double>::quiet_NaN();
}

template<typename TPointsContainer>
void GeoWay<TPointsContainer>::recalculateBoundary() {
	if (!points().size())
		return;

	const_iterator it(cbegin());
	GeoPoint p = *it;
	++it;
		
	double minLat = p.lat();
	double minLon = p.lon();
	double maxLat = p.lat();
	double maxLon = p.lon();

	for(const_iterator end(cend()); it != end; ++it) {
		p = *it;
		minLat = std::min(p.lat(), minLat);
		minLon = std::min(p.lon(), minLon);
		maxLat = std::max(p.lat(), maxLat);
		maxLon = std::max(p.lon(), maxLon);
	}
	
	m_boundary.lat()[0] = minLat;
	m_boundary.lat()[1] = maxLat;
	
	m_boundary.lon()[0] = minLon;
	m_boundary.lon()[1] = maxLon;
}

template<typename TPointsContainer>
GeoRect GeoWay<TPointsContainer>::boundary() const {
	return m_boundary;
}

}

namespace detail {
	template<>
	UByteArrayAdapter & GeoWay< std::vector<GeoPoint> >::append(UByteArrayAdapter & destination) const;
}


typedef detail::GeoWay< std::vector<GeoPoint> > GeoWay;

}}//end namespace

namespace std {
template<>
inline void swap<sserialize::spatial::GeoWay>(sserialize::spatial::GeoWay & a, sserialize::spatial::GeoWay & b) { a.swap(b);}
}

///serializes without type info
template<typename TPointsContainer>
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::detail::GeoWay<TPointsContainer> & p) {
	return p.append(destination);
}


#endif