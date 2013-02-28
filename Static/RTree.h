#ifndef SSERIALIZE_STATIC_RTREE_H
#define SSERIALIZE_STATIC_RTREE_H
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexIterator.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/Static/GeoPoint.h>
#include "Deque.h"
#include <list>

namespace sserialize {
namespace Static {
namespace spatial {

/** 
  * The Tree is serialized in Top-Down-level-order 
  * This way one only needs to store the id of the first child nodes as one can calculate the other node ids with it
  * 
  * NodeLayout:
  *
  *------------------------------------------------------------
  *SizeType|IndexStore-ItemIndexId/IdOfFirstChildNode|GeoRects|
  *------------------------------------------------------------
  * vlu32  |               vlu32                     |   *
  *------------------------------------------------------------
  *
  * TreeLayout:
  *------------------------
  *Static::Deque<RTreeNode>
  *------------------------
  *
  */
class RTreeNode {
private:
	UByteArrayAdapter m_data;
	uint32_t m_sizeType;
	uint32_t m_idPtr;
public:
	RTreeNode() : m_sizeType(0), m_idPtr(0) {}
	RTreeNode(const UByteArrayAdapter & data) :
	m_data(data),
	m_sizeType(m_data.getVlPackedUint32()),
	m_idPtr(m_data.getVlPackedUint32()) {
		m_data.shrinkToGetPtr();
	}
	virtual ~RTreeNode() {}
	bool isLeaf() { return m_sizeType & 0x1; }
	inline uint32_t size() const { return size >> 1; }
	///this either returns the ItemIndexId in case this node is a leaf node, or the id of the first child
	inline uint32_t idPtr() const { return m_idPtr; }
	template<typename TPushBackContainer>
	void selectChildren(const sserialize::spatial::GeoRect & rect, TPushBackContainer & definite, TPushBackContainer & possible) const {
		uint32_t mySize = size();
		UByteArrayAdapter rectData( m_data );
		for(uint32_t i = 0, childId = m_idPtr; i < mySize; ++i, ++childId) {
			Static::GeoPoint bL(rectData);
			rectData += GeoPoint::SERIALIZED_SIZE;
			GeoPoint tR(rectData);
			rectData += GeoPoint::SERIALIZED_SIZE;
			if ()
		}
	}
};

template<typename DataBaseType>
class RTree {
	Static::Deque<RTreeNode> m_nodes;
	ItemIndexStore m_idxStore;
	DataBaseType m_db;
public:
	RTree(const UByteArrayAdapter & data, const DataBaseType & db, ItemIndexStore & idxStore);
	virtual ~RTree();
	
	ItemIndex complete(const sserialize::spatial::GeoRect & rect, bool approximate) const {
		if (!m_nodes.size())
			return ItemIndex();
		DynamicBitSet definiteItems;
		DynamicBitSet possibleItems;
		std::list<uint32_t> nodesToVisit;
		nodesToVisit.push_back(0); //rootNode
		while(!nodesToVisit.empty()) {
			RTreeNode node( m_nodes.at(nodesToVisit.front()) );
			nodesToVisit.pop_front();
			if (node.isLeaf()) {
				m_idxStore.at( node.idPtr() ).putInto(resultSet);
			}
			else {
				std::vector<uint32_t> nextIds = node.select(rect);
				nodesToVisit.insert(nodesToVisit.end(), nextIds.begin(), nextIds.end());
			}
		}
		
		if (approximate) {
			return ;
		}
		
		std::vector<uint32_t> res(resultSet.begin(), resultSet.end());
		return ItemIndex::absorb( res );
	}

	ItemIndexIterator partialComplete(const sserialize::spatial::GeoRect & rect, bool approximate) const {
		return ItemIndexIterator( complete(rect, approximate) );
	}
	
	ItemIndex filter(const sserialize::spatial::GeoRect & rect, bool approximate, const ItemIndex & partner) const {
		return complete(rect, approximate) / partner;
	}
	
	ItemIndexIterator filter(const sserialize::spatial::GeoRect & rect, bool approximate, const ItemIndexIterator & partner) const {
		return db().filter(rect, approximate, partner);
	}
};

}}}//end namespace


#endif