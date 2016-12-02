#include <sserialize/spatial/GeoUnionShape.h>

namespace sserialize {
namespace spatial {

GeoUnionShape::GeoUnionShape(const GeoUnionShape::ShapeIdStore& shapes, const RCPtrWrapper< GeoShapeFactory >& factory) :
m_shapes(shapes),
m_factory(factory),
m_pointCount(0)
{}

GeoUnionShape::~GeoUnionShape() {}

GeoShapeType GeoUnionShape::type() const {
	return GS_UNION_SHAPE;
}

uint32_t GeoUnionShape::size() const {
	if (m_pointCount) {
		//To be fully conformant m_pointCount would need to be protected by an std::atomic wrapper
		//But almost all platforms schould do atomic stores for integral types
		//So the worst thing that can happen is multiple size calculations by multiple threads
		uint32_t tmp = 0;
		for(uint32_t i(0), s(shapeCount()); i < s; ++i) {
			GeoShape * gs = shape(i);
			tmp += gs->size();
			delete gs;
		}
		m_pointCount = tmp;
	}
	return m_pointCount;
}

GeoRect GeoUnionShape::boundary() const {
	if (!m_bounds.valid()) {
		const_cast<GeoUnionShape*>(this)->recalculateBoundary();
	}
	return m_bounds;
}

void GeoUnionShape::recalculateBoundary() {
	sserialize::spatial::GeoRect tmp = sserialize::spatial::GeoRect();
	for(uint32_t i(0), s(shapeCount()); i < s; ++i) {
		GeoShape * gs = shape(i);
		tmp.enlarge(gs->boundary());
		delete gs;
	}
	//minimize the time that we have an invalid state
	//This is of course not thread-safe since we have at least 4 stores to integral data types
	//Making this "valid" code would introduce a mutex for almost no reason
	m_bounds = tmp;
}

bool GeoUnionShape::intersects(const GeoRect & boundary) const {
	bool doesIntersect = false;
	for(uint32_t i(0), s(shapeCount()); !doesIntersect && i < s; ++i) {
		GeoShape * gs = shape(i);
		if (gs->intersects(boundary)) {
			doesIntersect = true;
		}
		delete gs;
	}
	return doesIntersect;
}

double GeoUnionShape::distance(const sserialize::spatial::GeoShape & other, const sserialize::spatial::DistanceCalculator & distanceCalculator) const {
	double dist = std::numeric_limits<double>::max();
	for(uint32_t i(0), s(shapeCount()); i < s; ++i) {
		GeoShape * gs = shape(i);
		dist = std::min<double>( dist, gs->distance(other, distanceCalculator) );
		delete gs;
	}
	return dist;
}

UByteArrayAdapter & GeoUnionShape::append(sserialize::UByteArrayAdapter & destination) const {
	throw sserialize::UnsupportedFeatureException("sserialize::spatial::GeoUnionShape does not support serialization");
	return destination;
}

GeoShape * GeoUnionShape::copy() const {
	return new GeoUnionShape(m_shapes, m_factory);
}

uint32_t GeoUnionShape::shapeCount() const {
	return (uint32_t) m_shapes.size();
}

GeoShape * GeoUnionShape::shape(uint32_t pos) const {
	return m_factory->shape(m_shapes.at(pos));
}


}}//end namespace sserialize::spatial