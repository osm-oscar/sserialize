#include <sserialize/spatial/GridRegionTree.h>

const uint32_t sserialize::spatial::GridRegionTree::NullNodePtr = 0xFFFFFFFF;

namespace sserialize {
namespace spatial {

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
	out << "Storage usage: " << storageUsage << std::endl;
	out << "GridRegionTree::printstats--END" << std::endl;
}

}}//end namespace