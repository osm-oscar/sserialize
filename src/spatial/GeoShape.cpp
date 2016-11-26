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

}}//end namespace sserialize::spatial