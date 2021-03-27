#include <sserialize/Static/TriangulationGeoHierarchyArrangement.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/spatial/LatLonCalculations.h>
#include <sserialize/mt/ThreadPool.h>

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

TriangulationGeoHierarchyArrangement::cellid_type
TriangulationGeoHierarchyArrangement::cellIdFromFaceId(FaceId faceId) const {
	if (faceId == Triangulation::NullFace) {
		return NullCellId;
	}
	uint32_t tmp = m_faceIdToRefinedCellId.at(faceId.ut());
	if (tmp != cellCount()) {
		return tmp;
	}
	else {
		return NullCellId;
	}
}

TriangulationGeoHierarchyArrangement::Triangulation::FaceId
TriangulationGeoHierarchyArrangement::faceIdFromCellId(cellid_type cellId) const {
	if (cellId >= cellCount()) {
		return Triangulation::NullFace;
	}
	else {
		return FaceId(m_refinedCellIdToFaceId.at(cellId));
	}
}

TriangulationGeoHierarchyArrangement::CFGraph TriangulationGeoHierarchyArrangement::cfGraph(cellid_type cellId) const {
	if (cellId >= cellCount()) {
		return TriangulationGeoHierarchyArrangement::CFGraph(this, Triangulation::Face());
	}
	return TriangulationGeoHierarchyArrangement::CFGraph(this, tds().face(FaceId(m_refinedCellIdToFaceId.at(cellId))));
}

uint32_t TriangulationGeoHierarchyArrangement::cellId(double lat, double lon) const {
	auto faceId = m_grid.faceId(lat, lon);
	if (faceId == Triangulation::NullFace) {
		return NullCellId;
	}
	uint32_t tmp = m_faceIdToRefinedCellId.at(faceId.ut());
	if (tmp >= cellCount()) {
		return NullCellId;
	}
	return tmp;
}


std::set<TriangulationGeoHierarchyArrangement::cellid_type>
TriangulationGeoHierarchyArrangement::cellIds(double lat, double lon) const {
	auto face = m_grid.face(lat, lon);
	if (!face.valid()) {
		return std::set<cellid_type>();
	}
	
	std::set<cellid_type> result;
	auto where = face.where(Point(lat, lon));
	SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(where, face.CT_OUTSIDE);
	if (where != face.CT_INSIDE) {
		if (face.CT_ON_VERTEX_0 <= where && where <= face.CT_ON_VERTEX_2) {
			auto vertex = face.vertex(where-face.CT_ON_VERTEX_0);
			auto fit = vertex.facesBegin();
			auto fend = vertex.facesEnd();
			while(true) {
				auto cf = fit.face();
				if (cf.valid()) {
					cellid_type tmp = m_faceIdToRefinedCellId.at(cf.id().ut());
					if (tmp < cellCount()) {
						result.insert(tmp);
					}
				}
				if (fit == fend) {
					break;
				}
				else {
					++fit;
				}
			}
		}
		else { //on edge
			cellid_type tmp = m_faceIdToRefinedCellId.at(face.id().ut());
			if (tmp < cellCount()) {
				result.insert(tmp);
			}
			
			auto nFace = face.neighbor(where-face.CT_ON_EDGE_0);
			if (nFace.valid()) {
				tmp = m_faceIdToRefinedCellId.at(nFace.id().ut());
				if (tmp < cellCount()) {
					result.insert(tmp);
				}
			}
		}
	}
	else {
		cellid_type tmp = m_faceIdToRefinedCellId.at(face.id().ut());
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
	auto startFace = m_grid.faceId(start);
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
		std::unordered_set<cellid_type> result;
		const sserialize::spatial::GeoPoint * beginPts;
		const sserialize::spatial::GeoPoint * endPts;
		WorkContext(const sserialize::spatial::GeoPoint * beginPts, const sserialize::spatial::GeoPoint * endPts) :
		beginPts(beginPts), endPts(endPts) {}
	};
	WorkContext wct(begin, end);
	wct.parent = this;

	
	if (radius <= 0.0) {
		auto sourceHint = Triangulation::NullFace;
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
		auto startFace = Triangulation::NullFace;
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
					cellid_type cellId = wct.parent->cellIdFromFaceId(f.id());
					if (cellId != NullCellId) {
						wct.result.insert(cellId);
					}
				}
				return ok;
		});
	}
	std::vector<cellid_type> tmp(wct.result.begin(), wct.result.end());
	std::sort(tmp.begin(), tmp.end());
	return sserialize::ItemIndex(std::move(tmp));
}

sserialize::ItemIndex
TriangulationGeoHierarchyArrangement::trianglesAlongPath(const TriangulationGeoHierarchyArrangement::Point* begin, const TriangulationGeoHierarchyArrangement::Point* end) const {
// 	typedef CGAL::Exact_predicates_inexact_constructions_kernel MyGeomTraits;
	std::vector<sserialize::ItemIndex::value_type> faceIds;
	auto sourceHint = Triangulation::NullFace;
	for(const spatial::GeoPoint * prev(begin), * it(begin+1); it < end; ++it, ++prev) {
		if (sourceHint == Triangulation::NullFace) {
		sourceHint = m_grid.faceId(*prev);
		}
		if (sourceHint != Triangulation::NullFace) {
			faceIds.emplace_back(static_cast<sserialize::ItemIndex::value_type>(sourceHint.ut()));
			sourceHint = tds().traverse(*it, *prev, [&faceIds](const Triangulation::Face & face) {
				SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(face.id(), Triangulation::NullFace)
				SSERIALIZE_CHEAP_ASSERT(face.valid())
				faceIds.emplace_back(static_cast<sserialize::ItemIndex::value_type>(face.id().ut()));
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
	//BUG: potential overflow
	return sserialize::ItemIndex(faceIds.begin(), faceIds.end());
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

void
TriangulationGeoHierarchyArrangement::statsSummary(std::ostream & out) const {
	struct State {
		std::mutex lock;
		sserialize::MinMaxMean<double> area;
		const TriangulationGeoHierarchyArrangement * that;
	};
	struct Worker {
		Worker(State * state) : state(state) {}
		Worker(Worker const & other) : state(other.state) {}
		~Worker() {
			std::lock_guard<std::mutex> lck(state->lock);
			state->area.update(area);
		}
		State * state;
		sserialize::MinMaxMean<double> area;
		void operator()() {
			for(cellid_type i(0), s(state->that->cellCount()); i < s; ++i) {
				area.update(state->that->cfGraph(i).area());
			}
		}
	};
	State state;
	state.that = this;
	sserialize::ThreadPool::execute(Worker(&state), 0, sserialize::ThreadPool::CopyTaskTag());
	
	out << "# cells: " << cellCount() << '\n';
	out << "min cell area km^2: " << state.area.min()/(1000*1000) << '\n'; 
	out << "mean cell area km^2: " << state.area.mean()/(1000*1000) << '\n'; 
	out << "max cell area km^2: " << state.area.max()/(1000*1000) << '\n'; 
	out << std::flush;
}

void
TriangulationGeoHierarchyArrangement::stats(std::ostream & out) const {
	out << "cell id;area [m^2];number of triangles\n"; 
	for(uint32_t i(1), s(cellCount()); i < s; ++i) {
		double cellArea = 0.0;
		uint32_t triangCount = 0;
		this->cfGraph(i).visitCB([&cellArea, &triangCount](Triangulation::Face const & face) {
			cellArea += face.area();
			++triangCount;
		});
		out << i << ';' << cellArea << ';' << triangCount << '\n';
	}
	out << std::flush;
}

}}}//end namespace
