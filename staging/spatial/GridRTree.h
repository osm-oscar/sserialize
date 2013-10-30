#ifndef SSERIALIZE_GRID_RTREE_H
#define SSERIALIZE_GRID_RTREE_H
#include <staging/spatial/RTree.h>
#include <sserialize/spatial/GeoShape.h>
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
public:
	GridRTree(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount) : m_bbox(rect), m_latCount(latcount), m_lonCount(loncount) {}
	virtual ~GridRTree() {}
	void bulkLoad(const BulkLoadType & idToRect);
};

}}//end namespace



#endif