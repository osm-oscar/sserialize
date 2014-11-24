#include <sserialize/spatial/GridRegionTree.h>
#include <sserialize/utility/printers.h>

const uint32_t sserialize::spatial::GridRegionTree::NullNodePtr = 0xFFFFFFFF;

namespace sserialize {
namespace spatial {

detail::GridRegionTree::FixedSizeRefiner::FixedSizeRefiner() :
m_minLatStep(1.0),
m_minLonStep(1.0),
m_latCount(2),
m_lonCout(2)
{}

detail::GridRegionTree::FixedSizeRefiner::FixedSizeRefiner(double minLatStep, double minLonStep, uint32_t latCount, uint32_t lonCount) :
m_minLatStep(minLatStep),
m_minLonStep(minLonStep),
m_latCount(latCount),
m_lonCout(lonCount)
{}


bool detail::GridRegionTree::FixedSizeRefiner::operator()(const sserialize::spatial::GeoRect& maxBounds, const std::vector< sserialize::spatial::GeoRegion* >& rId2Ptr, const std::vector< uint32_t >& sortedRegions, sserialize::spatial::GeoGrid& newGrid) const {
	if (maxBounds.maxLon()-maxBounds.minLon() > m_minLonStep && maxBounds.maxLat()-maxBounds.minLat() > m_minLatStep && sortedRegions.size()) {
		std::vector< uint32_t >::const_iterator it(sortedRegions.cbegin());
		GeoRect myBounds(rId2Ptr[*it]->boundary());
		for(std::vector<uint32_t>::const_iterator end(sortedRegions.cend()); it != end; ++it) {
			myBounds.enlarge(rId2Ptr[*it]->boundary());
		}
		newGrid = GeoGrid( myBounds / maxBounds, m_latCount, m_lonCout);
		return true;
	}
	return false;
}

GridRegionTree::GridRegionTree() :
m_cumulatedResultSetSize(0),
m_intersectTestCount(0)
{}

GridRegionTree::GridRegionTree(GridRegionTree && other) {
	m_nodes.swap(other.m_nodes);
	m_nodeGrids.swap(other.m_nodeGrids);
	m_nodePtr.swap(other.m_nodePtr);
	m_leafInfo.swap(other.m_leafInfo);
	m_regions.swap(other.m_regions);
	m_cumulatedResultSetSize.store(other.m_cumulatedResultSetSize.load());
	m_intersectTestCount.store(other.m_intersectTestCount.load());
}

GridRegionTree::GridRegionTree(const GridRegionTree & other) :
m_nodes(other.m_nodes),
m_nodeGrids(other.m_nodeGrids),
m_nodePtr(other.m_nodePtr),
m_leafInfo(other.m_leafInfo),
m_regions(other.m_regions)
{
	m_cumulatedResultSetSize.store(other.m_cumulatedResultSetSize.load());
	m_intersectTestCount.store(other.m_intersectTestCount.load());
}

GridRegionTree & GridRegionTree::operator=(const GridRegionTree & other) {
	m_nodes = other.m_nodes;
	m_nodeGrids = other.m_nodeGrids;
	m_nodePtr = other.m_nodePtr;
	m_leafInfo = other.m_leafInfo;
	m_regions = other.m_regions;
	m_cumulatedResultSetSize.store(other.m_cumulatedResultSetSize.load());
	m_intersectTestCount.store(other.m_intersectTestCount.load());
	return *this;
}
GridRegionTree & GridRegionTree::operator=(GridRegionTree & other) {
	m_nodes.swap(other.m_nodes);
	m_nodeGrids.swap(other.m_nodeGrids);
	m_nodePtr.swap(other.m_nodePtr);
	m_leafInfo.swap(other.m_leafInfo);
	m_regions.swap(other.m_regions);
	m_cumulatedResultSetSize.store(other.m_cumulatedResultSetSize.load());
	m_intersectTestCount.store(other.m_intersectTestCount.load());
	return *this;
}

void GridRegionTree::shrink_to_fit() {
	m_leafInfo.shrink_to_fit();
	m_nodeGrids.shrink_to_fit();
	m_nodePtr.shrink_to_fit();
	m_nodes.shrink_to_fit();
}

void GridRegionTree::printStats(std::ostream & out) const {
	out << "GridRegionTree::printstats--BEGIN" << std::endl;
	out << "Nodes: " << m_nodes.size() << std::endl;
	out << "Grids: " << m_nodeGrids.size() << std::endl;
	uint32_t nullNodeCount = 0;
	for(const uint32_t nptr : m_nodePtr) {
		nullNodeCount += (nptr == NullNodePtr ? 1 : 0);
	}
	out << "ChildPtrs: " << m_nodePtr.size() << " of which " << (double)nullNodeCount/m_nodePtr.size()*100 << "% are NULL" << std::endl;
	uint64_t enclosedCount = 0;
	for(const Node & n : m_nodes) {
		enclosedCount += n.internalLeaf().enclosedCount;
	}
	out << "LeafInfo: " << m_leafInfo.size() << " of which " << (double)enclosedCount/m_leafInfo.size()*100 << "% are enclosed" << std::endl;
	out << "Regions: " << m_regions.size() << std::endl;

	uint64_t storageUsage = 0; 
	storageUsage += m_nodes.capacity() * sizeof(Node);
	storageUsage += m_nodeGrids.capacity() * sizeof(GeoGrid);
	storageUsage += m_leafInfo.capacity() * sizeof(uint32_t);
	storageUsage += m_regions.capacity() * sizeof(sserialize::spatial::GeoRegion*);

	uint64_t storageUsageSize = 0;
	storageUsageSize += m_nodes.size() * sizeof(Node);
	storageUsageSize += m_nodeGrids.size() * sizeof(GeoGrid);
	storageUsageSize += m_leafInfo.size() * sizeof(uint32_t);
	storageUsageSize += m_regions.size() * sizeof(sserialize::spatial::GeoRegion*);

	out << "Real Storage usage: " << sserialize::prettyFormatSize(storageUsage) << std::endl;
	out << "Storage usage: " << storageUsageSize << std::endl;
	out << "Containment tests: " << m_cumulatedResultSetSize.load() << " of which ";
	out << (double) m_intersectTestCount.load()/m_cumulatedResultSetSize.load() * 100;
	out << "% were intersection tests" << std::endl;
	out << "GridRegionTree::printstats--END" << std::endl;
}

}}//end namespace