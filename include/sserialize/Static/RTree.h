#ifndef SSERIALIZE_STATIC_SPATIAL_RTREE_H
#define SSERIALIZE_STATIC_SPATIAL_RTREE_H
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/spatial/GeoRect.h>
#include <boost-1_49/boost/concept_check.hpp>
#define SSERIALIZE_STATIC_SPATIAL_RTREE_VERSION 0

/** File format
  *
  *--------------------------------------------
  *VERSION|ROOT_NODE_RECT|TREE_NODES*
  *--------------------------------------------
  *
  *NodeLayout:
  *----------------------------------
  *IndexPtr|COUNT|(Rects|ChildPtrs)*
  *----------------------------------
  *
  *
  *
  *
  */

namespace sserialize {
namespace Static {
namespace spatial {

class RTree {
private:
	class Node {
		UByteArrayAdapter m_data;
		uint32_t m_indexPtr;
		uint32_t m_size;
		std::vector<sserialize::spatial::GeoRect> m_rects;
		std::vector<uint32_t> m_childrenPtr;
	private:
		inline UByteArrayAdapter childDataAt(uint32_t pos) const {
			return m_data+m_childrenPtr[pos];
		}
	public:
		Node();
		Node(const UByteArrayAdapter & data);
		virtual ~Node() {}
		inline uint32_t size() const { return m_size; }
		inline const sserialize::spatial::GeoRect rectAt(uint32_t pos) const {
			return m_rects[pos];
		}
		
		inline std::shared_ptr<Node> childAt(uint32_t pos) const {
			return std::shared_ptr<Node>(new Node(childDataAt(pos)));
		}
		inline uint32_t indexPtr() const { return m_indexPtr; }
		inline uint32_t childIndexPtr(uint32_t pos) const {
			return Node::nodeIndexPtr(childDataAt(pos));
		}
		inline static uint32_t nodeIndexPtr(const UByteArrayAdapter & data) {
			return data.getVlPackedUint32(0, 0);
		}
	};
public:
	class ElementIntersecter {
	public:
		ElementIntersecter() {}
		virtual ~ElementIntersecter() {}
		virtual bool operator()(uint32_t id, const sserialize::spatial::GeoRect & rect) const = 0;
	};
private:
	sserialize::spatial::GeoRect m_rootBoundary;
	std::shared_ptr<Node> m_root;
	sserialize::Static::ItemIndexStore m_indexStore;
private:
	inline void handleIndex(const sserialize::spatial::GeoRect & rect, uint32_t id, DynamicBitSet & dest, const ElementIntersecter * intersecter);
	void intersectRecurse(const std::shared_ptr<Node> & node, const sserialize::spatial::GeoRect & rect, DynamicBitSet & dest, const ElementIntersecter * intersecter);
public:
	RTree();
	RTree(const sserialize::UByteArrayAdapter & data, const Static::ItemIndexStore & indexStore);
	virtual ~RTree();
	///@param intersecter: if this is not null, then this class is used to check the elements
	void intersect(const sserialize::spatial::GeoRect & rect, DynamicBitSet & dest, ElementIntersecter * intersecter);
};

}}}//end namespace
  
#endif