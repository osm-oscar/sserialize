#include <sserialize/spatial/GeoShape.h>
#include <iostream>

namespace sserialize {
namespace spatial {

void GeoShape::dump() const {
	this->asString(std::cout);
}

}}//end namespace sserialize::spatial