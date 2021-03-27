#include <sserialize/Static/TriangulationRegionArrangement.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {
namespace spatial {

constexpr uint32_t TriangulationRegionArrangement::NullCellId;

TriangulationRegionArrangement::TriangulationRegionArrangement() {}

TriangulationRegionArrangement::TriangulationRegionArrangement(const sserialize::UByteArrayAdapter& d) :
m_grid(d+1),
m_baseCellIdToIndexPtr(d+(1+m_grid.getSizeInBytes())),
m_refinedToBaseCellId(d+(1+m_grid.getSizeInBytes()+m_baseCellIdToIndexPtr.getSizeInBytes())),
m_faceIdToRefinedCellId(d+(1+m_grid.getSizeInBytes()+m_baseCellIdToIndexPtr.getSizeInBytes()+m_refinedToBaseCellId.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_REGION_ARRANGEMENT_VERSION, d.at(0), "sserialize::Static::spatial::TriangulationRegionArrangement::TriangulationRegionArrangement");
}

TriangulationRegionArrangement::~TriangulationRegionArrangement() {}

UByteArrayAdapter::OffsetType TriangulationRegionArrangement::getSizeInBytes() const {
	return 1+m_grid.getSizeInBytes()+m_baseCellIdToIndexPtr.getSizeInBytes()+m_refinedToBaseCellId.getSizeInBytes()+m_faceIdToRefinedCellId.getSizeInBytes();
}

TriangulationRegionArrangement::cellid_type TriangulationRegionArrangement::baseCellId(cellid_type cellId) const {
	return m_refinedToBaseCellId.at(cellId);
}

TriangulationRegionArrangement::indexid_type TriangulationRegionArrangement::regionListPtr(cellid_type cellId) const {
	return m_baseCellIdToIndexPtr.at(baseCellId(cellId));
}

TriangulationRegionArrangement::cellid_type TriangulationRegionArrangement::cellId(const TriangulationRegionArrangement::Point& p) const {
	return cellId(p.lat(), p.lon());
}

TriangulationRegionArrangement::cellid_type TriangulationRegionArrangement::cellId(double lat, double lon) const {
	auto faceId = m_grid.faceId(lat, lon);
	if (faceId == Triangulation::NullFace) {
		return NullCellId;
	}
	return m_faceIdToRefinedCellId.at(faceId.ut());
}

}}}//end namespace
