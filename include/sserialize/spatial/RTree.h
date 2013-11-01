#ifndef SSERIALIZE_SPATIAL_RTREE_H
#define SSERIALIZE_SPATIAL_RTREE_H
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/containers/ItemIndexFactory.h>

/** This is a simple implementation of R-Tree like tree (not a real RTree as it doesn't use pages)
  *
  *
  *
  */


namespace sserialize {
namespace spatial {

class RTree {
public:
	struct Node {
		///This will destroy all children!
		virtual ~Node() {}
		bool leafNode;
		spatial::GeoRect rect;
	};
	
	struct InnerNode: Node {
		virtual ~InnerNode() {
			for(uint32_t i = 0; i< children.size(); ++i) {
				if (children[i])
					delete children[i];
			}
		}
		std::vector<Node*> children;
	};
	
	struct LeafNode: Node {
		virtual ~LeafNode() {}
		std::vector<uint32_t> items;
	};
private:
	Node * m_root;
private:
	std::vector<Node*> nodesInLevelOrder() const;
	void sserializeNode(Node * node, const std::vector<uint32_t> & children, uint32_t indexId);
protected:
	Node* & rootNode() { return m_root; }
public:
	RTree() : m_root(0) {}
	virtual ~RTree() {
		delete m_root;
	}
	void serialize(sserialize::UByteArrayAdapter & dest, sserialize::ItemIndexFactory & indexFactory);
};

}}//end namespace

#endif