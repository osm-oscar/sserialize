#ifndef SSERIALIZE_GEO_AREA_H
#define SSERIALIZE_GEO_AREA_H
#include <sserialize/spatial/GeoPolygon.h>

namespace sserialize {
namespace spatial {
namespace detail {

///Call recalculateBoundaries() after construction

template<typename TPolygonContainer, typename TPolygon = typename TPolygonContainer::value_type>
class GeoMultiPolygon: public GeoRegion {
public:
	typedef TPolygonContainer PolygonList;
	typedef sserialize::spatial::GeoPoint Point;
	typedef TPolygon GeoPolygon;
	typedef typename TPolygon::MyBaseClass GeoWay;
private:
	//total number of GeoPoints
	uint32_t m_size;
	//outer polygons define the region of space
	PolygonList m_outerPolygons;
	//Inner polygons are within an enclosing polygon. The space inside ist defined as beeing outside of the enclosing polygon
	PolygonList m_innerPolygons;
	//The outer boundary is the boundary enclosing the whole MultiPolygon
	GeoRect m_outerBoundary;
	//The inner boundary is the merged boundary of all inner polygons 
	GeoRect m_innerBoundary;

private:
	bool collidesWithMultiPolygon(const GeoMultiPolygon & multiPoly) const;
	bool collidesWithPolygon(const GeoPolygon & poly) const;
	bool collidesWithWay(const GeoWay & way) const;
	
	template<typename TIterator>
	GeoRect calcBoundary(TIterator it,  TIterator end);
	
public:
	GeoMultiPolygon();
	GeoMultiPolygon(const sserialize::UByteArrayAdapter & d);
	GeoMultiPolygon(const PolygonList & outer, const PolygonList & inner);
	GeoMultiPolygon(uint32_t size, const PolygonList & outer, const PolygonList & inner, const GeoRect & outerBoundary, const GeoRect & innerBoundary);
	GeoMultiPolygon(GeoMultiPolygon && other);
	GeoMultiPolygon(const GeoMultiPolygon & other);
	virtual ~GeoMultiPolygon();
	GeoMultiPolygon & operator=(GeoMultiPolygon && other);
	GeoMultiPolygon & operator=(const GeoMultiPolygon & other);
	virtual GeoShapeType type() const override;
	virtual uint32_t size() const override;
	virtual GeoRect boundary() const override;
	virtual bool intersects(const GeoRect & boundary) const override;
	virtual bool contains(const GeoPoint & p) const override;
	///@return true if the line p1->p2 intersects this region
	virtual bool intersects(const GeoPoint & p1, const GeoPoint & p2) const override;
	virtual bool intersects(const GeoRegion & other) const override;
	virtual double distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const override;
	
	virtual void recalculateBoundary() override;
	
	bool encloses(const GeoPolygon & polygon) const;
	
	sserialize::spatial::GeoPoint at(uint32_t pos) const;
	
	///boundary of the inner polygons that define holes
	const GeoRect & innerPolygonsBoundary() const;
	///boundary of polygons that define the region
	const GeoRect & outerPolygonsBoundary() const;
	
	///polygons that lie within a outer polygon and their space defined whats outside
	const PolygonList & innerPolygons() const;
	///polygons that lie within a outer polygon and their space defined whats outside
	PolygonList & innerPolygons();
	
	///polygons that define what's inside
	const PolygonList & outerPolygons() const;
	///polygons that define what's inside
	PolygonList & outerPolygons();

	virtual UByteArrayAdapter & append(sserialize::UByteArrayAdapter & destination) const override;
	
	virtual sserialize::spatial::GeoShape * copy() const override;
	virtual std::ostream & asString(std::ostream & out) const override;
};

template<typename TPolygonContainer, typename TPolygon>
bool GeoMultiPolygon<TPolygonContainer, TPolygon>::collidesWithMultiPolygon(const GeoMultiPolygon & multiPoly) const {
	if (!m_outerBoundary.overlap(multiPoly.m_outerBoundary))
		return false;
	throw sserialize::UnimplementedFunctionException("sserialize::spatial::GeoMultiPolygon::collidesWithMultiPolygon");
	return false;
}

template<typename TPolygonContainer, typename TPolygon>
bool GeoMultiPolygon<TPolygonContainer, TPolygon>::collidesWithPolygon(const GeoPolygon & poly) const {
	if (!m_outerBoundary.overlap(poly.boundary()))
		return false;
	
	bool collides = false;
	for(typename PolygonList::const_iterator it(outerPolygons().cbegin()), end(outerPolygons().cend()); it != end; ++it) {
		if ((*it).intersects(poly)) {
			collides = true;
			break;
		}
	}
	//now check if the test polygon is fully contained in any of our outer polygons:
	if (collides) {
		for(typename PolygonList::const_iterator it(innerPolygons().cbegin()), end(innerPolygons().cend()); it != end; ++it) {
			if ((*it).encloses(poly)) {
				collides = false;
				break;
			}
		}
	}
	return collides;
}

template<typename TPolygonContainer, typename TPolygon>
bool GeoMultiPolygon<TPolygonContainer, TPolygon>::collidesWithWay(const GeoWay & way) const {
	if (!way.boundary().overlap(m_outerBoundary) || !way.points().size())
		return false;

	if (way.points().size() == 1) {
		return contains(way.points().front());
	}
	//check if any point of the way is contained
	for(typename GeoWay::PointsContainer::const_iterator it(way.points().cbegin()), end(way.points().cend()); it != end; ++it) {
		if (contains(*it))
			return true;
	}
	//check for an intersection with any way segment
	for(typename GeoWay::PointsContainer::const_iterator prev(way.points().cbegin()), it(way.points().cbegin()+1), end(way.points().cend()); it != end; ++prev, ++it) {
		if (intersects(*prev, *it))
			return true;
	}
	return false;
}

template<typename TPolygonContainer, typename TPolygon>
GeoMultiPolygon<TPolygonContainer, TPolygon>::GeoMultiPolygon() {}

template<typename TPolygonContainer, typename TPolygon>
GeoMultiPolygon<TPolygonContainer, TPolygon>::GeoMultiPolygon(const PolygonList & outer, const PolygonList & inner) :
m_outerPolygons(outer),
m_innerPolygons(inner)
{
	recalculateBoundary();
}

template<typename TPolygonContainer, typename TPolygon>
GeoMultiPolygon<TPolygonContainer, TPolygon>::GeoMultiPolygon(uint32_t size, const PolygonList & outer, const PolygonList & inner, const GeoRect & outerBoundary, const GeoRect & innerBoundary) :
m_size(size),
m_outerPolygons(outer),
m_innerPolygons(inner),
m_outerBoundary(outerBoundary),
m_innerBoundary(innerBoundary)
{}

template<typename TPolygonContainer, typename TPolygon>
GeoMultiPolygon<TPolygonContainer, TPolygon>::GeoMultiPolygon(GeoMultiPolygon && other) :
m_size(other.m_size),
m_outerPolygons(other.m_outerPolygons),
m_innerPolygons(other.m_innerPolygons),
m_outerBoundary(other.m_outerBoundary),
m_innerBoundary(other.m_innerBoundary)
{}

template<typename TPolygonContainer, typename TPolygon>
GeoMultiPolygon<TPolygonContainer, TPolygon>::GeoMultiPolygon(const GeoMultiPolygon & other) :
m_size(other.m_size),
m_outerPolygons(other.m_outerPolygons),
m_innerPolygons(other.m_innerPolygons),
m_outerBoundary(other.m_outerBoundary),
m_innerBoundary(other.m_innerBoundary)
{}

template<typename TPolygonContainer, typename TPolygon>
GeoMultiPolygon<TPolygonContainer, TPolygon>::~GeoMultiPolygon() {}

template<typename TPolygonContainer, typename TPolygon>
GeoMultiPolygon<TPolygonContainer, TPolygon> & GeoMultiPolygon<TPolygonContainer, TPolygon>::operator=(GeoMultiPolygon && other) {
	using std::swap;
	m_size = other.m_size;
	swap(m_outerPolygons, other.m_outerPolygons);
	swap(m_innerPolygons, other.m_innerPolygons);
	m_outerBoundary = other.m_outerBoundary;
	m_innerBoundary = other.m_innerBoundary;
	return *this;
}

template<typename TPolygonContainer, typename TPolygon>
GeoMultiPolygon<TPolygonContainer, TPolygon> & GeoMultiPolygon<TPolygonContainer, TPolygon>::operator=(const GeoMultiPolygon & other) {
	m_size = other.m_size;
	m_outerPolygons = other.m_outerPolygons;
	m_innerPolygons = other.m_innerPolygons;
	m_outerBoundary = other.m_outerBoundary;
	m_innerBoundary = other.m_innerBoundary;
	return *this;
}

template<typename TPolygonContainer, typename TPolygon>
GeoShapeType GeoMultiPolygon<TPolygonContainer, TPolygon>::type() const {
	return GS_MULTI_POLYGON;
}

template<typename TPolygonContainer, typename TPolygon>
uint32_t GeoMultiPolygon<TPolygonContainer, TPolygon>::size() const {
	return m_size;
}

template<typename TPolygonContainer, typename TPolygon>
GeoRect GeoMultiPolygon<TPolygonContainer, TPolygon>::boundary() const {
	return outerPolygonsBoundary();
}

template<typename TPolygonContainer, typename TPolygon>
bool GeoMultiPolygon<TPolygonContainer, TPolygon>::intersects(const GeoRect & boundary) const {
	if (this->outerPolygonsBoundary().overlap(boundary)) {
		bool intersects = false;
		for(typename PolygonList::const_iterator it(outerPolygons().cbegin()), end(outerPolygons().cend()); it != end; ++it) {
			if ((*it).intersects(boundary)) {
				intersects = true;
				break;
			}
		}
		if (intersects && this->innerPolygonsBoundary().overlap(boundary)) {
			GeoPolygon boundaryPoly(GeoPolygon::fromRect(boundary));
			for(typename PolygonList::const_iterator it(innerPolygons().cbegin()), end(innerPolygons().cend()); it != end; ++it) {
				if ((*it).encloses(boundaryPoly)) {
					intersects = false;
					break;
				}
			}
		}
		return intersects;
	}
	return false;
}

template<typename TPolygonContainer, typename TPolygon>
bool GeoMultiPolygon<TPolygonContainer, TPolygon>::contains(const GeoPoint & p) const {
	if (!outerPolygonsBoundary().contains(p.lat(), p.lon()))
		return false;
	bool contained = false;
	for(typename PolygonList::const_iterator it(outerPolygons().cbegin()), end(outerPolygons().cend()); it != end; ++it) {
		if ((*it).contains(p)) {
			contained = true;
			break;
		}
	}
	if (contained && innerPolygonsBoundary().contains(p.lat(), p.lon())) {
		for(typename PolygonList::const_iterator it(innerPolygons().cbegin()), end(innerPolygons().cend()); it != end; ++it) {
			if ((*it).contains(p)) {
				contained = false;
				break;
			}
		}
	}
	return contained;
}

template<typename TPolygonContainer, typename TPolygon>
bool GeoMultiPolygon<TPolygonContainer, TPolygon>::intersects(const GeoPoint & p1, const GeoPoint & p2) const {
	if (!outerPolygonsBoundary().contains(p1.lat(), p1.lon()) && !outerPolygonsBoundary().contains(p2.lat(), p2.lon()))
		return false;
	if (contains(p1) || contains(p2))
		return true;
	//check for intersection with any polygon
	for(typename PolygonList::const_iterator it(outerPolygons().cbegin()), end(outerPolygons().cend()); it != end; ++it) {
		if ((*it).intersects(p1, p2)) {
			return true;
		}
	}
	//Check for intersection with inner polygons. i.e. if both points lie within a star-shaped inner polygon
	for(typename PolygonList::const_iterator it(innerPolygons().cbegin()), end(innerPolygons().cend()); it != end; ++it) {
		if ((*it).intersects(p1, p2)) {
			return true;
		}
	}
	return false;
}

template<typename TPolygonContainer, typename TPolygon>
bool GeoMultiPolygon<TPolygonContainer, TPolygon>::intersects(const GeoRegion & other) const {
	if (other.type() <= type()) {
		switch (other.type()) {
		case GS_WAY:
			return collidesWithWay(*static_cast<const GeoWay*>(&other));
		case GS_POLYGON:
			return collidesWithPolygon(*static_cast<const GeoPolygon*>(&other));
		case GS_MULTI_POLYGON:
			return collidesWithMultiPolygon(*static_cast<const GeoMultiPolygon*>(&other));
		default:
			sserialize::UnimplementedFunctionException("Unable to do intersection with unknown type");
			return false;
		}
	}
	else {
		return other.intersects(*this);
	}
}

template<typename TPolygonContainer, typename TPolygon>
double
GeoMultiPolygon<TPolygonContainer, TPolygon>::distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const {
	GeoShapeType gt = other.type();
	if (gt <= GS_MULTI_POLYGON) {
		if (gt >= GS_POINT) {
			double d = std::numeric_limits<double>::max();
			for(typename PolygonList::const_iterator it(m_outerPolygons.cbegin()), end(m_outerPolygons.cend()); it != end; ++it) {
				d = std::min<double>(d, (*it).distance(other, distanceCalculator));
			}
			if (m_outerBoundary.overlap( other.boundary() )) {
				for(typename PolygonList::const_iterator it(m_innerPolygons.cbegin()), end(m_innerPolygons.cend()); it != end; ++it) {
					d = std::min<double>(d, (*it).distance(other, distanceCalculator));
				}
			}
		}
	}
	else {
		return other.distance(*this, distanceCalculator);
	}
	return std::numeric_limits<double>::quiet_NaN();
}

template<typename TPolygonContainer, typename TPolygon>
template<typename TIterator>
GeoRect GeoMultiPolygon<TPolygonContainer, TPolygon>::calcBoundary(TIterator it,  TIterator end) {
	if (it != end) {
		typename PolygonList::reference itP = *it;
		itP.recalculateBoundary();
		GeoRect b = itP.boundary();
		for(++it; it != end; ++it) {
			typename PolygonList::reference itP = *it;
			itP.recalculateBoundary();
			b.enlarge(itP.boundary());
		}
		return b;
	}
	return GeoRect();
}

template<typename TPolygonContainer, typename TPolygon>
void GeoMultiPolygon<TPolygonContainer, TPolygon>::recalculateBoundary() {
	m_innerBoundary = calcBoundary(m_innerPolygons.begin(), m_innerPolygons.end());
	m_outerBoundary = calcBoundary(m_outerPolygons.begin(), m_outerPolygons.end());
	auto c = [](uint32_t v, const GeoPolygon & a) {return v+a.size();};
	m_size = std::accumulate(m_innerPolygons.cbegin(), m_innerPolygons.cend(), static_cast<uint32_t>(0), c);
	m_size += std::accumulate(m_outerPolygons.cbegin(), m_outerPolygons.cend(), static_cast<uint32_t>(0), c);
}

template<typename TPolygonContainer, typename TPolygon>
sserialize::spatial::GeoPoint GeoMultiPolygon<TPolygonContainer, TPolygon>::at(uint32_t pos) const {
	if (pos <= m_size) {
		for(uint32_t i = 0, s = m_outerPolygons.size(); i < s; ++i) {
			typename TPolygonContainer::const_reference poly = m_outerPolygons.at(i);
			if (poly.size() > pos) {
				return poly.points().at(pos);
			}
			else {
				pos -= poly.size();
			}
		}
		for(uint32_t i = 0, s = m_innerPolygons.size(); i < s; ++i) {
			typename TPolygonContainer::const_reference poly = m_innerPolygons.at(i);
			if (poly.size() > pos) {
				return poly.points().at(pos);
			}
			else {
				pos -= poly.size();
			}
		}
	}
	return sserialize::spatial::GeoPoint();
}

template<typename TPolygonContainer, typename TPolygon>
bool GeoMultiPolygon<TPolygonContainer, TPolygon>::encloses(const TPolygon & polygon) const {
	if (!m_outerBoundary.contains(polygon.boundary())) {
		return false;
	}
	bool enclosed = false;
	for(typename PolygonList::const_iterator it(outerPolygons().cbegin()), end(outerPolygons().cend()); it != end; ++it) {
		if ((*it).encloses(polygon)) {
			enclosed = true;
			break;
		}
	}
	if (enclosed && innerPolygonsBoundary().overlap(polygon.boundary())) {
		for(typename PolygonList::const_iterator it(innerPolygons().cbegin()), end(innerPolygons().cend()); it != end; ++it) {
			if ((*it).intersects(polygon)) {
				enclosed = false;
				break;
			}
		}
	}
	return enclosed;
}

template<typename TPolygonContainer, typename TPolygon>
sserialize::UByteArrayAdapter & GeoMultiPolygon<TPolygonContainer, TPolygon>::append(sserialize::UByteArrayAdapter & dest) const {
	dest.putVlPackedUint32(m_size);
	dest << m_outerBoundary;
	dest << m_innerBoundary;
	dest << m_outerPolygons;
	dest << m_innerPolygons;
	return dest;
}

template<typename TPolygonContainer, typename TPolygon>
const GeoRect & GeoMultiPolygon<TPolygonContainer, TPolygon>::innerPolygonsBoundary() const { return m_innerBoundary; }

template<typename TPolygonContainer, typename TPolygon>
const GeoRect & GeoMultiPolygon<TPolygonContainer, TPolygon>::outerPolygonsBoundary() const { return m_outerBoundary; }

template<typename TPolygonContainer, typename TPolygon>
const typename GeoMultiPolygon<TPolygonContainer, TPolygon>::PolygonList & GeoMultiPolygon<TPolygonContainer, TPolygon>::innerPolygons() const { return m_innerPolygons; }

template<typename TPolygonContainer, typename TPolygon>
typename GeoMultiPolygon<TPolygonContainer, TPolygon>::PolygonList & GeoMultiPolygon<TPolygonContainer, TPolygon>::innerPolygons() { return m_innerPolygons; }

template<typename TPolygonContainer, typename TPolygon>
const typename GeoMultiPolygon<TPolygonContainer, TPolygon>::PolygonList & GeoMultiPolygon<TPolygonContainer, TPolygon>::outerPolygons() const { return m_outerPolygons; }

template<typename TPolygonContainer, typename TPolygon>
typename GeoMultiPolygon<TPolygonContainer, TPolygon>::PolygonList & GeoMultiPolygon<TPolygonContainer, TPolygon>::outerPolygons() { return m_outerPolygons; }

template<typename TPolygonContainer, typename TPolygon>
sserialize::spatial::GeoShape * GeoMultiPolygon<TPolygonContainer, TPolygon>::copy() const {
	return new GeoMultiPolygon<TPolygonContainer, TPolygon>(*this);
}

template<typename TPolygonContainer, typename TPolygon>
std::ostream & GeoMultiPolygon<TPolygonContainer, TPolygon>::asString(std::ostream & out) const {
	out << "GeoMultiPolygon[" << m_outerBoundary << m_innerBoundary << "{";
	for(typename PolygonList::const_iterator it(outerPolygons().cbegin()), end(outerPolygons().cend()); it != end; ++it) {
		(*it).asString(out);
	}
	for(typename PolygonList::const_iterator it(innerPolygons().cbegin()), end(innerPolygons().cend()); it != end; ++it) {
		(*it).asString(out);
	}
	out << "}]";
	return out;
}

}//end namespace detail

typedef sserialize::spatial::detail::GeoMultiPolygon< std::vector<sserialize::spatial::GeoPolygon>, sserialize::spatial::GeoPolygon > GeoMultiPolygon;

//template specialization
namespace detail {

template<>
UByteArrayAdapter & GeoMultiPolygon< std::vector<sserialize::spatial::GeoPolygon>, sserialize::spatial::GeoPolygon >::append(::sserialize::UByteArrayAdapter & destination) const;

}

}}

#endif