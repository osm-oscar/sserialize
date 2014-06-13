#ifndef SSERIALIZE_SPATIAL_GRID_REGION_TREE_H
#define SSERIALIZE_SPATIAL_GRID_REGION_TREE_H
#include <sserialize/spatial/GeoRegion.h>
#include <sserialize/spatial/GeoGrid.h>
#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/spatial/GeoMultiPolygon.h>
#include <unordered_set>

namespace sserialize {
namespace spatial {
namespace detail {
namespace GridRegionTree {

struct Node {
	typedef enum {NT_INVALID=0, NT_INTERNAL=1, NT_LEAF=2} NodeType;
	union Data {
		struct {
			uint64_t d0;
			uint64_t d1;
		} raw;
		struct {
			uint64_t type:2;
			uint64_t d1:62;
			uint64_t d2;
		} all;
		struct InternalLeaf {
			uint64_t type:2;
			uint64_t padding1:30;
			uint64_t valueBegin:30;
			uint64_t padding2:2;
			uint64_t enclosedCount:24;
			uint64_t padding3:40;
		} internalLeaf;
		struct Internal {
			uint64_t type:2;
			uint64_t gridPtr:30;//max 1 billion internal nodes
			uint64_t valueBegin:30;
			uint64_t padding:2;
			uint64_t enclosedCount:24;
			uint64_t childrenBegin:40;
		} internal;
		struct Leaf {
			uint64_t type:2;
			uint64_t padding1:30;
			uint64_t valueBegin:30; //max 1 billion leaf nodes
			uint64_t padding2:2;
			uint64_t enclosedCount:24; //max 24 million enclosed
			uint64_t intersectedCount:24; //max 24 million intersected
			uint64_t padding3:16;
		} leaf;
		Data() : raw{0, 0} {}
	} d;
	Node() {
		d.all.type = NT_INVALID;
	}
	Node(NodeType t) {
		d.all.type = t;
	}
	bool valid() const { return d.all.type != NT_INVALID; }
	NodeType type() const { return (NodeType) d.all.type; }
	Data::Internal & internal() { return d.internal; }
	const Data::Internal & internal() const { return d.internal; }
	Data::Leaf & leaf() { return d.leaf; }
	const Data::Leaf & leaf() const { return d.leaf; }
	Data::InternalLeaf & internalLeaf() { return d.internalLeaf; }
	const Data::InternalLeaf & internalLeaf() const { return d.internalLeaf; }
};

class FixedSizeRefiner {
private:
	double m_minLatStep;
	double m_minLonStep;
	uint32_t m_latCount;
	uint32_t m_lonCout;
public:
	FixedSizeRefiner(double minLatStep, double minLonStep, uint32_t latCount, uint32_t lonCount);
	~FixedSizeRefiner() {}
	bool operator()(const sserialize::spatial::GeoRect& maxBounds, const std::vector<sserialize::spatial::GeoRegion*>& rId2Ptr, const std::vector<uint32_t> & sortedRegions, sserialize::spatial::GeoGrid & newGrid) const;
};

}}//end namespace detail

class GridRegionTree {
private:
	typedef detail::GridRegionTree::Node Node;
public:
	static const uint32_t NullNodePtr;
	typedef detail::GridRegionTree::FixedSizeRefiner FixedSizeRefiner;
private:
	std::vector<Node> m_nodes;
	std::vector<GeoGrid> m_nodeGrids;
	std::vector<uint32_t> m_nodePtr;
	std::vector<uint32_t> m_leafInfo;
	std::vector<GeoRegion*> m_regions;
private:
	template<typename T_REGION_ID_ITERATOR, typename T_OUTPUT_ITERATOR>
	void getEnclosing(const GeoRect & bounds, T_REGION_ID_ITERATOR begin, const T_REGION_ID_ITERATOR & end, T_OUTPUT_ITERATOR out);
	
	template<typename T_GRID_REFINER>
	uint32_t insert(const T_GRID_REFINER & refiner, const std::unordered_map<GeoRegion*, uint32_t> & rPtr2Id, std::vector<uint32_t> sortedRegions, const sserialize::spatial::GeoRect & maxBounds);
public:
	GridRegionTree() {}
	///@param refiner: refines a given node: bool operator()(GeoRect maxBounds, std::unordered_set<uint32_t> regionids, GeoGrid & newGrid) const,
	///@return false if no further refinement should happen
	template<typename T_GEOREGION_ITERATOR, typename T_GRID_REFINER>
	GridRegionTree(const GeoGrid & initial, T_GEOREGION_ITERATOR begin, T_GEOREGION_ITERATOR end, T_GRID_REFINER refiner) {
		std::unordered_map<GeoRegion*, uint32_t> rPtr2IdH;
		m_regions.reserve(end-begin);
		rPtr2IdH.reserve(m_regions.capacity());
		for(T_GEOREGION_ITERATOR it(begin); it != end; ++it) {
			rPtr2IdH[*it] = m_regions.size();
			m_regions.push_back(*it);
		}
		
		const uint32_t nodePtr = 0;
		const uint32_t childPtr = 0;
		const uint32_t gridPtr = 0;
		
		m_nodes.push_back(Node(Node::NT_INTERNAL));
		m_nodeGrids.push_back(initial);
		m_nodePtr.resize(initial.tileCount(), sserialize::spatial::GridRegionTree::NullNodePtr);
		
		m_nodes[nodePtr].internal().childrenBegin = childPtr;
		m_nodes[nodePtr].internal().gridPtr = gridPtr;

		for(uint32_t tile=0, ts=initial.tileCount(); tile < ts; ++tile) {
			GeoGrid::GridBin gridBin(initial.select(tile));
			std::vector<uint32_t> tmp;
			GeoRect tmpRect(initial.cellBoundary(gridBin));
			for(uint32_t rIt(0), rEnd(m_regions.size()); rIt != rEnd; ++rIt) {
				if (m_regions[rIt]->intersects(tmpRect)) {
					tmp.push_back(rIt);
				}
			}
			if (tmp.size()) {
				//put this into a temporary as m_nodePtr will likely change during recursion and therefore the fetched adress on the left is wrong
				uint32_t tmpRet = insert(refiner, rPtr2IdH, tmp, tmpRect);
				m_nodePtr[childPtr+gridBin.tile] = tmpRet;
			}
		}
	}
	virtual ~GridRegionTree() {}
	void shrink_to_fit();
	template<typename T_INSERT_ITERATOR1, typename T_INSERT_ITERATOR2>
	void find(const sserialize::spatial::GeoPoint & p, T_INSERT_ITERATOR1 intersecting, T_INSERT_ITERATOR2 enclosing) const;
};

template<typename T_REGION_ID_ITERATOR, typename T_OUTPUT_ITERATOR>
void GridRegionTree::getEnclosing(const sserialize::spatial::GeoRect& bounds, T_REGION_ID_ITERATOR begin, const T_REGION_ID_ITERATOR& end, T_OUTPUT_ITERATOR out) {
	sserialize::spatial::GeoPolygon boundsPoly(sserialize::spatial::GeoPolygon::fromRect(bounds));
	for(; begin != end; ++begin) {
		spatial::GeoRegion * r = m_regions[*begin];
		switch (r->type()) {
		case sserialize::spatial::GS_POLYGON:
			if (static_cast<const GeoPolygon*>(r)->encloses(boundsPoly)) {
				*out = *begin;
				++out;
			}
			break;
		case sserialize::spatial::GS_MULTI_POLYGON:
			if (static_cast<const GeoMultiPolygon*>(r)->encloses(boundsPoly)) {
				*out = *begin;
				++out;
			}
			break;
		default:
			break;
		}
	}
}

template<typename T_GRID_REFINER>
uint32_t GridRegionTree::insert(const T_GRID_REFINER& refiner, const std::unordered_map< sserialize::spatial::GeoRegion*, uint32_t >& rPtr2Id, std::vector< uint32_t > sortedRegions, const sserialize::spatial::GeoRect& maxBounds) {
	GeoGrid newGrid;
	uint32_t nodePtr = m_nodes.size();
	m_nodes.push_back(Node(Node::NT_INVALID));
	{//remove regions that fully enclose this tile;
		m_nodes[nodePtr].internalLeaf().valueBegin = m_leafInfo.size();
		std::vector<uint32_t> enclosed;
		getEnclosing(newGrid.rect(), sortedRegions.begin(), sortedRegions.end(), std::back_insert_iterator< std::vector<uint32_t> >(enclosed));
		diffSortedContainer(sortedRegions, sortedRegions, enclosed);
		m_nodes[nodePtr].internalLeaf().enclosedCount = enclosed.size();
		m_leafInfo.insert(m_leafInfo.end(), enclosed.cbegin(), enclosed.cend());
	}

	if (refiner(maxBounds, m_regions, sortedRegions, newGrid)) {
		uint32_t childPtr = m_nodePtr.size();
		uint32_t gridPtr = m_nodeGrids.size();
		
		m_nodeGrids.push_back(newGrid);
		m_nodePtr.resize(m_nodePtr.size()+newGrid.tileCount(), NullNodePtr);
		
		m_nodes[nodePtr].internal().type = Node::NT_INTERNAL;
		m_nodes[nodePtr].internal().childrenBegin = childPtr;
		m_nodes[nodePtr].internal().gridPtr = gridPtr;
	
		for(uint32_t tile=0, ts=newGrid.tileCount(); tile < ts; ++tile) {
			std::vector<uint32_t> tmp;
			GeoGrid::GridBin gridBin(newGrid.select(tile));
			GeoRect tmpRect(newGrid.cellBoundary(gridBin));
			for(uint32_t rId : sortedRegions) {
				if (m_regions[rId]->intersects(tmpRect)) {
					tmp.push_back(rId);
				}
			}
			if (tmp.size()) {
				uint32_t tmpRet = insert(refiner, rPtr2Id, tmp, tmpRect);
				m_nodePtr[childPtr+gridBin.tile] = tmpRet;
			}
		}
	}
	else {
		Node & n = m_nodes[nodePtr];
		n.leaf().type = Node::NT_LEAF;
		n.leaf().intersectedCount = sortedRegions.size();
		m_leafInfo.insert(m_leafInfo.end(), sortedRegions.cbegin(), sortedRegions.cend());
	}
	return nodePtr;
}

template<typename T_OUTPUT_ITERATOR1, typename T_OUTPUT_ITERATOR2>
void GridRegionTree::find(const sserialize::spatial::GeoPoint & p, T_OUTPUT_ITERATOR1 intersecting, T_OUTPUT_ITERATOR2 enclosing) const {
	if (!m_nodes.size())
		return;
	uint32_t nodePtr = 0;
	while (nodePtr != NullNodePtr) {
		const Node & n = m_nodes[nodePtr];
		if (n.type() == Node::NT_INTERNAL) {
			const GeoGrid & grid = m_nodeGrids[n.internal().gridPtr];
			if (!grid.contains(p))
				return;
			if (n.internal().enclosedCount) {
				std::vector<uint32_t>::const_iterator it(m_leafInfo.cbegin()+n.internal().valueBegin);
				std::vector<uint32_t>::const_iterator end(it+n.internal().enclosedCount);
				for(; it != end; ++it) {
					*enclosing = m_regions[*it];
					++enclosing;
				}
			}
			nodePtr = m_nodePtr[n.internal().childrenBegin + grid.select(p).tile];
		}
		else if (n.type() == Node::NT_LEAF) {
			std::vector<uint32_t>::const_iterator it(m_leafInfo.cbegin()+n.leaf().valueBegin);
			std::vector<uint32_t>::const_iterator end(it+n.leaf().enclosedCount);
			for(; it != end; ++it) {
				*enclosing = m_regions[*it];
				++enclosing;
			}
			it = end;
			end =it + n.leaf().intersectedCount;
			for(; it != end; ++it) {
				if (m_regions[*it]->contains(p)) {
					*intersecting = m_regions[*it];
					++intersecting;
				}
			}
			return;
		}
		else {
			return;
		}
	}
}

}}//end namespace

#endif