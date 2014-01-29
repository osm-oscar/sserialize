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
	typedef typename TPointsContainer::iterator PointsIterator;
	typedef typename TPointsContainer::const_iterator ConstPointsIterator;	
	typedef GeoRegion MyBaseClass;
private:
	TPointsContainer m_points;
	GeoRect m_boundary;
protected:
	inline const GeoRect & myBoundary() const { return m_boundary; }
public:
	GeoWay();
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
	inline ConstPointsIterator cbegin() const { return points().cbegin(); }
	inline ConstPointsIterator cend() const { return points().cend(); }
	
	
	virtual GeoShapeType type() const { return sserialize::spatial::GS_WAY; }
	virtual uint32_t size() const;
	virtual GeoRect boundary() const;
	
	virtual bool contains(const GeoPoint & p) const;
	virtual bool intersects(const sserialize::spatial::GeoRect & rect) const;
	///@return true if the line p1->p2 intersects this region
	virtual bool intersects(const GeoPoint & p1, const GeoPoint & p2) const;
	virtual bool intersects(const GeoRegion & other) const;
	
	virtual UByteArrayAdapter & append(UByteArrayAdapter & destination) const {
		throw sserialize::UnimplementedFunctionException("sserialize::spatial::GeoWay<PointsContainer>::append");
	}
	
	virtual sserialize::spatial::GeoShape * copy() const { return new GeoWay(*this); }
};

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay() {}

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay(const TPointsContainer & points) : m_points(points) {
	recalculateBoundary();
}

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay(const GeoWay & other) :
m_points(other.m_points),
m_boundary(other.m_boundary)
{}

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay(TPointsContainer && points) :
m_points(points)
{
	recalculateBoundary();
}

template<typename TPointsContainer>
GeoWay<TPointsContainer>::GeoWay(GeoWay<TPointsContainer> && other) :
m_points(other.m_points),
m_boundary(other.m_boundary)
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
	for(size_t i = 0; i < points().size(); ++i) {
		Point p (points().at(i));
		if (rect.contains(p.lat(), p.lon()))
			return true;
	}
	return false;
}

template<typename TPointsContainer>
bool GeoWay<TPointsContainer>::contains(const GeoPoint & p) const {
	if (m_boundary.contains(p.lat(), p.lon())) {
		for(typename TPointsContainer::const_iterator it(m_points.cbegin()), end(m_points.cend()); it != end; ++it) {
			if (p == *it)
				return true;
		}
	}
	return false;
}

template<typename TPointsContainer>
bool GeoWay<TPointsContainer>::intersects(const GeoPoint & p1, const GeoPoint & p2) const {
	if (m_boundary.contains(p1.lat(), p1.lon()) || m_boundary.contains(p2.lat(), p2.lon())) {
		for(std::size_t i(0), j(1), end(m_points.size()); j < end; ++i, ++j) {
			if (sserialize::spatial::GeoPoint::intersect(m_points[i], m_points[j], p1, p2)) {
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

template<typename TPointsContainer>
void GeoWay<TPointsContainer>::recalculateBoundary() {
	if (!points().size())
		return;

	double minLat = points().at(0).lat();
	double minLon = points().at(0).lon();
	double maxLat = points().at(0).lat();
	double maxLon = points().at(0).lon();

	for(size_t i = 1; i < points().size(); i++) {
		minLat = std::min(points().at(i).lat(), minLat);
		minLon = std::min(points().at(i).lon(), minLon);
		maxLat = std::max(points().at(i).lat(), maxLat);
		maxLon = std::max(points().at(i).lon(), maxLon);
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