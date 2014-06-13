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
	m_nodes.shrink_to_fit();}


}}//end namespace