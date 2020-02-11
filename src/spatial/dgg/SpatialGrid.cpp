#include <sserialize/spatial/dgg/SpatialGrid.h>

namespace sserialize::spatial::dgg::interface {

std::string SpatialGrid::typeId() const {
	return name();
}

SpatialGrid::Size
SpatialGrid::childPosition(PixelId parent, PixelId child) const {
	uint32_t result = std::numeric_limits<uint32_t>::max();
	for(uint32_t i(0), s(childrenCount(parent)); i < s; ++i) {
		if (index(parent, i) == child) {
			return i;
		}
	}
	throw sserialize::spatial::dgg::exceptions::InvalidPixelId(this->to_string(child) + " is not a child of " + this->to_string(parent));
	return result;
}

std::string
SpatialGrid::to_string(PixelId pixel) const {
	return std::to_string(pixel);
}

SpatialGrid::SpatialGrid() {}

SpatialGrid::~SpatialGrid() {}

}//end namespace sserialize::spatial::dgg::interface
