#ifndef SSERIALIZE_GEO_HIERARCHY_H
#define SSERIALIZE_GEO_HIERARCHY_H
#include <sserialize/Static/Deque.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/spatial/GeoShape.h>

namespace sserialize {
namespace spatial {

class GeoHierarchy {
public:

	struct Region {
		std::vector<uint32_t> children;
		std::vector<uint32_t> parents;
		std::vector<uint32_t> cells;
		sserialize::spatial::GeoShapeType type;
		uint32_t id;
	};

	struct Cell {
		std::vector<uint32_t> parents;
		std::vector<uint32_t> items;
	};
	
	struct Way {
		std::vector<uint32_t> parents;
		std::vector<uint32_t> items;
	};
private:
	std::vector<Cell> m_cells;
	std::vector<Region> m_regions;
public:
	GeoHierarchy() {}
	virtual ~GeoHierarchy() {}
	inline std::vector<Cell> & cells() { return m_cells; }
	inline std::vector<Region> & regions() { return m_regions;}
	inline const std::vector<Cell> & cells() const { return m_cells; }
	inline const std::vector<Region> & regions() const { return m_regions;}
	bool make_consistend();
	///data structure has to be consistent before using this
	bool append(UByteArrayAdapter & dest, ItemIndexFactory & idxFactory) const;
};

}} //end namespace


#endif