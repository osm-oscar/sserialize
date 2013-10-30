#ifndef SSERIALIZE_QUAD_RTREE_H
#define SSERIALIZE_QUAD_RTREE_H
#include <sserialize/spatial/RTree.h>
#include <sserialize/spatial/GeoRect.h>
#include <unordered_map>

namespace sserialize {
namespace spatial {

class QuadRTree: public RTree {
public:
	QuadRTree();
	virtual ~QuadRTree();
	void bulkLoad(const std::unordered_map<uint32_t, sserialize::spatial::GeoRect> & idToRect);
};

}}//end namespace



#endif