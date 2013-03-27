#ifndef SSERIALIZE_SPATIAL_RTREE_H
#define SSERIALIZE_SPATIAL_RTREE_H
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/utility/UByteArrayAdapter.h>

/** This is a simple implementation of RTree (not a real RTree as it doesn't use pages)
  *
  *
  *
  */


namespace sserialize {
namespace spatial {

class RTree {
protected:
	struct Node {
		bool leafNode;
	};
	
	struct InnerNode {
		struct Description {
			Node * d;
			spatial::GeoRect rect;
		};
		std::vector<Description> children;
	};
	
	struct LeafNode {
		std::vector<uint32_t> items;
	};
private:
	Node * m_root;
private:
	std::vector<Node*> nodesInLevelOrder();
	void sserializeNode(Node * node, const std::vector<uint32_t> children, uint32_t indexId);
protected:
	Node* & rootNode() { return m_root; }
public:
	RTree();
	virtual ~RTree() {}
	virtual void bulkLoad() = 0;
	
	void sserialize(sserialize::UByteArrayAdapter & dest) const;
};

class SimpleRTree: public RTree {
public:
	SimpleRTree();
	virtual ~SimpleRTree();
	virtual void bulkLoad();
};

}}//end namespace

#endif