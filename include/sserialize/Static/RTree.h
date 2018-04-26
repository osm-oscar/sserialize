#ifndef SSERIALIZE_STATIC_SPATIAL_RTREE_H
#define SSERIALIZE_STATIC_SPATIAL_RTREE_H
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/spatial/GeoRect.h>
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
  * v32    | v32 |(GeoRect|v32)
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
		UByteArrayAdapter m_childrenData;
		uint32_t m_indexPtr;
		uint32_t m_size;
		std::vector<sserialize::spatial::GeoRect> m_rects;
		std::vector<uint32_t> m_childrenPtr;
	private:
		inline UByteArrayAdapter childDataAt(uint32_t pos) const {
			return m_childrenData+m_childrenPtr[pos];
		}
	public:
		Node();
		Node(const UByteArrayAdapter & data);
		virtual ~Node() {}
		inline uint32_t size() const { return m_size; }
		inline const sserialize::spatial::GeoRect rectAt(uint32_t pos) const {
			return m_rects[pos];
		}
		
		inline Node childAt(uint32_t pos) const {
			return Node(childDataAt(pos));
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
	void intersectRecurse(const Node & node, const sserialize::spatial::GeoRect & rect, DynamicBitSet & dest, const ElementIntersecter * intersecter);
	
	void intersectRecurse(const Node & node, const sserialize::spatial::GeoRect & rect, ItemIndex & fullyContained, ItemIndex & intersected);
	
public:
	RTree();
	RTree(const sserialize::UByteArrayAdapter & data, const Static::ItemIndexStore & indexStore);
	virtual ~RTree();
	///@param intersecter: if this is not null, then this class is used to check the elements
	void intersect(const sserialize::spatial::GeoRect & rect, DynamicBitSet & dest, ElementIntersecter * intersecter);
	///@param intersecter: if this is not null, then this class is used to check the elements
	ItemIndex intersect(const sserialize::spatial::GeoRect & rect, ElementIntersecter * intersecter);
	const sserialize::Static::ItemIndexStore & indexStore() const { return m_indexStore; }
	sserialize::Static::ItemIndexStore & indexStore() { return m_indexStore; }
	const sserialize::spatial::GeoRect & boundary() const { return m_rootBoundary; }
};

}}}//end namespace
  
#endif
