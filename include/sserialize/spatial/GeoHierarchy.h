#ifndef SSERIALIZE_GEO_HIERARCHY_H
#define SSERIALIZE_GEO_HIERARCHY_H
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/Static/GeoHierarchy.h>

namespace sserialize {
namespace spatial {

class GeoHierarchy {
public:
	//Pointers are sorted

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
	std::vector<Region> m_regions; //the largest region is the last region
	Region m_rootRegion;
private:
	void depths(std::vector<uint32_t> & d, uint32_t me) const;
	template<typename T_SET_CONTAINER>
	void getAncestors(uint32_t regionId, T_SET_CONTAINER & out) const;
	//returned parents are sorted
	void splitCellParents(uint32_t cellId, std::vector<uint32_t> & directParents, std::vector<uint32_t> & remainingParents) const;
	//check functions
	bool regionEqTest(uint32_t i, const sserialize::spatial::GeoHierarchy::Region& r, const sserialize::Static::spatial::GeoHierarchy::Region& sr, const sserialize::Static::ItemIndexStore* idxStore) const;

public:
	GeoHierarchy() {}
	virtual ~GeoHierarchy() {}
	///This sets the root region which has every node without a parent as child
	void createRootRegion();
	inline std::vector<Cell> & cells() { return m_cells; }
	inline std::vector<Region> & regions() { return m_regions;}
	inline const std::vector<Cell> & cells() const { return m_cells; }
	inline const std::vector<Region> & regions() const { return m_regions;}
	inline const Cell & cell(uint32_t pos) const { return m_cells.at(pos);}
	inline Cell & cell(uint32_t pos) { return m_cells.at(pos);}
	inline const Region & region(uint32_t pos) const { return m_regions.at(pos);}
	inline Region & region(uint32_t pos) { return m_regions.at(pos);}
	bool checkConsistency();
	///data structure has to be consistent before using this
	UByteArrayAdapter append(sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory& idxFactory, bool fullItemsIndex = true) const;
	void printStats(std::ostream & out) const;
	bool testEquality(const sserialize::Static::spatial::GeoHierarchy & sgh, const sserialize::Static::ItemIndexStore * idxStore = 0) const;
	///get the regions in level order, where the level of a region is defined by the longest path from root to it self
	std::vector<uint32_t> getRegionsInLevelOrder() const;
	///@return first = id, second = size
	std::vector< std::pair< uint32_t, uint32_t > > createFullRegionItemIndex(sserialize::ItemIndexFactory& idxFactory) const;
	
	///This reduces the size of the hierarchy by eliminating redundancy:
	///For each Cell only those parents are kept, that are not an ancestor of another parent of the cell
	///For each region, only those cells are kept, that are not part of a descendant of the region
	///Calculating aproximate region item counts during completion is then more involved
	///This should only be called before serialization
	void compactify(bool compactifyCells, bool compactifyRegions);
};

template<typename T_SET_CONTAINER>
void GeoHierarchy::getAncestors(uint32_t regionId, T_SET_CONTAINER & out) const {
	const Region & r = m_regions[regionId];
	out.insert(r.parents.cbegin(), r.parents.cend());
	for(uint32_t parent : r.parents) {
		getAncestors(parent, out);
	}
}


}} //end namespace


#endif