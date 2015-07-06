#include <sserialize/Static/TriangulationGeoHierarchyArrangement.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {
namespace spatial {

constexpr uint32_t TriangulationGeoHierarchyArrangement::NullCellId;

TriangulationGeoHierarchyArrangement::TriangulationGeoHierarchyArrangement() {}

TriangulationGeoHierarchyArrangement::TriangulationGeoHierarchyArrangement(const sserialize::UByteArrayAdapter& d) :
m_cellCount(d.getUint32(sserialize::SerializationInfo<uint8_t>::length)),
m_grid(d+(sserialize::SerializationInfo<uint8_t>::length+sserialize::SerializationInfo<uint32_t>::length)),
m_faceIdToRefinedCellId(d+(sserialize::SerializationInfo<uint8_t>::length+sserialize::SerializationInfo<uint32_t>::length+m_grid.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_VERSION, d.at(0), "sserialize::Static::spatial::TriangulationRegionArrangement::TriangulationRegionArrangement");
}

TriangulationGeoHierarchyArrangement::~TriangulationGeoHierarchyArrangement() {}

UByteArrayAdapter::OffsetType TriangulationGeoHierarchyArrangement::getSizeInBytes() const {
	return sserialize::SerializationInfo<uint8_t>::length+sserialize::SerializationInfo<uint32_t>::length+
			m_grid.getSizeInBytes()+m_faceIdToRefinedCellId.getSizeInBytes();
}

uint32_t TriangulationGeoHierarchyArrangement::cellId(const TriangulationGeoHierarchyArrangement::Point& p) const {
	return cellId(p.lat(), p.lon());
}

uint32_t TriangulationGeoHierarchyArrangement::cellId(double lat, double lon) const {
	uint32_t faceId = m_grid.faceId(lat, lon);
	if (faceId == Triangulation::NullFace) {
		return NullCellId;
	}
	uint32_t tmp = m_faceIdToRefinedCellId.at(faceId);
	if (tmp >= m_cellCount) {
		return NullCellId;
	}
	return tmp;
}

}}}//end namespace
