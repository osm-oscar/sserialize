#include <sserialize/spatial/GeoShape.h>
#include <iostream>

namespace sserialize {
namespace spatial {

std::string GeoShape::toString() const {
	std::stringstream ss;
	this->asString(ss);
	return ss.str();
}

void GeoShape::dump() const {
	this->asString(std::cout);
}


std::string to_string(GeoShapeType gst) {
	switch(gst) {
	case GS_POINT:
		return "point";
	case GS_WAY:
		return "way";
	case GS_POLYGON:
		return "polygon";
	case GS_MULTI_POLYGON:
		return "multi-polygon";
	default:
		return "invalid";
	};
}

}}//end namespace sserialize::spatial
