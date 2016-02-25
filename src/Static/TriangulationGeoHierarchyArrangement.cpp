#include <sserialize/Static/TriangulationGeoHierarchyArrangement.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/spatial/LatLonCalculations.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

namespace sserialize {
namespace Static {
namespace spatial {

constexpr uint32_t TriangulationGeoHierarchyArrangement::NullCellId;

TriangulationGeoHierarchyArrangement::TriangulationGeoHierarchyArrangement() {}

TriangulationGeoHierarchyArrangement::TriangulationGeoHierarchyArrangement(const sserialize::UByteArrayAdapter& d) :
m_grid(d+(sserialize::SerializationInfo<uint8_t>::length)),
m_faceIdToRefinedCellId(d+(sserialize::SerializationInfo<uint8_t>::length+m_grid.getSizeInBytes())),
m_refinedCellIdToFaceId(d+(sserialize::SerializationInfo<uint8_t>::length+m_grid.getSizeInBytes()+m_faceIdToRefinedCellId.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GEO_HIERARCHY_ARRANGEMENT_VERSION, d.at(0), "sserialize::Static::spatial::TriangulationRegionArrangement::TriangulationRegionArrangement");
}

TriangulationGeoHierarchyArrangement::~TriangulationGeoHierarchyArrangement() {}

UByteArrayAdapter::OffsetType TriangulationGeoHierarchyArrangement::getSizeInBytes() const {
	return sserialize::SerializationInfo<uint8_t>::length+m_grid.getSizeInBytes()+
			m_faceIdToRefinedCellId.getSizeInBytes()+m_refinedCellIdToFaceId.getSizeInBytes();
}

uint32_t TriangulationGeoHierarchyArrangement::cellId(const TriangulationGeoHierarchyArrangement::Point& p) const {
	return cellId(p.lat(), p.lon());
}

uint32_t TriangulationGeoHierarchyArrangement::cellIdFromFaceId(uint32_t faceId) const {
	uint32_t tmp = m_faceIdToRefinedCellId.at(faceId);
	if (tmp != cellCount()) {
		return tmp;
	}
	else {
		return NullCellId;
	}
}

uint32_t TriangulationGeoHierarchyArrangement::faceIdFromCellId(uint32_t cellId) const {
	if (cellId >= cellCount()) {
		return Triangulation::NullFace;
	}
	else {
		return m_refinedCellIdToFaceId.at(cellId);
	}
}

TriangulationGeoHierarchyArrangement::CFGraph TriangulationGeoHierarchyArrangement::cfGraph(uint32_t cellId) const {
	if (cellId >= cellCount()) {
		return TriangulationGeoHierarchyArrangement::CFGraph(this, Triangulation::Face());
	}
	return TriangulationGeoHierarchyArrangement::CFGraph(this, tds().face(m_refinedCellIdToFaceId.at(cellId)));
}

uint32_t TriangulationGeoHierarchyArrangement::cellId(double lat, double lon) const {
	uint32_t faceId = m_grid.faceId(lat, lon);
	if (faceId == Triangulation::NullFace) {
		return NullCellId;
	}
	uint32_t tmp = m_faceIdToRefinedCellId.at(faceId);
	if (tmp >= cellCount()) {
		return NullCellId;
	}
	return tmp;
}

sserialize::ItemIndex
TriangulationGeoHierarchyArrangement::
cellsBetween(const sserialize::spatial::GeoPoint& start, const sserialize::spatial::GeoPoint& end, double radius) const {
	uint32_t startFace = m_grid.faceId(start);
	if (startFace == Triangulation::NullFace) {
		return sserialize::ItemIndex();
	}
	
	struct WorkContext {
		sserialize::spatial::CrossTrackDistanceCalculator dc;
		const TriangulationGeoHierarchyArrangement * parent;
		std::unordered_set<uint32_t> result;
		WorkContext(const sserialize::spatial::GeoPoint& start, const sserialize::spatial::GeoPoint& end) :
		dc(start.lat(), start.lon(), end.lat(), end.lon()) {}
	};
	WorkContext wct(start, end);
	wct.parent = this;
	
	tds().explore(startFace, [&wct, radius](const Triangulation::Face & f) {
		bool ok = false;
		{
			sserialize::spatial::GeoPoint ct(f.centroid());
			double myDist = wct.dc(ct.lat(), ct.lon());
			myDist = std::fabs<double>(myDist);
			ok = myDist < radius;
		}
		for(int j(0); !ok && j < 3; ++j) {
			Triangulation::Point fp(f.point(j));
			double myDist = wct.dc(fp.lat(), fp.lon());
			myDist = std::fabs<double>(myDist);
			ok = myDist < radius;
		}
		if (ok) {
			uint32_t cellId = wct.parent->cellIdFromFaceId(f.id());
			if (cellId != NullCellId) {
				wct.result.insert(cellId);
			}
		}
		return ok;
	});
	std::vector<uint32_t> tmp(wct.result.begin(), wct.result.end());
	std::sort(tmp.begin(), tmp.end());
	return sserialize::ItemIndex(std::move(tmp));
}


sserialize::ItemIndex
TriangulationGeoHierarchyArrangement::cellsAlongPath(double radius, const spatial::GeoPoint* begin, const spatial::GeoPoint* end) const {

	struct WorkContext {
		const TriangulationGeoHierarchyArrangement * parent;
		std::unordered_set<uint32_t> result;
		const sserialize::spatial::GeoPoint * beginPts;
		const sserialize::spatial::GeoPoint * endPts;
		WorkContext(const sserialize::spatial::GeoPoint * beginPts, const sserialize::spatial::GeoPoint * endPts) :
		beginPts(beginPts), endPts(endPts) {}
	};
	WorkContext wct(begin, end);
	wct.parent = this;

	
	if (radius <= 0.0) {
		typedef CGAL::Exact_predicates_inexact_constructions_kernel MyGeomTraits;
		uint32_t startFace = Triangulation::NullFace;
		for(const spatial::GeoPoint * prev(begin), * it(begin+1); it < end; ++it, ++prev) {
			if (startFace == Triangulation::NullFace) {
				startFace = m_grid.faceId(*prev);
			}
			if (startFace != Triangulation::NullFace) {
				startFace = tds().traverse(it->lat(), it-> lon(), startFace, [&wct](const Triangulation::Face & face) {
					wct.result.insert( wct.parent->cellIdFromFaceId(face.id()) );
				}, MyGeomTraits());
			}
		}
	}
	else {
		uint32_t startFace = Triangulation::NullFace;
		for(const spatial::GeoPoint * it(begin); startFace == Triangulation::NullFace && it != end; ++it) {
			startFace = m_grid.faceId(*it);
		}

		if (startFace == Triangulation::NullFace) {
			return sserialize::ItemIndex();
		}

		tds().explore(startFace, [&wct, radius](const Triangulation::Face & f) {
				sserialize::spatial::GeoPoint ct(f.centroid());
				bool ok = false;
				typedef const sserialize::spatial::GeoPoint* MyIt;
				for(MyIt it(wct.beginPts), end(wct.endPts-1); !ok && it != end; ++it) {
					double myDist = sserialize::spatial::distance(it->lat(), it->lon(), (it+1)->lat(), (it+1)->lon(), ct.lat(), ct.lon());
					myDist = std::fabs<double>(myDist);
					ok = myDist < radius;
					for(int j(0); !ok && j < 3; ++j) {
						Triangulation::Point gp(f.point(j));
						myDist = sserialize::spatial::distance(it->lat(), it->lon(), (it+1)->lat(), (it+1)->lon(), gp.lat(), gp.lon());
						myDist = std::fabs<double>(myDist);
						ok = myDist < radius;
					}
					if (!ok) {
						ok = f.contains(*it); //TODO: intersection needs to be tested aswell
					}
				}
				if (ok) {
					uint32_t cellId = wct.parent->cellIdFromFaceId(f.id());
					if (cellId != NullCellId) {
						wct.result.insert(cellId);
					}
				}
				return ok;
		});
	}
	std::vector<uint32_t> tmp(wct.result.begin(), wct.result.end());
	std::sort(tmp.begin(), tmp.end());
	return sserialize::ItemIndex(std::move(tmp));
}


}}}//end namespace
