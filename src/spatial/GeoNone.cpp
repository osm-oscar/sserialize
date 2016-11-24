#include <sserialize/spatial/GeoNone.h>

namespace sserialize {
namespace spatial {

GeoNone::GeoNone() {}

GeoNone::~GeoNone() {}

GeoShapeType GeoNone::type() const {
	return GS_NONE;
}

uint32_t GeoNone::size() const {
	return 0;
}

GeoRect GeoNone::boundary() const {
	return GeoRect();
}

void GeoNone::recalculateBoundary() {}

bool GeoNone::intersects(const GeoRect &) const {
	return false;
}

double GeoNone::distance(const sserialize::spatial::GeoShape &, const sserialize::spatial::DistanceCalculator &) const {
	return std::numeric_limits<double>::quiet_NaN();
}

UByteArrayAdapter & GeoNone::append(sserialize::UByteArrayAdapter & dest) const {
	return dest;
}

GeoShape * GeoNone::copy() const {
	return new GeoNone();
}

std::ostream & GeoNone::asString(std::ostream & out) const {
	return out << "GS_NONE";
}

}} //end namespace sserialize::spatial