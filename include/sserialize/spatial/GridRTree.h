#ifndef SSERIALIZE_GRID_RTREE_H
#define SSERIALIZE_GRID_RTREE_H
#include <sserialize/spatial/RTree.h>
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/spatial/ItemGeoGrid.h>
#include <unordered_map>

namespace sserialize {
namespace spatial {

class GridRTree: public RTree {
public:
	typedef std::unordered_map<uint32_t, const sserialize::spatial::GeoShape*> BulkLoadType;
private:
	GeoRect m_bbox;
	uint32_t m_latCount;
	uint32_t m_lonCount;
private:
	RTree::Node * createTree(ItemGeoGrid & grid, uint32_t xmin, uint32_t ymin, uint32_t xmax, uint32_t ymax);
public:
	GridRTree(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount) : m_bbox(rect), m_latCount(latcount), m_lonCount(loncount) {}
	virtual ~GridRTree() {}
	
	///@param 
	template<typename ItemGenerator>
	void bulkLoad(ItemGenerator generator) {
		sserialize::spatial::ItemGeoGrid grid(m_bbox, m_latCount, m_lonCount);
		std::cout << "GridRTree::bulkLoadaing: creating grid..." << std::flush;
		for(; generator.valid(); generator.next()) {
			grid.addItem(generator.id(), generator.shape());
		}
		std::cout << "done" << std::endl;
		rootNode() = createTree(grid, 0, 0, m_latCount, m_lonCount);
	}
};

}}//end namespace



#endif