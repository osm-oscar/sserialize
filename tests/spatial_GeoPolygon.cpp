#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/spatial/GridRegionTree.h>
#include <iostream>
#include "datacreationfuncs.h"

using namespace sserialize;
using namespace sserialize::spatial;

int main() {

	SamplePolygonTestData m_data;
	spatial::GridRegionTree m_grt;
	std::unordered_map<spatial::GeoRegion*, uint32_t> m_grIdMap;
	createHandSamplePolygons(m_data);
	for (size_t i = 0; i < m_data.polys.size(); ++i) {
		m_grIdMap[&(m_data.polys[i].first)] =  m_data.polys[i].second;
	}
	std::vector<spatial::GeoRegion*> regions;
	regions.reserve(m_grIdMap.size());
	for(auto & x : m_grIdMap) {
		regions.push_back(x.first);
	}
	spatial::GeoRect initialRect = spatial::GeoShape::bounds(regions.cbegin(), regions.cend());
	spatial::detail::GridRegionTree::FixedSizeRefiner refiner(0.1, 0.1, 2, 2);
	
	for(spatial::GeoRegion * r : regions) {
		sserialize::spatial::GeoPolygon * gp = (sserialize::spatial::GeoPolygon*)r;
		r->asString(std::cout);
		GeoRect gpb = r->boundary();
		gpb.resize(0.9, 0.9);
		if (gp->encloses( sserialize::spatial::GeoPolygon::fromRect(gpb))) {
			std::cout << "bui" << std::endl;
		}
		else {
			std::cout << "hui" << std::endl;
		}
	}
	m_grt = spatial::GridRegionTree(spatial::GeoGrid(initialRect, 1, 1), regions.begin(), regions.end(), refiner);
	
	return 0;
}