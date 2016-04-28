#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_VERSION 2
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/containers/OOMArray.h>
#include <sserialize/algorithm/oom_algorithm.h>
#include <sserialize/spatial/LatLonCalculations.h>
#include <sserialize/spatial/DistanceCalculator.h>

#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	#include <sserialize/algorithm/hashspecializations.h>
#endif

#include <CGAL/number_utils.h>
#include <CGAL/enum.h>
#include <CGAL/Constrained_triangulation_2.h>

#include <queue>

namespace sserialize {
namespace Static {
namespace spatial {

class Triangulation;

namespace detail {
namespace Triangulation {

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
	//this will prefer long edges over short edges during re-adding (note the std:reverse later)
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

}}//end namespace detail::Triangulation

/**
  * Layout (v1)
  * {
  *   VERSION u8
  *   POINTS sserialize::Array<GeoPoint> gp (id identical with vertex ids)
  *   VERTICES sserialize::MultiVarBitArray
  *   {
  *      FacesBegin u?
  *      FacesEnd u?
  *   }
  *   FACES    sserialize::MultiVarBitArray
  *   {
  *      ValidNeighbor u3
  *      Neighbors 3*u?
  *      Vertices 3*u?
  *   }
  * }
  * Changelog:
  */

///Triangulation of the convex-hull of a set of points
class Triangulation final {
public:
	typedef sserialize::spatial::GeoPoint Point;
	typedef uint32_t FaceId;
	typedef uint32_t VertexId;
	class Face;
	class Vertex;
	class FaceCirculator;
	
	class Vertex final {
	private:
		friend class Triangulation;
		friend class Face;
		friend class FaceCirculator;
	private:
		typedef enum {
			VI_FACES_BEGIN=0, VI_FACES_END=1, VI__NUMBER_OF_ENTRIES=2
		} VertexInfo;
	private:
		const Triangulation * m_p;
		uint32_t m_pos;
	private:
		Vertex(const Triangulation * p, uint32_t pos);
		uint32_t beginFaceId() const;
		uint32_t endFaceId() const;
		Face beginFace() const;
		Face endFace() const;
	public:
		Vertex();
		~Vertex();
		bool valid() const;
		inline uint32_t id() const { return m_pos; }
		Point point() const;
		FaceCirculator faces() const;
		FaceCirculator facesBegin() const;
		///This iterator points to the LAST face and NOT! one-passed the end
		FaceCirculator facesEnd() const;
		bool operator==(const Vertex & other) const;
		bool operator!=(const Vertex & other) const;
		void dump(std::ostream & out) const;
		void dump() const;
	};

	///A Face has up to 3 neighbors
	class Face final {
	private:
		friend class Triangulation;
		friend class Vertex;
		friend class FaceCirculator;
	private:
		typedef enum : uint32_t {
			FI_NEIGHBOR_VALID=0,
			FI_NEIGHBOR0=1, FI_NEIGHBOR1=2, FI_NEIGHBOR2=3, FI_NEIGHBOR_BEGIN=FI_NEIGHBOR0, FI_NEIGHBOR_END=FI_NEIGHBOR2+1,
			FI_VERTEX0=4, FI_VERTEX1=5, FI_VERTEX2=6, FI_VERTEX_BEGIN=FI_VERTEX0, FI_VERTEX_END=FI_VERTEX2+1,
			FI_IS_DEGENERATE=7, FI_ADDITIONAL_INFO_BEGIN=FI_IS_DEGENERATE, FI_ADDITIONAL_INFO_END=FI_IS_DEGENERATE+1,
			FI__NUMBER_OF_ENTRIES=FI_ADDITIONAL_INFO_END
		} FaceInfo;
	private:
		const Triangulation * m_p;
		uint32_t m_pos;
	private:
		Face(const Triangulation * p, uint32_t pos);
	public:
		Face();
		~Face();
		inline uint32_t id() const { return m_pos; }
		bool valid() const;
		///a face is degenerate if any two vertices have the same coordinates
		///This may happen since the static version uses fixed precision numbers
		///The topology is still correct
		bool isDegenerate() const;
		bool isNeighbor(uint32_t pos) const;
		uint32_t neighborId(uint32_t pos) const;
		Face neighbor(uint32_t pos) const;
		uint32_t vertexId(uint32_t pos) const;
		Vertex vertex(uint32_t pos) const;
		Point point(uint32_t pos) const;
		bool contains(const Point & p) const;
		bool intersects(const Point & p, const Point & q) const;
		///inexact computed centroid
		Point centroid() const;
		///index of the vertex, -1 if vertex is not part of this face
		int index(const Vertex & v) const;
		void dump(std::ostream & out) const;
		void dump() const;
		bool operator==(const Face & other) const;
		bool operator!=(const Face & other) const;
	};
	
	class FaceCirculator final {
	private:
		Face m_f;
		Vertex m_v;
	public:
		FaceCirculator(const Vertex & v, const Face & f);
		~FaceCirculator();
		Face operator*() const;
		Face operator->() const;
		FaceCirculator & operator++();
		FaceCirculator operator++(int);
		FaceCirculator & operator--();
		FaceCirculator operator--(int);
		bool operator==(const FaceCirculator & other) const;
		bool operator!=(const FaceCirculator & other) const;
		const Vertex & vertex() const;
		const Face & face() const;
	};
public:
	static constexpr uint32_t NullFace = 0xFFFFFFFF;
	static constexpr uint32_t NullVertex = 0xFFFFFFFF;
private:
	typedef sserialize::MultiVarBitArray FaceInfos;
	typedef sserialize::MultiVarBitArray VertexInfos;
	typedef sserialize::Static::Array<Point> PointsContainer;
private:
	PointsContainer m_p;
	VertexInfos m_vi;
	FaceInfos m_fi;
protected:
	inline const FaceInfos & faceInfo() const { return m_fi; }
	inline const VertexInfos & vertexInfos() const { return m_vi; }
	inline const PointsContainer & points() const { return m_p; }
public:
	Triangulation();
	Triangulation(const sserialize::UByteArrayAdapter & d);
	~Triangulation();
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t vertexCount() const { return m_vi.size(); }
	uint32_t faceCount() const { return m_fi.size(); }
	Face face(uint32_t pos) const;
	Vertex vertex(uint32_t pos) const;
	
	///traverse the triangulation in a more or less straight line starting from startFace to endpoint
	///@return faceid where the destination point is inside or NullFace
	///@param visitor operator()(const Face & face)
	template<typename TVisitor, typename T_GEOMETRY_TRAITS>
	uint32_t traverse(double lat, double lon, uint32_t hint, TVisitor visitor, T_GEOMETRY_TRAITS traits = T_GEOMETRY_TRAITS()) const;
	///Locate the face the point=(lat, lon) lies in, need exact predicates, hint: id of start face
	template<typename T_GEOMETRY_TRAITS>
	uint32_t locate(double lat, double lon, uint32_t hint = 0, T_GEOMETRY_TRAITS traits = T_GEOMETRY_TRAITS()) const;
	///Explores the triangulation starting at startFace
	///@param explorer operator()(const Face & face) -> bool, return false if the exploration should stop at this face (neighbors of this face are not explored)
	template<typename T_EXPLORER>
	void explore(uint32_t startFace, T_EXPLORER explorer) const;
	bool selfCheck() const;
	void printStats(std::ostream & out) const;
	//counter-clock-wise next vertex/neighbor as defined in cgal
	inline static int ccw(const int i) { return (i+1)%3; }
	//clock-wise next vertex/neighbor as defined in cgal
	inline static int cw(const int i) { return (i+2)%3; }
	
	//counter-clock-wise next vertex/neighbor as defined in cgal
	inline static uint32_t ccw(const uint32_t i) { return (i+1)%3; }
	//clock-wise next vertex/neighbor as defined in cgal
	inline static uint32_t cw(const uint32_t i) { return (i+2)%3; }

	///prepare triangulation for serialization (currently contracts faces that are representable)
	template<typename T_CTD, typename T_REMOVED_EDGES = detail::Triangulation::PrintRemovedEdges>
	static bool prepare(T_CTD& ctd, T_REMOVED_EDGES re = T_REMOVED_EDGES(), uint32_t maxRounds = 1);
	
	template<typename T_CGAL_TRIANGULATION_DATA_STRUCTURE, typename T_VERTEX_TO_VERTEX_ID_MAP, typename T_FACE_TO_FACE_ID_MAP>
	static sserialize::UByteArrayAdapter & append(T_CGAL_TRIANGULATION_DATA_STRUCTURE & src, T_FACE_TO_FACE_ID_MAP & faceToFaceId, T_VERTEX_TO_VERTEX_ID_MAP & vertexToVertexId, sserialize::UByteArrayAdapter & dest);

};


//TODO:add support for degenerate faces
template<typename TVisitor, typename T_GEOMETRY_TRAITS>
uint32_t Triangulation::traverse(double lat, double lon, uint32_t hint, TVisitor visitor, T_GEOMETRY_TRAITS traits) const {
	typedef T_GEOMETRY_TRAITS K;
	typedef typename K::Point_2 Point_2;
	typedef typename K::Orientation_2 Orientation_2;
	typedef typename K::Collinear_are_ordered_along_line_2 Collinear_are_ordered_along_line_2;
	if (!faceCount()) {
		return NullFace;
	}
	
	auto getPoint2 = [](const Vertex & v) {
		Point gp(v.point());
		return Point_2(gp.lat(), gp.lon());
	};
	
	//TODO: there's currently no way to tell that the point is identical with a vertex
	auto returnFaceFromVertex = [](const Vertex & v) -> uint32_t {
		return v.facesBegin().face().id();
	};
	
	if (hint >= faceCount()) {
		hint = 0;
	}
	//(p,q,r) ->  CGAL::Orientation
	Orientation_2 ot(traits.orientation_2_object());
	//(p,q,r) = 1 iff q is between p and r
	Collinear_are_ordered_along_line_2 oal(traits.collinear_are_ordered_along_line_2_object());
	Vertex circleVertex = face(hint).vertex(0);
	Point_2 cv;
	
	Point_2 q(lat, lon); //target
	Point_2 p(getPoint2(circleVertex)); //start point
	
	//TODO: do initialization step with centroid of face instead of an initial circle step

	Point_2 rp,lp;
	Vertex rv, lv;
	Face curFace;
	
	while (true) {
		if (circleVertex.valid()) {
			cv = getPoint2(circleVertex);
			
			//reset p to cv, since then we don't have to explicity check if q is inside the active triangle in case s is collinear with p->q
			//this has the overhead of going back to the former triangle but simplifies the logic
			p = cv;
			
			if (cv == q) {
				return returnFaceFromVertex(circleVertex);
			}
			//p->q goes through circleVertex, we have to find the right triangle
			//That triangle has circleVertex as a vertex
			FaceCirculator fcBegin(circleVertex.facesBegin()), fcEnd(circleVertex.facesEnd());
			while (true) {
				const Face & cf = fcBegin.face();
				SSERIALIZE_CHEAP_ASSERT(cf.valid());
				int cvIdx = cf.index(circleVertex);
				SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(cvIdx, -1);
				
				Vertex myLv(cf.vertex((uint32_t)Triangulation::cw(cvIdx))), myRv(cf.vertex((uint32_t)Triangulation::ccw(cvIdx)));
				Point_2 myLP(getPoint2(myLv)), myRP(getPoint2(myRv));
				
				CGAL::Orientation lvOt = ot(p, q, myLP);
				CGAL::Orientation rvOt = ot(p, q, myRP);
				
				//we've found the right triangle
				if (lvOt == CGAL::Orientation::LEFT_TURN && rvOt == CGAL::Orientation::RIGHT_TURN) {
					//check if the point is within this triangle
					//we do this by checking if it is to the right of the line l->r or on it
					CGAL::Orientation lrqO = ot(myLP, myRP, q);
					if (lrqO == CGAL::Orientation::RIGHT_TURN || lrqO == CGAL::COLLINEAR) {
						return cf.id();
					}
					else {
						rv = myRv;
						lv = myLv;
						rp = myRP;
						lp = myLP;
						circleVertex = Vertex();
						//the next face is the face that shares the edge myLv<->myRv with cf
						//check if it's inside our triangulation 
						if (!cf.isNeighbor((uint32_t)cvIdx)) { //next face is outside (this is only valid for convex triangulations)
							return NullFace;
						}
						curFace = cf.neighbor((uint32_t)cvIdx);
						visitor(curFace);
						break;
					}
				}
				else if (lvOt == CGAL::Orientation::COLLINEAR) {
					if (oal(p, q, myLP)) { //q lies on the edge, this has to come first in case both p and q lie on vertices
						return cf.id();
					}
					else if (oal(p, myLP, q)) {
						circleVertex = myLv;
						break;
					}
				}
				else if (rvOt == CGAL::Orientation::COLLINEAR) {
					if (oal(p, q, myRP)) { //q lies on the edge, this has to come first in case both p and q lie on vertices
						return cf.id();
					}
					else if (oal(p, myRP, q)) {
						circleVertex = myRv;
						break;
					}
				}
				//we've tested all faces and none matched, thus p must be outside of our triangulation
				//This is only correct, if the triangulation is convex
				if (fcBegin == fcEnd) {
					return NullFace;
				}
				else {
					++fcBegin;
				}
			}
			//next iteration
			continue;
		}
		else {
			SSERIALIZE_CHEAP_ASSERT(curFace.valid());
			//we have a face, r, l, rv and lv are set, find s
			//p->q does not pass through r or l but may pass through s
			//p->q intersects l->r with l beeing on the left and r beeing on the right
			Vertex sv;
			int lvIndex = curFace.index(lv);
			int rvIndex;
			if (curFace.vertexId((uint32_t)Triangulation::cw(lvIndex)) != rv.id()) {
				sv = curFace.vertex((uint32_t)cw(lvIndex));
				rvIndex = ccw(lvIndex);
			}
			else {
				sv = curFace.vertex((uint32_t)ccw(lvIndex));
				rvIndex = cw(lvIndex);
			}
			SSERIALIZE_CHEAP_ASSERT_EQUAL(curFace.vertexId((uint32_t)rvIndex), rv.id());
			SSERIALIZE_CHEAP_ASSERT_EQUAL(curFace.vertexId((uint32_t)lvIndex), lv.id());
			
			Point_2 sp(getPoint2(sv));
			CGAL::Orientation sot = ot(p, q, sp);
			if (CGAL::Orientation::COLLINEAR == sot) {
				//top takes care of q beeing in the current triangle
				circleVertex = sv;
			}
			else if (CGAL::Orientation::LEFT_TURN == sot) {
				//check if q is within our face
				//this is the case if q is to the right of l->s 
				CGAL::Orientation rsqO = ot(rp, sp, q);
				if (rsqO == CGAL::Orientation::LEFT_TURN || rsqO == CGAL::Orientation::COLLINEAR) {
					return curFace.id();
				}
				if (!curFace.isNeighbor((uint32_t)lvIndex)) {
					return NullFace;
				}
				lv = sv;
				lp = sp;
				curFace = curFace.neighbor((uint32_t)lvIndex);
				visitor(curFace);
			}
			else if (CGAL::Orientation::RIGHT_TURN == sot) {
				//check if q is within our face
				//this is the case if q is to the left of r->s
				CGAL::Orientation lsqO = ot(lp, sp, q);
				if (lsqO == CGAL::Orientation::RIGHT_TURN || lsqO == CGAL::Orientation::COLLINEAR) {
					return curFace.id();
				}
				if (!curFace.isNeighbor((uint32_t)rvIndex)) {
					return NullFace;
				}
				rv = sv;
				rp = sp;
				curFace = curFace.neighbor((uint32_t)rvIndex);
				visitor(curFace);
			}
			else {
				throw std::runtime_error("sserialize::Static::Triangulation::locate: unexpected error");
			}
		}
	}
	
	return NullFace;
}

template<typename T_GEOMETRY_TRAITS>
uint32_t Triangulation::locate(double lat, double lon, uint32_t hint, T_GEOMETRY_TRAITS traits) const {
	return traverse(lat, lon, hint, [](Face const &) {}, traits);
}

template<typename T_EXPLORER>
void Triangulation::explore(uint32_t startFace, T_EXPLORER explorer) const {
	std::unordered_set<uint32_t> visitedFaces;
	std::unordered_set<uint32_t> pendingFaces;
	pendingFaces.insert(startFace);
	
	while (pendingFaces.size()) {
		uint32_t cfId;
		{
			auto tmp(pendingFaces.begin());
			cfId = *tmp;
			pendingFaces.erase(tmp);
		}
		Face cf(face(cfId));
		if (explorer(cf)) {
			for(uint32_t j(0); j < 3; ++j) {
				if (cf.isNeighbor(j)) {
					uint32_t nId = cf.neighborId(j);
					if (!visitedFaces.count(nId)) {
						pendingFaces.insert(nId);
						//nId is going to be visited in the future
						visitedFaces.insert(nId);
					}
				}
			}
		}
	}
}

///This will handle points created by intersections of constrained edges
///This makes all points representable! Beware that this very likely changes the triangulation (removing faces and adding new ones)
///You should therefore snap points before creating the triangulation
///@return true iff changes were made
template<typename T_CTD, typename T_REMOVED_EDGES>
bool Triangulation::prepare(T_CTD & ctd, T_REMOVED_EDGES re, uint32_t maxRounds) {
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
// 			SSERIALIZE_ASSERT(pts.count(IntPoint(2336098625, 3137055126).toU64()));
		}
	}
	
	uint32_t initialQueueSize = ceQueue.size();
	uint32_t targetQueueRounds = initialQueueSize;
	uint32_t queueRound = 0;
	
	//ceQueue makes sure that long edges come first
	sserialize::ProgressInfo pinfo;
	pinfo.begin(targetQueueRounds, "Triangulation::prepare: Processing edges");
	for(; ceQueue.size(); ++queueRound) {
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
		detail::Triangulation::intersects(ctd, v1, v2, [&ctd, &distanceTo, &v1, &v2, &e, &intersected, &ceQueue, &targetQueueRounds](const Edge & xEdge) -> bool {
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
	#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	{
		std::unordered_set< std::pair<uint32_t, uint32_t> > pts;
		for(Finite_vertices_iterator vt(ctd.finite_vertices_begin()), vtEnd(ctd.finite_vertices_end()); vt != vtEnd; ++vt) {
			IntPoint p(vt->point());
			pts.emplace(p.lat, p.lon);
		}
		SSERIALIZE_CHEAP_ASSERT_EQUAL(pts.size(), ctd.number_of_vertices());
	}
	#endif
	return 0;
}

template<typename T_CGAL_TRIANGULATION_DATA_STRUCTURE, typename T_VERTEX_TO_VERTEX_ID_MAP, typename T_FACE_TO_FACE_ID_MAP>
sserialize::UByteArrayAdapter &
Triangulation::append(T_CGAL_TRIANGULATION_DATA_STRUCTURE & src, T_FACE_TO_FACE_ID_MAP & faceToFaceId, T_VERTEX_TO_VERTEX_ID_MAP & vertexToVertexId, sserialize::UByteArrayAdapter & dest) {
	typedef T_CGAL_TRIANGULATION_DATA_STRUCTURE TDS;
	typedef typename TDS::Face_handle Face_handle;
	typedef typename TDS::Vertex_handle Vertex_handle;
	typedef typename TDS::Finite_faces_iterator Finite_faces_iterator;
	typedef typename TDS::Finite_vertices_iterator Finite_vertices_iterator;
	typedef typename TDS::Face_circulator Face_circulator;
	
	faceToFaceId.clear();
	vertexToVertexId.clear();
	
	uint32_t faceId = 0;
	uint32_t vertexId = 0;
	
	uint32_t faceCount = 0;
	uint32_t vertexCount = 0;
	
	uint32_t degenerateFaceCount = 0;
	//the reduced precision of the serialization may produce degenerate triangultions
	//or even triangulations that are incorrect (like self intersections due to rounding errors etc.)
	//we would need to snap the vertices accordingly, unfortunately this not easy
	//as a remedy, mark faces that are degenerate
	//Alternative?: remove vertices that change their precision in such a way that they snap on to another vertex
	
	
	auto toIntLat = [](double v) { return sserialize::spatial::GeoPoint::toIntLat(v); };
	auto toIntLon = [](double v) { return sserialize::spatial::GeoPoint::toIntLon(v); };
	
	for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
		faceToFaceId[fh] = faceId;
		++faceId;
	}
	faceCount = faceId;
	
	dest.putUint8(2);//VERSION
	{ //put the points
		sserialize::spatial::GeoPoint gp;
		sserialize::Static::ArrayCreator<sserialize::spatial::GeoPoint> va(dest);
		va.reserveOffsets(src.number_of_vertices());
		for(Finite_vertices_iterator vt(src.finite_vertices_begin()), vtEnd(src.finite_vertices_end()); vt != vtEnd; ++vt) {
			gp.lat() = CGAL::to_double(vt->point().x());
			gp.lon() = CGAL::to_double(vt->point().y());
			va.put(gp);
			vertexToVertexId[vt] = vertexId;
			++vertexId;
		}
		va.flush();
		vertexCount = vertexId;
	}
	{//put the vertex info
		std::vector<uint8_t> bitConfig(Triangulation::Vertex::VI__NUMBER_OF_ENTRIES);
		bitConfig[Triangulation::Vertex::VI_FACES_BEGIN] = (uint8_t) sserialize::CompactUintArray::minStorageBits(faceCount);
		bitConfig[Triangulation::Vertex::VI_FACES_END] = bitConfig[Triangulation::Vertex::VI_FACES_BEGIN];
		sserialize::MultiVarBitArrayCreator va(bitConfig, dest);
		for(Finite_vertices_iterator vt(src.finite_vertices_begin()), vtEnd(src.finite_vertices_end()); vt != vtEnd; ++vt) {
			SSERIALIZE_NORMAL_ASSERT(vertexToVertexId.is_defined(vt));
			uint32_t vertexId = vertexToVertexId[vt];
			uint32_t beginFace, endFace;
			Face_circulator fc(src.incident_faces(vt));
			{
				uint32_t numFinite(0), numInfinite(0);
				Face_circulator fcIt(fc);
				++fcIt;
				for(;fcIt != fc; ++fcIt) {
					if (src.is_infinite(fcIt)) {
						++numInfinite;
					}
					else {
						++numFinite;
					}
				}
				if (src.is_infinite(fc)) {
					++numInfinite;
				}
				else {
					++numFinite;
				}
			}
			for(;src.is_infinite(fc); ++fc) {}
			SSERIALIZE_NORMAL_ASSERT(!src.is_infinite(fc));
			//now move forward/backward until we either reach the fc or reach an infite face
			Face_circulator fcBegin(fc), fcEnd(fc);
			while(true) {
				--fcBegin;
				if (fcBegin == fc) { //we came around once
					break;
				}
				if (src.is_infinite(fcBegin)) {
					++fcBegin;
					break;
				}
				
			}
			while(true) {
				++fcEnd;
				if (fcEnd == fc) { //we came around once, prev is the last valid face
					--fcEnd;
					break;
				}
				if (src.is_infinite(fcEnd)) {
					--fcEnd;
					break;
				}
			}
			
			SSERIALIZE_NORMAL_ASSERT(!src.is_infinite(fcBegin));
			SSERIALIZE_NORMAL_ASSERT(!src.is_infinite(fcEnd));
			SSERIALIZE_NORMAL_ASSERT(faceToFaceId.is_defined(fcBegin));
			SSERIALIZE_NORMAL_ASSERT(faceToFaceId.is_defined(fcEnd));
			
			beginFace = faceToFaceId[fcBegin];
			endFace = faceToFaceId[fcEnd];
			
			va.set(vertexId, Triangulation::Vertex::VI_FACES_BEGIN, beginFace);
			va.set(vertexId, Triangulation::Vertex::VI_FACES_END, endFace);
		}
		va.flush();
	}
	{
		std::vector<uint8_t> bitConfig(Triangulation::Face::FI__NUMBER_OF_ENTRIES);
		bitConfig[Triangulation::Face::FI_NEIGHBOR_VALID] = 3;
		bitConfig[Triangulation::Face::FI_NEIGHBOR0] = (uint8_t)sserialize::CompactUintArray::minStorageBits(faceCount);
		bitConfig[Triangulation::Face::FI_NEIGHBOR1] = bitConfig[Triangulation::Face::FI_NEIGHBOR0];
		bitConfig[Triangulation::Face::FI_NEIGHBOR2] = bitConfig[Triangulation::Face::FI_NEIGHBOR0];
		bitConfig[Triangulation::Face::FI_VERTEX0] = (uint8_t)sserialize::CompactUintArray::minStorageBits(vertexCount);
		bitConfig[Triangulation::Face::FI_VERTEX1] = bitConfig[Triangulation::Face::FI_VERTEX0];
		bitConfig[Triangulation::Face::FI_VERTEX2] = bitConfig[Triangulation::Face::FI_VERTEX0];
		bitConfig[Triangulation::Face::FI_IS_DEGENERATE] = 1;
		sserialize::MultiVarBitArrayCreator fa(bitConfig, dest);
	
		faceId = 0;
		for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
			SSERIALIZE_NORMAL_ASSERT(faceToFaceId.is_defined(fh) && faceToFaceId[fh] == faceId);
			{//check degeneracy
				auto p0 = fh->vertex(0)->point();
				auto p1 = fh->vertex(1)->point();
				auto p2 = fh->vertex(2)->point();
				if (p0 == p1 || p0 == p2 || p1 == p2) { //this should not happen
					throw sserialize::CreationException("Triangulation has degenerate face in source triangulation");
				}
				using CGAL::to_double;
				if ((toIntLat(to_double(p0.x())) == toIntLat(to_double(p1.x())) && toIntLon(to_double(p0.y())) == toIntLon(to_double(p1.y()))) ||
					(toIntLat(to_double(p0.x())) == toIntLat(to_double(p2.x())) && toIntLon(to_double(p0.y())) == toIntLon(to_double(p2.y()))) ||
					(toIntLat(to_double(p1.x())) == toIntLat(to_double(p2.x())) && toIntLon(to_double(p1.y())) == toIntLon(to_double(p2.y()))))
				{
					throw sserialize::CreationException("Triangulation has degenerate face after serialization");
					fa.set(faceId, Triangulation::Face::FI_IS_DEGENERATE, 1);
					++degenerateFaceCount;
				}
			}
			
			uint8_t validNeighbors = 0;
			for(int j(0); j < 3; ++j) {
				Face_handle nfh = fh->neighbor(j);
				uint32_t nfhId = 0xFFFFFFFF;
				if (faceToFaceId.is_defined(nfh)) {
					nfhId = faceToFaceId[nfh];
					validNeighbors |= static_cast<uint8_t>(1) << j;
				}
				fa.set(faceId, Triangulation::Face::FI_NEIGHBOR_BEGIN+(uint32_t)j, nfhId);
			}
			fa.set(faceId, Triangulation::Face::FI_NEIGHBOR_VALID, validNeighbors);
			
			for(int j(0); j < 3; ++j) {
				Vertex_handle vh = fh->vertex(j);
				SSERIALIZE_NORMAL_ASSERT(vertexToVertexId.is_defined(vh));
				uint32_t vertexId = vertexToVertexId[vh];
				fa.set(faceId, Triangulation::Face::FI_VERTEX_BEGIN+(uint32_t)j, vertexId);
			}
			++faceId;
		}
		fa.flush();
		SSERIALIZE_NORMAL_ASSERT(faceId == faceCount);
	}
	if (degenerateFaceCount) {
		std::cout << "Triangulation has " << degenerateFaceCount << " degenerate faces!" << std::endl;
	}
	return dest;
}


}}} //end namespace

#endif