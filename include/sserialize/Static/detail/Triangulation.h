#ifndef SSERIALIZE_STATIC_SPATIAL_DETAIL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_DETAIL_TRIANGULATION_H
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/Static/PointOnS2.h>
#include <sserialize/spatial/LatLonCalculations.h>

#include <queue>

#include <CGAL/number_utils.h>
#include <CGAL/enum.h>
#include <CGAL/intersections.h>
#include <CGAL/Constrained_triangulation_2.h>

namespace sserialize {
namespace Static {
namespace spatial {

class Triangulation;

namespace detail {
namespace Triangulation {

//BEGIN stuff for snapping

template<typename _Tp, typename _Sequence = std::vector<_Tp>,
	typename _Compare  = std::less<typename _Sequence::value_type> >
class MyPrioQueue: public std::priority_queue<_Tp, _Sequence, _Compare> {
public:
	typedef std::priority_queue<_Tp, _Sequence, _Compare> MyBaseClass;
public:
	MyPrioQueue() {}
	MyPrioQueue(MyBaseClass && base) : MyBaseClass(std::move(base)) {}
	MyPrioQueue(const MyBaseClass & base) : MyBaseClass(base) {}
	const _Sequence & container() const { return MyBaseClass::c; }
	using MyBaseClass::emplace;
	using MyBaseClass::top;
	using MyBaseClass::pop;
};

template<typename T_POINT>
struct IntPoint {
	typedef T_POINT Point;

	uint32_t lat;
	uint32_t lon;
	
	IntPoint() : lat(0xFFFFFFFF), lon(0xFFFFFFFF) {}
	
	IntPoint(uint32_t lat, uint32_t lon) : lat(lat), lon(lon) {}
	
	IntPoint(const IntPoint & other) :
	lat(other.lat),
	lon(other.lon)
	{}
	
	IntPoint(const Point & p) :
	lat(sserialize::spatial::GeoPoint::toIntLat(CGAL::to_double(p.x()))),
	lon(sserialize::spatial::GeoPoint::toIntLon(CGAL::to_double(p.y())))
	{}
	
	IntPoint(const GeoPoint & p)  :
	lat(sserialize::spatial::GeoPoint::toIntLat(p.lat())),
	lon(sserialize::spatial::GeoPoint::toIntLon(p.lon()))
	{}
	
	IntPoint(uint64_t v) : lat(v >> 32), lon(v & 0xFFFFFFFF) {}
	
	GeoPoint toGeoPoint() const {
		double dlat = sserialize::spatial::GeoPoint::toDoubleLat(lat);
		double dlon = sserialize::spatial::GeoPoint::toDoubleLon(lon);
		return sserialize::spatial::GeoPoint(dlat, dlon);
	}
	
	Point toPoint() const {
		double dlat = sserialize::spatial::GeoPoint::toDoubleLat(lat);
		double dlon = sserialize::spatial::GeoPoint::toDoubleLon(lon);
		return Point(dlat, dlon);
	}
	
	uint64_t toU64() const { return (static_cast<uint64_t>(lat) << 32) | lon; }
	
	static bool changes(const Point & p) {
		Point tmp(IntPoint(p).toPoint());
		return tmp.x() != p.x() || tmp.y() != p.y();
	}
	
	bool operator==(const IntPoint & other) const {
		return lat == other.lat && lon == other.lon;
	}
	
	bool operator!=(const IntPoint & other) const {
		return lat != other.lat || lon != other.lon;
	}
	bool operator<(const IntPoint & other) const {
		return (lat == other.lat ? lon < other.lon : lat < other.lat);
	}
};

template<typename T_POINT>
struct ConstrainedEdge {
	typedef T_POINT Point;
	typedef sserialize::Static::spatial::detail::Triangulation::IntPoint<Point> IntPoint;
	IntPoint p1;
	IntPoint p2;
	double length;
	ConstrainedEdge() : length(-1) {}
	ConstrainedEdge(const ConstrainedEdge & other) : p1(other.p1), p2(other.p2), length(other.length) {}
	ConstrainedEdge(const IntPoint & p1, const IntPoint & p2) : p1(p1), p2(p2) {
		double l1 = (double)p1.lat - (double) p2.lat;
		double l2 = (double)p1.lon - (double) p2.lon;
		length = l1*l1 + l2*l2;
	}
	ConstrainedEdge(const Point & p1, const Point & p2) : ConstrainedEdge(IntPoint(p1), IntPoint(p2)) {}
	bool valid() const {
		return p1 != p2;
	}
	bool operator==(const ConstrainedEdge & other) const {
		return p1 == other.p1 && p2 == other.p2;
	}
	//this way we get a max heap for the prio queue in prepare
	bool operator<(const ConstrainedEdge & other) const {
		if (length == other.length) {
			return (p1 == other.p1 ? p2 < other.p2 : p1 < other.p1);
		}
		else {
			return length < other.length;
		}
	}
};

template<typename T_CTD, typename T_CECONTAINER>
struct CEBackInsertIterator {
	typedef T_CTD CTD;
	typedef typename CTD::Point Point;
	typedef typename CTD::Edge Edge;
	typedef typename CTD::Vertex_handle Vertex_handle;
	typedef T_CECONTAINER CEContainer;
	typedef sserialize::Static::spatial::detail::Triangulation::IntPoint<Point> IntPoint;
	CTD & ctd;
	CEContainer & dest;
	CEBackInsertIterator(CTD & ctd, CEContainer & dest) : ctd(ctd), dest(dest) {}
	CEBackInsertIterator & operator++() { return *this; }
	CEBackInsertIterator & operator++(int) { return *this; }
	CEBackInsertIterator & operator*() { return *this; }
	CEBackInsertIterator & operator=(const Edge & e) {
		Vertex_handle v1 = e.first->vertex(CTD::cw(e.second));
		Vertex_handle v2 = e.first->vertex(CTD::ccw(e.second));
		IntPoint p1(v1->point()), p2(v2->point());
		if (p1 != p2) {
			dest.emplace(p1, p2);
		}
		return *this;
	}
	
};

struct PrintRemovedEdges {
	void operator()(const sserialize::spatial::GeoPoint & gp1, const sserialize::spatial::GeoPoint & gp2) const {
		std::cout << "Could not add edge " << gp1 << " <-> " << gp2 << " with a length of ";
		std::cout << std::abs<double>( sserialize::spatial::distanceTo(gp1.lat(), gp1.lon(), gp2.lat(), gp2.lon()) ) << '\n';
	}
};

///ic: operator()(T_TDS::Edge) -> bool calls for every intersected constraint
///iff returnv value is false, exit function, otherwise continue intersecting
template<typename T_TDS, typename T_INTERSECTED_CONSTRAINTS>
void intersects(T_TDS & tds, typename T_TDS::Vertex_handle sv, typename T_TDS::Vertex_handle tv, T_INTERSECTED_CONSTRAINTS ic) {
	typedef T_TDS TDS;
	typedef typename TDS::Geom_traits Geom_traits;
	typedef typename TDS::Face_circulator Face_circulator;
	typedef typename TDS::Edge Edge;
	typedef typename Geom_traits::Orientation_2 Orientation_2;
	typedef typename Geom_traits::Collinear_are_ordered_along_line_2 Collinear_are_ordered_along_line_2;

	
	Orientation_2 ot( tds.geom_traits().orientation_2_object() );
	Collinear_are_ordered_along_line_2 oal( tds.geom_traits().collinear_are_ordered_along_line_2_object() );
	
	auto tp = tv->point();
	
	while (true) {
		//we first have to do a circle step to determine the face our line intersects
		//after that only face tests follow
		if (sv == tv) {
			return;
		}
		SSERIALIZE_CHEAP_ASSERT(sv->point() != tv->point());

		auto sp = sv->point();
		
		bool faceStep = true;
		Edge enteringEdge;
		
		//first the circle step
		{
			Face_circulator fc(tds.incident_faces(sv));
			Face_circulator fcEnd(fc);
			do {
				SSERIALIZE_CHEAP_ASSERT(!tds.is_infinite(fc));
				
				int svIdx = fc->index(sv);
				auto lv = fc->vertex(TDS::cw(svIdx));
				auto rv = fc->vertex(TDS::ccw(svIdx));
				auto lp = lv->point();
				auto rp = rv->point();
				
				auto lvOT = ot(sp, tp, lp);
				auto rvOT = ot(sp, tp, rp);
				
				//intersects edge lv->rv, remember that tv cannot be within the face
				if (lvOT == CGAL::Orientation::LEFT_TURN && rvOT == CGAL::Orientation::RIGHT_TURN) {
					if (fc->is_constrained(svIdx)) {
						if (!ic(Edge(fc, svIdx))) {
							return;
						}
					}
					faceStep = true;
					enteringEdge = tds.mirror_edge(Edge(fc, svIdx));
					break;
				}
				else if (lvOT == CGAL::Orientation::COLLINEAR) {
					if (oal(sp, lp, tp)) { //make sure that we don't jump back
						faceStep = false;
						sv = lv;
						break;
					}
				}
				else if (rvOT == CGAL::Orientation::COLLINEAR) {
					if (oal(sp, rp, tp)) { //make sure that we don't jump back
						faceStep = false;
						sv = rv;
						break;
					}
				}
				++fc;
			} while (fc != fcEnd);
		}
		
		//and now the face steps
		//here we have to check which edge of the current face our line sv->tv intersects
		while (faceStep) {
			auto ov = enteringEdge.first->vertex(enteringEdge.second);
			auto op = ov->point();

			//check through which edge we leave, since we already know that we came through edge enteringEdge
			//it's enough to check if ov is left, right, on -sv-tv-
			//remember that orientations are with respect to -sv-tv-
			
			auto oOT = ot(sp, tp, op);
			if (oOT == CGAL::Orientation::LEFT_TURN) {
				//leave through the edge left of op
				//this is the edge whose opposive vertex is ccw of ov
				enteringEdge.second = TDS::ccw(enteringEdge.second);
			}
			else if (oOT == CGAL::Orientation::RIGHT_TURN) {
				//leave through the edge right of op
				//this is the edge whose opposive vertex is cw of ov
				enteringEdge.second = TDS::cw(enteringEdge.second);
			}
			else if (oOT == CGAL::Orientation::COLLINEAR) {
				//opposite vertex is on the line -sv-tv-
				sv = ov;
				break;
			}
			else {
				SSERIALIZE_CHEAP_ASSERT(false);
			}
			
			//enteringEdge should now be the one through which we leave this face
			if (tds.is_constrained(enteringEdge)) {
				if (!ic(enteringEdge)) {
					return;
				}
			}
			enteringEdge = tds.mirror_edge(enteringEdge);
		}
	}
}

///@return true if the line v1->v2 intersects another constraint edge
template<typename T_TDS>
bool intersects(T_TDS & tds, typename T_TDS::Vertex_handle sv, typename T_TDS::Vertex_handle tv) {
	bool doesIntersect = false;
	intersects(tds, sv, tv, [&doesIntersect](const typename T_TDS::Edge &) -> bool {
		doesIntersect = true;
		return false;
	});
	return doesIntersect;
}


///@return true if the line v1->v2 intersects another constraint edge
template<typename T_CTD, typename T_OUTPUT_ITERATOR>
void intersection_points(T_CTD & ctd, typename T_CTD::Vertex_handle sv, typename T_CTD::Vertex_handle tv, T_OUTPUT_ITERATOR out) {
	typedef T_CTD CTD;
	typedef typename CTD::Edge Edge;
	typedef typename CTD::Point Point;
	typedef typename CTD::Intersection_tag Intersection_tag;
	intersects(ctd, sv, tv, [&ctd, &sv, &tv, &out](const Edge & e) -> bool {
		Point xP;
		const Point & pc = e.first->vertex(CTD::ccw(e.second))->point();
		const Point & pd = e.first->vertex(CTD::cw(e.second))->point();
		const Point & p1 = sv->point();
		const Point & p2 = tv->point();
		Intersection_tag itag = Intersection_tag();
		CGAL::intersection(ctd.geom_traits(), p1, p2, pc, pd, xP, itag);
		*out  = xP;
		++out;
		return true;
	});
}

///This will handle points created by intersections of constrained edges
///This makes all points representable! Beware that this very likely changes the triangulation (removing faces and adding new ones)
///You should therefore snap points before creating the triangulation
///@return number of changed points
template<typename T_CTD, typename T_REMOVED_EDGES>
uint32_t snap_vertices(T_CTD & ctd, T_REMOVED_EDGES re, double minEdgeLength) {
	typedef T_CTD TDS;
	typedef typename TDS::Face_handle Face_handle;
	typedef typename TDS::Vertex_handle Vertex_handle;
	typedef typename TDS::Finite_vertices_iterator Finite_vertices_iterator;
	typedef typename TDS::Point Point;
	typedef typename TDS::Edge Edge;
	typedef typename TDS::Locate_type Locate_type;
	typedef typename TDS::Intersection_tag Intersection_tag;
	
	//internal typedefs
	typedef detail::Triangulation::IntPoint<Point> IntPoint;
	typedef detail::Triangulation::ConstrainedEdge<Point> ConstrainedEdge;
	typedef detail::Triangulation::MyPrioQueue<ConstrainedEdge> CEContainer;
	typedef detail::Triangulation::CEBackInsertIterator<TDS, CEContainer> CEBackInsertIterator;

	//simple version: first get all points that change their coordinates and save these and their incident constraint edges
	//then remove these vertices and readd the constraints that don't intersect other constraints
	//avoiding the creation of new , do this until no new intersection points are created
	
	auto locateVertex = [&ctd](const IntPoint & p) -> Vertex_handle {
		Locate_type lt = (Locate_type) -1;
		int li = 4;
		auto f = ctd.locate(p.toPoint(), lt, li);
		if (lt != TDS::VERTEX) {
			throw std::runtime_error("Could not locate vertex");
		}
		return f->vertex(li);
	};
	
	sserialize::spatial::DistanceCalculator dc(sserialize::spatial::DistanceCalculator::DCT_GEODESIC_ACCURATE);
	auto distanceTo = [&dc](const Point & p1, const Point & p2) {
		double lat1 = CGAL::to_double(p1.x());
		double lon1 = CGAL::to_double(p1.y());
		double lat2 = CGAL::to_double(p2.x());
		double lon2 = CGAL::to_double(p2.y());
		return std::abs<double>( dc.calc(lat1, lon1, lat2, lon2) );
	};
	
	uint32_t numChangedPoints;
	CEContainer ceQueue;
	//do the first global step which snaps all points
	{
		std::vector<Point> rmPoints;
		std::unordered_set<uint64_t> noConstraintsPoints;
		
		CEBackInsertIterator ceIt(ctd, ceQueue);
		
		for(Finite_vertices_iterator vt(ctd.finite_vertices_begin()), vtEnd(ctd.finite_vertices_end()); vt != vtEnd; ++vt) {
			const Point & p = vt->point();
			if (!IntPoint::changes(p)) {
				continue;
			}
			//point changes, save it and add its constrained edges
			if( ctd.are_there_incident_constraints(vt) ) {
				ctd.incident_constraints(vt, ceIt);
			}
			else {
				noConstraintsPoints.emplace(IntPoint(p).toU64());
			}
			rmPoints.emplace_back(p);
		}
		//now remove all those bad points
		for(const Point & p : rmPoints) {
			Face_handle fh;
			Locate_type lt = (Locate_type) -1;
			int li;
			fh = ctd.locate(p, lt, li);
			if (lt != TDS::VERTEX) {
				std::cerr << "sserialize::Static::Triangulation::prepare: Could not locate point" << std::endl;
				continue;
			}
			if (li < 4) { //if dimension is 0, then locate returns a NullFace with lt set to VERTEX and li set to 4
				Vertex_handle v = fh->vertex(li);
				ctd.remove_incident_constraints(v);
				ctd.remove(v);
			}
		}
		//add points from edges and points without constraints, we first remove all multiple occurences
		{
			std::unordered_set<uint64_t> pts = std::move(noConstraintsPoints);
			for(const ConstrainedEdge & e : ceQueue.container()) {
				pts.emplace(e.p1.toU64());
				pts.emplace(e.p2.toU64());
			}
			std::vector<Point> ipts;
			ipts.reserve(pts.size());
			for(uint64_t x : pts) {
				ipts.emplace_back(IntPoint(x).toPoint());
			}
			ctd.insert(ipts.begin(), ipts.end());
			numChangedPoints = (uint32_t) ipts.size();
// 			SSERIALIZE_ASSERT(pts.count(IntPoint(2336098625, 3137055126).toU64()));
		}
	}
	
	uint32_t initialQueueSize = (uint32_t) ceQueue.size();
	uint32_t targetQueueRounds = initialQueueSize;
	uint32_t queueRound = 0;
	
	//ceQueue makes sure that long edges come first
	sserialize::ProgressInfo pinfo;
	pinfo.begin(targetQueueRounds, "Triangulation::prepare: Processing edges");
	for(; ceQueue.size() && ceQueue.top().length > minEdgeLength; ++queueRound) {
		pinfo(queueRound, targetQueueRounds);
		ConstrainedEdge e = ceQueue.top();
		ceQueue.pop();
		SSERIALIZE_CHEAP_ASSERT(e.valid());
		Vertex_handle v1(locateVertex(e.p1));
		Vertex_handle v2(locateVertex(e.p2));
		if (ctd.is_edge(v1, v2)) {
			ctd.insert_constraint(v1, v2);
			continue;
		}
		//we have intersections, calculate the intersection points and decide how to snap them
		bool intersected = false;
		Triangulation::intersects(ctd, v1, v2, [&ctd, &distanceTo, &v1, &v2, &e, &intersected, &ceQueue, &targetQueueRounds, &numChangedPoints](const Edge & xEdge) -> bool {
			intersected = true;
			Point xP;
			const Point & p1 = v1->point();
			const Point & p2 = v2->point();
			const Point & pc = xEdge.first->vertex(TDS::ccw(xEdge.second))->point();
			const Point & pd = xEdge.first->vertex(TDS::cw(xEdge.second))->point();
			CGAL::intersection(ctd.geom_traits(), p1, p2, pc, pd, xP, Intersection_tag());

			SSERIALIZE_CHEAP_ASSERT(!IntPoint::changes(pc));
			SSERIALIZE_CHEAP_ASSERT(!IntPoint::changes(pd));
			//maps from distance -> point
			std::array< std::pair<double, IntPoint>, 4> tmp;
			tmp[0].second = e.p1;
			tmp[1].second = e.p2;
			tmp[2].second = IntPoint(pc);
			tmp[3].second = IntPoint(pd);
			tmp[0].first = distanceTo(p1, xP);
			tmp[1].first = distanceTo(p2, xP);
			tmp[2].first = distanceTo(pc, xP);
			tmp[3].first = distanceTo(pd, xP);
			auto tmpMinIt = std::min_element(tmp.begin(), tmp.end(),
				[](const std::pair<double, IntPoint> & a, const std::pair<double, IntPoint> & b) {
					return a.first < b.first;
				}
			);
			
			IntPoint xIntPoint;
			bool insertXIntP = false;
			//now check how far away that point is from one of our endpoints
			if (tmpMinIt->first < 0.05) { //close enough
				xIntPoint = tmpMinIt->second;
			}
			else {
				xIntPoint = IntPoint(xP);
				insertXIntP = true;
			}
			const IntPoint & pcInt = tmp[2].second;
			const IntPoint & pdInt = tmp[3].second;

			//requeue edge and xEdge with their new intersection point
			//xP and xIntPoint maybe different at this point
			if (e.p1 != xIntPoint) {
				ceQueue.emplace(e.p1, xIntPoint);
				++targetQueueRounds;
			}
			if (e.p2 != xIntPoint) {
				ceQueue.emplace(e.p2, xIntPoint);
				++targetQueueRounds;
			}
			if (pcInt!= xIntPoint) {
				ceQueue.emplace(pcInt, xIntPoint);
				++targetQueueRounds;
			}
			if (pdInt != xIntPoint) {
				ceQueue.emplace(pdInt, xIntPoint);
				++targetQueueRounds;
			}
			//and remove the constrained on our current edge
			ctd.remove_constrained_edge(xEdge.first, xEdge.second);
			if (insertXIntP) {
				++numChangedPoints; //TODO: if xIntPoint is already in the tds, then this is wrong
				ctd.insert(xIntPoint.toPoint());
			}
			//its important to return false here since ctd.remove_constrained_edge
			//changes the triangulation
			return false; 
		});
		//readd constraint, since function above was not called due to no intersection
		if (!intersected) {
			ctd.insert_constraint(v1, v2);
			continue;
		}
	}
	pinfo.end();
	std::cout << "Processed " << initialQueueSize << " changed constrained edges in " << queueRound << " rounds" << std::endl;
	for(const ConstrainedEdge & e : ceQueue.container()) {
		re(e.p1.toGeoPoint(), e.p2.toGeoPoint());
	}
	#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	for(Finite_vertices_iterator vt(ctd.finite_vertices_begin()), vtEnd(ctd.finite_vertices_end()); vt != vtEnd; ++vt) {
		SSERIALIZE_EXPENSIVE_ASSERT(!IntPoint::changes(vt->point()));
	}
	#endif
	return numChangedPoints;
}

//END stuff for snapping

template<typename T_SOURCE_POINT, typename T_TARGET_POINT>
struct Convert {
	T_TARGET_POINT operator()(const T_SOURCE_POINT & p) const;
};

template<typename T_TARGET_POINT>
struct Convert<sserialize::spatial::GeoPoint, T_TARGET_POINT> {
	T_TARGET_POINT operator()(const sserialize::spatial::GeoPoint & gp) const {
		return T_TARGET_POINT(gp.lat(), gp.lon());
	}
};

template<typename T_SOURCE_POINT>
struct Convert<T_SOURCE_POINT, sserialize::spatial::GeoPoint> {
	sserialize::spatial::GeoPoint operator()(const T_SOURCE_POINT & p) const {
		return sserialize::spatial::GeoPoint(CGAL::to_double(p.x()), CGAL::to_double(p.y()));
	}
};

template<typename T_TARGET_POINT>
struct Convert<sserialize::Static::spatial::ratss::PointOnS2, T_TARGET_POINT> {
	T_TARGET_POINT operator()(const sserialize::Static::spatial::ratss::PointOnS2 & p) const {
		return T_TARGET_POINT(p.x(), p.y(), p.z());
	}
};

template<typename T_SOURCE_POINT>
struct Convert<T_SOURCE_POINT, sserialize::Static::spatial::ratss::PointOnS2> {
	sserialize::Static::spatial::ratss::PointOnS2 operator()(const T_SOURCE_POINT & p) const {
		return sserialize::Static::spatial::ratss::PointOnS2(p.x(), p.y(), p.z());
	}
};

template<typename TPoint>
struct Compute_centroid {
	typedef TPoint Point;
	Point operator()(const Point & a, const Point & b, const Point & c) const;
};

template<>
struct Compute_centroid<sserialize::spatial::GeoPoint> {
	typedef sserialize::spatial::GeoPoint Point;
	Point operator()(const Point & a, const Point & b, const Point & c) const {
		double lat = (a.lat() + b.lat() + c.lat()) / 3;
		double lon = (a.lon() + b.lon() + c.lon()) / 3;
		return Point(lat, lon);
	}
};


}}//end namespace detail::Triangulation

}}}//end namespace


#endif