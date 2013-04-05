#ifndef SSERIALIZE_SPATIAL_RTREE_H
#define SSERIALIZE_SPATIAL_RTREE_H
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/containers/ItemIndexFactory.h>

/** This is a simple implementation of RTree (not a real RTree as it doesn't use pages)
  *
  *
  *
  */


namespace sserialize {
namespace spatial {

class RTree {
public:
	struct Node {
		bool leafNode;
		spatial::GeoRect rect;
	};
	
	struct InnerNode: Node {
		std::vector<Node*> children;
	};
	
	struct LeafNode: Node {
		std::vector<uint32_t> items;
	};
private:
	Node * m_root;
private:
	std::vector<Node*> nodesInLevelOrder();
	void sserializeNode(Node * node, const std::vector<uint32_t> & children, uint32_t indexId);
protected:
	Node* & rootNode() { return m_root; }
public:
	RTree();
	virtual ~RTree() {}
	void sserialize(sserialize::UByteArrayAdapter & dest, ItemIndexFactory & indexFactory) const;
};

}}//end namespace

#endif