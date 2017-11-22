#ifndef SSERIALIZE_SPATIAL_GRID_REGION_TREE_H
#define SSERIALIZE_SPATIAL_GRID_REGION_TREE_H
#include <sserialize/spatial/GeoRegion.h>
#include <sserialize/spatial/GeoGrid.h>
#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/spatial/GeoMultiPolygon.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/mt/ThreadPool.h>
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
	FixedSizeRefiner();
	FixedSizeRefiner(double minLatStep, double minLonStep, uint32_t latCount, uint32_t lonCount);
	~FixedSizeRefiner() {}
	bool operator()(const sserialize::spatial::GeoRect& maxBounds, const std::vector<sserialize::spatial::GeoRegion*>& rId2Ptr, const std::vector<uint32_t> & sortedRegions, sserialize::spatial::GeoGrid & newGrid) const;
};

}}//end namespace detail

///A 2D grid based tree allowing for fast point-in-region queries
///Pointers to regions need to be valid while using an instance of this class
class GridRegionTree {
private:
	typedef detail::GridRegionTree::Node Node;
	template<typename T_OUTPUT_ITERATOR>
	struct IteratorWrapper {
		const sserialize::spatial::GeoPoint & p;
		T_OUTPUT_ITERATOR & idsIt;
		const std::vector<sserialize::spatial::GeoRegion*> & m_regions;
		inline IteratorWrapper(const sserialize::spatial::GeoPoint& p, T_OUTPUT_ITERATOR & idsIt, const std::vector<sserialize::spatial::GeoRegion*> & regions) :
		p(p), idsIt(idsIt), m_regions(regions) {}
		inline IteratorWrapper & operator*() { return *this; }
		inline IteratorWrapper & operator++() { return *this; }
		inline IteratorWrapper & operator=(uint32_t regionId) {
			if (m_regions[regionId]->contains(p)) {
				*idsIt = regionId;
				++idsIt;
			}
			return *this;
		}
	};
public:
	static const uint32_t NullNodePtr;
	typedef detail::GridRegionTree::FixedSizeRefiner FixedSizeRefiner;
	template<typename T_GRID_REFINER = FixedSizeRefiner, typename T_GEOPOLYGON_TYPE = sserialize::spatial::GeoPolygon, typename T_MULTIGEOPOLYGON_TYPE = sserialize::spatial::GeoMultiPolygon>
	struct TypeTraits {
		typedef T_GRID_REFINER GridRefiner;
		typedef T_GEOPOLYGON_TYPE GeoPolygon;
		typedef T_MULTIGEOPOLYGON_TYPE GeoMultiPolygon;
	};
private:
	std::vector<Node> m_nodes;
	std::vector<GeoGrid> m_nodeGrids;
	std::vector<uint32_t> m_nodePtr;
	std::vector<uint32_t> m_leafInfo;
	std::vector<GeoRegion*> m_regions;
private:
	mutable std::atomic<uint64_t> m_cumulatedResultSetSize;
	mutable std::atomic<uint64_t> m_intersectTestCount;
	mutable std::mutex m_lck;
private:
	template<typename T_TYPES_CONTAINER, typename T_REGION_ID_ITERATOR, typename T_OUTPUT_ITERATOR>
	void getEnclosing(const GeoRect & bounds, T_REGION_ID_ITERATOR begin, const T_REGION_ID_ITERATOR & end, T_OUTPUT_ITERATOR out);
	
	template<typename T_TYPES_CONTAINER>
	void create(const GeoGrid & initial, typename T_TYPES_CONTAINER::GridRefiner refiner);
	
	template<typename T_TYPES_CONTAINER>
	uint32_t insert(const typename T_TYPES_CONTAINER::GridRefiner & refiner, const std::unordered_map<GeoRegion*, uint32_t> & rPtr2Id, std::vector<uint32_t> sortedRegions, const sserialize::spatial::GeoRect & maxBounds);
public:
	GridRegionTree();
	GridRegionTree(GridRegionTree && other);
	GridRegionTree(const GridRegionTree & other);
	///@param refiner: refines a given node: bool operator()(GeoRect maxBounds, std::unordered_set<uint32_t> regionids, GeoGrid & newGrid) const,
	///@return false if no further refinement should happen
	template<typename T_GEOREGION_ITERATOR, typename T_TYPE_TRAITS = TypeTraits<FixedSizeRefiner, sserialize::spatial::GeoPolygon, sserialize::spatial::GeoMultiPolygon> >
	GridRegionTree(const GeoGrid & initial, T_GEOREGION_ITERATOR begin, T_GEOREGION_ITERATOR end, T_TYPE_TRAITS /*traits*/, typename T_TYPE_TRAITS::GridRefiner refiner) :
	m_regions(begin, end),
	m_cumulatedResultSetSize(0),
	m_intersectTestCount(0)
	{
		create<T_TYPE_TRAITS>(initial, refiner);
	}
	template<typename T_TYPE_TRAITS = TypeTraits<FixedSizeRefiner, sserialize::spatial::GeoPolygon, sserialize::spatial::GeoMultiPolygon> >
	GridRegionTree(const GeoGrid & initial, const std::vector<GeoRegion*> & regions, T_TYPE_TRAITS /*traits*/, typename T_TYPE_TRAITS::GridRefiner refiner) :
	m_regions(regions),
	m_cumulatedResultSetSize(0),
	m_intersectTestCount(0)
	{
		create<T_TYPE_TRAITS>(initial, refiner);
	}
	virtual ~GridRegionTree() {}
	GridRegionTree & operator=(const GridRegionTree & other);
	GridRegionTree & operator=(GridRegionTree && other);
	
	void shrink_to_fit();
	GeoRegion * region(uint32_t id) { return m_regions.at(id); }
	const GeoRegion * region(uint32_t id) const { return m_regions.at(id); }
	template<typename T_OUTPUT_ITERATOR1, typename T_OUTPUT_ITERATOR2>
	void find(const sserialize::spatial::GeoPoint & p, T_OUTPUT_ITERATOR1 definiteEnclosing, T_OUTPUT_ITERATOR2 candidateEnclosing) const;
	template<typename T_OUTPUT_ITERATOR>
	void find(const sserialize::spatial::GeoPoint & p, T_OUTPUT_ITERATOR ids) const;
	void printStats(std::ostream & out) const;
	inline const GeoGrid & rootGrid() const { return m_nodeGrids.at(0); }
};

template<typename T_TYPES_CONTAINER, typename T_REGION_ID_ITERATOR, typename T_OUTPUT_ITERATOR>
void GridRegionTree::getEnclosing(const sserialize::spatial::GeoRect& bounds, T_REGION_ID_ITERATOR begin, const T_REGION_ID_ITERATOR& end, T_OUTPUT_ITERATOR out) {
	typedef typename T_TYPES_CONTAINER::GeoPolygon GeoPolygon;
	typedef typename T_TYPES_CONTAINER::GeoMultiPolygon GeoMultiPolygon;
	
	GeoPolygon boundsPoly(GeoPolygon::fromRect(bounds));
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

template<typename T_TYPES_CONTAINER>
void GridRegionTree::create(const GeoGrid & initial, typename T_TYPES_CONTAINER::GridRefiner refiner) {
	std::unordered_map<GeoRegion*, uint32_t> rPtr2IdH;
	rPtr2IdH.reserve(m_regions.size());
	for(uint32_t i = 0, s = (uint32_t) m_regions.size(); i < s; ++i) {
		rPtr2IdH[ m_regions[i] ] = i;
	}
	
	const uint32_t nodePtr = 0;
	const uint32_t childPtr = 0;
	const uint32_t gridPtr = 0;
	
	m_nodes.push_back(Node(Node::NT_INTERNAL));
	m_nodeGrids.push_back(initial);
	m_nodePtr.resize(initial.tileCount(), sserialize::spatial::GridRegionTree::NullNodePtr);
	
	m_nodes[nodePtr].internal().childrenBegin = childPtr;
	m_nodes[nodePtr].internal().gridPtr = gridPtr;

	sserialize::ProgressInfo pinfo;
	pinfo.begin(initial.tileCount(), "GridRegionTree::create");
	std::atomic<uint32_t> ti(0);
	uint32_t ts(initial.tileCount());
	std::atomic<uint32_t> tileCompletionCount(0);

	sserialize::ThreadPool::execute([&,this]() {
		while (true) {
			uint32_t tile = ti.fetch_add(1);
			
			if (tile >= ts) {
				return;
			}
			
			GeoGrid::GridBin gridBin(initial.select(tile));
			std::vector<uint32_t> tmp;
			GeoRect tmpRect(initial.cellBoundary(gridBin));
			for(uint32_t rIt(0), rEnd((uint32_t) m_regions.size()); rIt != rEnd; ++rIt) {
				if (m_regions[rIt]->intersects(tmpRect)) {
					tmp.push_back(rIt);
				}
			}
			if (tmp.size()) {
				//put this into a temporary as m_nodePtr will likely change during recursion and therefore the fetched adress on the left is wrong
				uint32_t tmpRet = insert<T_TYPES_CONTAINER>(refiner, rPtr2IdH, tmp, tmpRect);
	// 			#pragma omp critical
				{
					std::lock_guard<std::mutex> l(m_lck);
					m_nodePtr[childPtr+gridBin.tile] = tmpRet;
				}
			}
			++tileCompletionCount;
			pinfo(tileCompletionCount);
		}
	});
	SSERIALIZE_ASSERT(ti >= ts);
	pinfo.end();
}


template<typename T_TYPES_CONTAINER>
uint32_t GridRegionTree::insert(const typename T_TYPES_CONTAINER::GridRefiner & refiner, const std::unordered_map< sserialize::spatial::GeoRegion*, uint32_t >& rPtr2Id, std::vector< uint32_t > sortedRegions, const sserialize::spatial::GeoRect& maxBounds) {
	uint32_t nodePtr = NullNodePtr;
// 	#pragma omp critical
	{
		std::lock_guard<std::mutex> l(m_lck);
		nodePtr = (uint32_t) m_nodes.size();
		m_nodes.push_back(Node(Node::NT_INVALID));
	}
	//remove regions that fully enclose this tile;
	std::vector<uint32_t> enclosed;
	getEnclosing<T_TYPES_CONTAINER>(maxBounds, sortedRegions.begin(), sortedRegions.end(), std::back_insert_iterator< std::vector<uint32_t> >(enclosed));
	{
		decltype(sortedRegions) tmp;
		std::set_difference(sortedRegions.begin(), sortedRegions.end(), enclosed.begin(), enclosed.end(), std::back_inserter(tmp));
		sortedRegions = std::move(tmp);
	}
	GeoGrid newGrid;
	if (refiner(maxBounds, m_regions, sortedRegions, newGrid)) {
		uint32_t childPtr = NullNodePtr;
// 		#pragma omp critical
		{
			std::lock_guard<std::mutex> l(m_lck);
			childPtr = (uint32_t) m_nodePtr.size();
			Node & n = m_nodes[nodePtr];
			n.internal().type = Node::NT_INTERNAL;
			n.internalLeaf().valueBegin = m_leafInfo.size();
			n.internalLeaf().enclosedCount = enclosed.size();
			n.internal().childrenBegin = childPtr;
			n.internal().gridPtr = m_nodeGrids.size();
			m_nodeGrids.push_back(newGrid);
			m_nodePtr.resize(m_nodePtr.size()+newGrid.tileCount(), NullNodePtr);
			m_leafInfo.insert(m_leafInfo.end(), enclosed.cbegin(), enclosed.cend());
		}
	
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
				uint32_t tmpRet = insert<T_TYPES_CONTAINER>(refiner, rPtr2Id, tmp, tmpRect);
				#pragma omp critical
				m_nodePtr[childPtr+gridBin.tile] = tmpRet;
			}
		}
	}
	else {
// 		#pragma omp critical
		{
			std::lock_guard<std::mutex> l(m_lck);
			Node & n = m_nodes[nodePtr];
			n.leaf().type = Node::NT_LEAF;
			n.leaf().valueBegin = m_leafInfo.size();
			n.leaf().enclosedCount = enclosed.size();
			n.leaf().intersectedCount = sortedRegions.size();
			m_leafInfo.insert(m_leafInfo.end(), enclosed.cbegin(), enclosed.cend());
			m_leafInfo.insert(m_leafInfo.end(), sortedRegions.cbegin(), sortedRegions.cend());
		}
	}
	return nodePtr;
}

template<typename T_OUTPUT_ITERATOR1, typename T_OUTPUT_ITERATOR2>
void GridRegionTree::find(const sserialize::spatial::GeoPoint & p, T_OUTPUT_ITERATOR1 definiteEnclosing, T_OUTPUT_ITERATOR2 candidateEnclosing) const {
	if (!m_nodes.size() || !m_nodeGrids[0].contains(p))
		return;
	uint32_t nodePtr = 0;
	while (nodePtr != NullNodePtr) {
		const Node & n = m_nodes[nodePtr];
		if (n.type() == Node::NT_INTERNAL) {
			//if we're in this cell, then our test point intersects it
			//first do the check for enclosing polys since the grid associated with this cell may be smaller than the cell as defined by the parent of this npde
			//this may happen, if a larger polygon encloses this cell and has therefore been removed from the list of polys that need refinement,
			//which in turn might result in a reduced grid (depends on the used refiner)
			if (n.internal().enclosedCount) {
				std::vector<uint32_t>::const_iterator it(m_leafInfo.cbegin()+n.internal().valueBegin);
				std::vector<uint32_t>::const_iterator end(it+n.internal().enclosedCount);
				for(; it != end; ++it) {
					*definiteEnclosing = *it;
					++definiteEnclosing;
				}
				m_cumulatedResultSetSize += n.internal().enclosedCount;
			}
			const GeoGrid & grid = m_nodeGrids[n.internal().gridPtr];
			if (!grid.contains(p)) {
				return;
			}
			nodePtr = m_nodePtr[n.internal().childrenBegin + grid.select(p).tile];
		}
		else if (n.type() == Node::NT_LEAF) {
			std::vector<uint32_t>::const_iterator it(m_leafInfo.cbegin()+n.leaf().valueBegin);
			std::vector<uint32_t>::const_iterator end(it+n.leaf().enclosedCount);
			for(; it != end; ++it) {
				*definiteEnclosing = *it;
				++definiteEnclosing;
			}
			it = end;
			end =it + n.leaf().intersectedCount;
			for(; it != end; ++it) {
				*candidateEnclosing = *it;
				++candidateEnclosing;
			}
			m_cumulatedResultSetSize += n.leaf().intersectedCount + n.leaf().enclosedCount;
			m_intersectTestCount += n.leaf().intersectedCount;
			return;
		}
		else {
			return;
		}
	}
}

template<typename T_OUTPUT_ITERATOR>
void GridRegionTree::find(const sserialize::spatial::GeoPoint& p, T_OUTPUT_ITERATOR idsIt) const {
	find(p, idsIt, IteratorWrapper<T_OUTPUT_ITERATOR>(p, idsIt, m_regions));
}

}}//end namespace

#endif