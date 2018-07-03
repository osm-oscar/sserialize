#ifndef SSERIALIZE_SPATIAL_GRID_TREE_ARRANGEMENT_H
#define SSERIALIZE_SPATIAL_GRID_TREE_ARRANGEMENT_H
#include <sserialize/spatial/GridRegionTree.h>
#include <sserialize/containers/OOMArray.h>
#include <sserialize/spatial/GeoShape.h>

namespace sserialize {
namespace spatial {

	
//Maps nodeId -> contained items
//nodeId->intersecting region
class GridTreeArrangement {
public:
	struct ItemEntry {
		//the lowest node this is in
		uint32_t nodeId;
		uint32_t itemId;
	};
	struct RegionEntry {
		//the lowest node this is in
		uint32_t nodeId;
		uint32_t itemId;
	};
public:
	GridTreeArrangement();
	void addItem(GeoShape * shape, uint32_t itemId);
	void finalize();
private:
	sserialize::OOMArray<ItemEntry> m_items;
	sserialize::OOMArray<RegionEntry> m_regions;
};
	
}}//end namespace

#endif
