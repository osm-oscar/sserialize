#include <sserialize/spatial/GeoMultiPolygon.h>
#include <numeric>


namespace sserialize {
namespace spatial {


GeoMultiPolygon::GeoMultiPolygon() {}
GeoMultiPolygon::~GeoMultiPolygon() {}


uint32_t GeoMultiPolygon::size() const {
	return m_size;
}

GeoRect GeoMultiPolygon::boundary() const {
	return m_outerBoundary;
}

bool GeoMultiPolygon::intersects(const GeoRect & boundary) const {
	if (this->innerBoundary().overlap(boundary)) {
		bool intersects = false;
		for(PolygonList::const_iterator it(m_innerPolygons.begin()), end(m_innerPolygons.end()); it != end; ++it) {
			if (it->intersects(boundary)) {
				intersects = true;
				break;
			}
		}
		if (intersects && this->outerBoundary().overlap(boundary)) {
			for(PolygonList::const_iterator it(m_outerPolygons.begin()), end(m_outerPolygons.end()); it != end; ++it) {
				if (it->intersects(boundary)) {
					intersects = false;
					break;
				}
			}
		}
		return intersects;
	}
	return false;
}

template<typename TIterator>
GeoRect calcBoundary(TIterator it,  TIterator end) {
	if (it != end) {
		GeoRect b = it->boundary();
		for(++it; it != end; ++it) {
			b.enlarge(it->boundary());
		}
		return b;
	}
	return GeoRect();
}

void GeoMultiPolygon::recalculateBoundaries() {
	m_innerBoundary = calcBoundary(m_innerPolygons.begin(), m_innerPolygons.end());
	m_outerBoundary = calcBoundary(m_outerPolygons.begin(), m_outerPolygons.end());
	auto c = [](uint32_t v, const GeoPolygon & a) {return v+a.size();};
	m_size = std::accumulate(m_innerPolygons.begin(), m_innerPolygons.end(), static_cast<uint32_t>(0), c);
	m_size = std::accumulate(m_outerPolygons.begin(), m_outerPolygons.end(), m_size, c);
}
	
const GeoRect & GeoMultiPolygon::innerBoundary() const { return m_innerBoundary; }
const GeoRect & GeoMultiPolygon::outerBoundary() const { return m_outerBoundary; }

const GeoMultiPolygon::PolygonList & GeoMultiPolygon::innerPolygons() const { return m_innerPolygons; }
GeoMultiPolygon::PolygonList & GeoMultiPolygon::innerPolygons() { return m_innerPolygons; }

const GeoMultiPolygon::PolygonList & GeoMultiPolygon::outerPolygons() const { return m_outerPolygons; }
GeoMultiPolygon::PolygonList & GeoMultiPolygon::outerPolygons() { return m_outerPolygons; }

bool GeoMultiPolygon::test(const GeoMultiPolygon::Point & p) const {
	if (!m_innerBoundary.contains(p.lat, p.lon))
		return false;
	bool contained = false;
	for(PolygonList::const_iterator it(m_innerPolygons.begin()), end(m_innerPolygons.end()); it != end; ++it) {
		if (it->test(p)) {
			contained = true;
			break;
		}
	}
	if (contained && m_outerBoundary.contains(p.lat, p.lon)) {
		for(PolygonList::const_iterator it(m_outerPolygons.begin()), end(m_outerPolygons.end()); it != end; ++it) {
			if (it->test(p)) {
				contained = false;
				break;
			}
		}
	}
	return contained;
}

bool GeoMultiPolygon::test(const std::deque<GeoMultiPolygon::Point> & ps) const {
	for(const auto & p : ps) {
		if (test(p)) {
			return true;
		}
	}
	return false;
}

bool GeoMultiPolygon::test(const std::vector<GeoMultiPolygon::Point> & ps) const {
	for(const auto & p : ps) {
		if (test(p)) {
			return true;
		}
	}
	return false;
}

}}