#include <sserialize/Static/TriangulationRegionArrangement.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {
namespace spatial {

constexpr uint32_t TriangulationRegionArrangement::NullCellId;

TriangulationRegionArrangement::TriangulationRegionArrangement() {}

TriangulationRegionArrangement::TriangulationRegionArrangement(const sserialize::UByteArrayAdapter& d, const sserialize::Static::ItemIndexStore&) :
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

uint32_t TriangulationRegionArrangement::baseCellId(uint32_t cellId) const {
	return m_refinedToBaseCellId.at(cellId);
}

ItemIndex TriangulationRegionArrangement::regions(uint32_t cellId) const {
	return m_idxStore.at(m_baseCellIdToIndexPtr.at(baseCellId(cellId)));
}

uint32_t TriangulationRegionArrangement::cellId(const TriangulationRegionArrangement::Point& p) const {
	return cellId(p.lat(), p.lon());
}

uint32_t TriangulationRegionArrangement::cellId(double lat, double lon) const {
	uint32_t faceId = m_grid.faceId(lat, lon);
	if (faceId == Triangulation::NullFace) {
		return NullCellId;
	}
	return m_faceIdToRefinedCellId.at(faceId);
}

}}}//end namespace
