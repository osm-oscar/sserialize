#include <sserialize/Static/TriangulationGridLocator.h>
#include <sserialize/utility/exceptions.h>

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>

namespace sserialize {
namespace Static {
namespace spatial {

TriangulationGridLocator::TriangulationGridLocator() {}

TriangulationGridLocator::TriangulationGridLocator(const UByteArrayAdapter& d) :
m_trs(d+1),
m_grid(d+(1+m_trs.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GRID_LOCATOR_VERSION, d.at(0), "sserialize::Static::spatial::TriangulationGridLocator::TriangulationGridLocator");
}

TriangulationGridLocator::~TriangulationGridLocator() {}

UByteArrayAdapter::OffsetType TriangulationGridLocator::getSizeInBytes() const {
	return 1+m_grid.getSizeInBytes()+m_trs.getSizeInBytes();
}

bool TriangulationGridLocator::gridContains(const TriangulationGridLocator::Point& p) const {
	return m_grid.contains(p);
}

bool TriangulationGridLocator::gridContains(double lat, double lon) const {
	return m_grid.contains(lat, lon);
}

bool TriangulationGridLocator::contains(const TriangulationGridLocator::Point& p) const {
	return contains(p.lat(), p.lon());
}

bool TriangulationGridLocator::contains(double lat, double lon) const {
	return faceId(lat, lon) != NullFace;
}

uint32_t TriangulationGridLocator::faceId(double lat, double lon) const {
	if (gridContains(lat, lon)) {
		uint32_t hint = m_grid.at(lat, lon);
		return m_trs.locate<CGAL::Exact_predicates_exact_constructions_kernel>(lat, lon, hint);
	}
	return NullFace;
}

uint32_t TriangulationGridLocator::faceId(const TriangulationGridLocator::Point& p) const {
	return faceId(p.lat(), p.lon());
}

TriangulationGridLocator::Face TriangulationGridLocator::face(const TriangulationGridLocator::Point& p) const {
	return face(p.lat(), p.lon());
}

TriangulationGridLocator::Face TriangulationGridLocator::face(double lat, double lon) const {
	uint32_t fId = faceId(lat, lon);
	if (fId == NullFace) {
		return Face();
	}
	return m_trs.face(fId);
}

}}}//end namespace