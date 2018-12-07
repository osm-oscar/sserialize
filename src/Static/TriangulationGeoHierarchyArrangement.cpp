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
	if (faceId == Triangulation::NullFace) {
		return NullCellId;
	}
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


std::set<uint32_t> TriangulationGeoHierarchyArrangement::cellIds(double lat, double lon) const {
	auto face = m_grid.face(lat, lon);
	if (!face.valid()) {
		return std::set<uint32_t>();
	}
	
	std::set<uint32_t> result;
	if (face.isVertex(Point(lat, lon))) {
		auto vertex = face.vertex(Point(lat, lon));
		auto fit = vertex.facesBegin();
		auto fend = vertex.facesEnd();
		while(true) {
			uint32_t tmp = m_faceIdToRefinedCellId.at(fit.face().id());
			if (tmp < cellCount()) {
				result.insert(tmp);
			}
			if (fit == fend) {
				break;
			}
			else {
				++fit;
			}
		}
	}
	else {
		uint32_t tmp = m_faceIdToRefinedCellId.at(face.id());
		if (tmp < cellCount()) {
			result.insert(tmp);
		}
	}
	return result;
}

std::set<uint32_t> TriangulationGeoHierarchyArrangement::cellIds(const Point & p) const {
	return cellIds(p.lat(), p.lon());
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
	
	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef K::Point_2 Point_2;
	typedef K::Segment_2 Segment_2;

	Segment_2 se(start.convertTo<Point_2>(), end.convertTo<Point_2>());
	
	tds().explore(startFace, [&wct, &se, radius](const Triangulation::Face & f) {
		bool ok = false;
		{
			Segment_2 se01(f.point(0).convertTo<Point_2>(), f.point(1).convertTo<Point_2>());
			Segment_2 se02(f.point(0).convertTo<Point_2>(), f.point(2).convertTo<Point_2>());
			Segment_2 se12(f.point(1).convertTo<Point_2>(), f.point(2).convertTo<Point_2>());
			ok = CGAL::do_intersect(se, se01) || CGAL::do_intersect(se, se02) || CGAL::do_intersect(se, se12);
		}
		if (!ok && radius > 0.0) {
			sserialize::spatial::GeoPoint ct(f.centroid());
			double myDist = wct.dc(ct.lat(), ct.lon());
			myDist = std::fabs<double>(myDist);
			ok |= myDist < radius;
			
			for(int j(0); !ok && j < 3; ++j) {
				Triangulation::Point fp(f.point(j));
				double myDist = wct.dc(fp.lat(), fp.lon());
				myDist = std::fabs<double>(myDist);
				ok |= myDist < radius;
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
		uint32_t sourceHint = Triangulation::NullFace;
		for(const spatial::GeoPoint * prev(begin), * it(begin+1); it < end; ++it, ++prev) {
			if (sourceHint == Triangulation::NullFace) {
				sourceHint = m_grid.faceId(*prev);
			}
			if (sourceHint != Triangulation::NullFace) {
				sourceHint = tds().traverse(*it, *prev, [&wct](const Triangulation::Face & face) {
					SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(face.id(), Triangulation::NullFace)
					SSERIALIZE_CHEAP_ASSERT(face.valid())
					uint32_t cId = wct.parent->cellIdFromFaceId(face.id());
					if (cId != NullCellId) {
						wct.result.insert(cId);
					}
				}, sourceHint, sserialize::Static::spatial::Triangulation::TT_STRAIGHT);
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
				SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(f.id(), Triangulation::NullFace)
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
						ok = f.intersects(*it, *(it+1));
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

sserialize::ItemIndex
TriangulationGeoHierarchyArrangement::trianglesAlongPath(const TriangulationGeoHierarchyArrangement::Point* begin, const TriangulationGeoHierarchyArrangement::Point* end) const {
// 	typedef CGAL::Exact_predicates_inexact_constructions_kernel MyGeomTraits;
	std::vector<uint32_t> faceIds;
	uint32_t sourceHint = Triangulation::NullFace;
	for(const spatial::GeoPoint * prev(begin), * it(begin+1); it < end; ++it, ++prev) {
		if (sourceHint == Triangulation::NullFace) {
		sourceHint = m_grid.faceId(*prev);
		}
		if (sourceHint != Triangulation::NullFace) {
			faceIds.emplace_back(sourceHint);
			sourceHint = tds().traverse(*it, *prev, [&faceIds](const Triangulation::Face & face) {
				SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(face.id(), Triangulation::NullFace)
				SSERIALIZE_CHEAP_ASSERT(face.valid())
				faceIds.emplace_back(face.id());
			}, sourceHint, sserialize::Static::spatial::Triangulation::TT_STRAIGHT);
		}
	}
	std::sort(faceIds.begin(), faceIds.end());
	faceIds.resize(
		std::distance(
			faceIds.begin(),
			std::unique(faceIds.begin(), faceIds.end())
		)
	);
	faceIds.shrink_to_fit();
	return sserialize::ItemIndex(faceIds);
}

sserialize::ItemIndex
TriangulationGeoHierarchyArrangement::cellsAlongPath(double radius, const std::vector<Point>::const_iterator & begin, const std::vector<Point>::const_iterator & end) const {
	const Point * myBegin = &(*begin);
	const Point * myEnd = &(*end);
	return this->cellsAlongPath(radius, myBegin, myEnd);
}

sserialize::ItemIndex
TriangulationGeoHierarchyArrangement::cellsAlongPath(double radius, const std::vector<sserialize::spatial::GeoPoint>::iterator & begin, const std::vector<sserialize::spatial::GeoPoint>::iterator & end) const {
	const sserialize::spatial::GeoPoint * myBegin = &(*begin);
	const sserialize::spatial::GeoPoint * myEnd = &(*end);
	return this->cellsAlongPath(radius, myBegin, myEnd);
}

sserialize::ItemIndex
TriangulationGeoHierarchyArrangement::trianglesAlongPath(const std::vector<Point>::const_iterator & begin, const std::vector<Point>::const_iterator & end) const {
	const Point * myBegin = &(*begin);
	const Point * myEnd = &(*end);
	return this->trianglesAlongPath(myBegin, myEnd);
}

sserialize::ItemIndex
TriangulationGeoHierarchyArrangement::trianglesAlongPath(const std::vector<sserialize::spatial::GeoPoint>::iterator & begin, const std::vector<sserialize::spatial::GeoPoint>::iterator & end) const {
	const sserialize::spatial::GeoPoint * myBegin = &(*begin);
	const sserialize::spatial::GeoPoint * myEnd = &(*end);
	return this->trianglesAlongPath(myBegin, myEnd);
}


}}}//end namespace
