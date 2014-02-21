#ifndef SSERIALIZE_GEO_HIERARCHY_H
#define SSERIALIZE_GEO_HIERARCHY_H
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/Static/GeoHierarchy.h>

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
		GeoRect boundary;
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
	Region m_rootRegion;
public:
	GeoHierarchy() {}
	virtual ~GeoHierarchy() {}
	///This sets the root region which has every node without a parent as child
	void createRootRegion();
	inline std::vector<Cell> & cells() { return m_cells; }
	inline std::vector<Region> & regions() { return m_regions;}
	inline const std::vector<Cell> & cells() const { return m_cells; }
	inline const std::vector<Region> & regions() const { return m_regions;}
	bool checkConsistency();
	///data structure has to be consistent before using this
	UByteArrayAdapter append(sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory& idxFactory) const;
	void printStats(std::ostream & out) const;
	bool testEquality(const sserialize::Static::spatial::GeoHierarchy & sgh) const;
};

}} //end namespace


#endif