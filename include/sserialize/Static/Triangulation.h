#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/containers/UnionFind.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/GeoPoint.h>
#include <assert.h>
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_VERSION 1

#include <CGAL/number_utils.h>
#include <CGAL/enum.h>
#include <CGAL/Unique_hash_map.h>

namespace sserialize {
namespace Static {
namespace spatial {

class Triangulation;

namespace detail {
namespace Triangulation {

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
			FI__NUMBER_OF_ENTRIES=FI_VERTEX_END
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

	///contract node v2 and v1 by removing v2 and adding the respective constraints if needed
	template<typename T_CTD>
	static void contract(T_CTD & ctd, typename T_CTD::Vertex_handle v1, typename T_CTD::Vertex_handle v2);
	
	///prepare triangulation for serialization (currently contracts faces that are representable)
	template<typename T_CTD>
	static void prepare(T_CTD & ctd);
	
	template<typename T_CGAL_TRIANGULATION_DATA_STRUCTURE, typename T_VERTEX_TO_VERTEX_ID_MAP, typename T_FACE_TO_FACE_ID_MAP>
	static sserialize::UByteArrayAdapter & append(T_CGAL_TRIANGULATION_DATA_STRUCTURE & src, T_FACE_TO_FACE_ID_MAP & faceToFaceId, T_VERTEX_TO_VERTEX_ID_MAP & vertexToVertexId, sserialize::UByteArrayAdapter & dest);

};

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

template<typename T_CTD>
void Triangulation::contract(T_CTD & ctd, typename T_CTD::Vertex_handle v1, typename T_CTD::Vertex_handle v2) {
	typedef T_CTD TDS;
	typedef typename TDS::Vertex_handle Vertex_handle;
	typedef typename TDS::Vertex_circulator Vertex_circulator;
	
	SSERIALIZE_CHEAP_ASSERT(v1 != v2);
	//targets of constraint vertices
	std::vector<Vertex_handle> cVerts;
	
	Vertex_circulator vC(ctd.incident_vertices(v2));
	Vertex_circulator vEnd(vC);
	if (vC != 0) {
		do {
			if (vC != v1) {
				cVerts.push_back(vC);
			}
			++vC;
		} while (vC != vEnd);
	}
	//first remove constraints
	
	ctd.remove_incident_constraints(v2);
	ctd.remove(v2);
	
	//now readd the constraints TODO: does this create new intersection points?
	for(Vertex_handle & vh : cVerts) {
		ctd.insert_constraint(v1, vh);
	}
}

template<typename T_CTD>
void Triangulation::prepare(T_CTD & ctd) {
	typedef T_CTD TDS;
	typedef typename TDS::Vertex_handle Vertex_handle;
	typedef typename TDS::Finite_vertices_iterator Finite_vertices_iterator;
	typedef typename TDS::Vertex_circulator Vertex_circulator;
	typedef typename TDS::Point Point;
	
	typedef sserialize::UnionFind<Vertex_handle> UnionFind;
	typedef typename UnionFind::handle_type UFHandle;
	typedef CGAL::Unique_hash_map<Vertex_handle, UFHandle> VertexHashMap;
	
	//by definition p2 is different from intLat/intLon before mapping
	auto mapToSame = [](uint32_t intLat, uint32_t intLon, const Point & p2) -> bool {
		return (intLat == sserialize::spatial::GeoPoint::toIntLat(CGAL::to_double(p2.x())) ||
				intLon == sserialize::spatial::GeoPoint::toIntLon(CGAL::to_double(p2.y())));
	};
	
	std::size_t nr1PhaseTotalContractions = 0;
	std::size_t nr2PhaseTotalContractions = 0;
	std::size_t nr1PhaseCVerts = 0;
	std::size_t nr2PhaseCVerts = 0;
	std::size_t contractRounds = 0;
	do {
		++contractRounds;
		nr1PhaseCVerts = 0;
		nr2PhaseCVerts = 0;
		{
			UnionFind contractVerts;
			VertexHashMap vert2UFHandle;
			
			contractVerts.reserve(ctd.number_of_vertices());
			for(Finite_vertices_iterator vt(ctd.finite_vertices_begin()), vtEnd(ctd.finite_vertices_end()); vt != vtEnd; ++vt) {
				auto h = contractVerts.make_set(vt);
				vert2UFHandle[vt] = h;
			}
			
			//find nodes that need to be contracted (is this even necessary?
			for(typename UnionFind::iterator it(contractVerts.begin()), end(contractVerts.end()); it != end; ++it) {
				Vertex_handle vh = it.value();
				SSERIALIZE_CHEAP_ASSERT(vert2UFHandle.is_defined(vh));
				UFHandle vufh = vert2UFHandle[vh];
				Point vhp = vh->point();
				uint32_t intLat = sserialize::spatial::GeoPoint::toIntLat(CGAL::to_double(vhp.x()));
				uint32_t intLon = sserialize::spatial::GeoPoint::toIntLon(CGAL::to_double(vhp.y()));
				Vertex_circulator vC(ctd.incident_vertices(vh));
				Vertex_circulator vEnd(vC);
				if (vC == 0) {
					continue;
				}
				do {
					if (mapToSame(intLat, intLon, vC->point())) {
						SSERIALIZE_CHEAP_ASSERT(vert2UFHandle.is_defined(vC));
						UFHandle ufh = vert2UFHandle[vC];
						contractVerts.unite(vufh, ufh);
					}
					++vC;
				} while (vC != vEnd);
			}
			
			//now let's contract the nodes
			for(typename UnionFind::iterator it(contractVerts.begin()), end(contractVerts.end()); it != end; ++it) {
				UFHandle rep = contractVerts.find( it.handle() );
				if (rep != it.handle()) {
					contract(ctd, contractVerts.get(rep), it.value());
					++nr1PhaseCVerts ;
				}
			}
		}
		{
			std::unordered_map< std::pair<uint32_t, uint32_t>, Vertex_handle> baseVerts;
			std::vector<Vertex_handle> contractVerts;
			std::pair<uint32_t, uint32_t> ip;
			for(Finite_vertices_iterator vt(ctd.finite_vertices_begin()), vtEnd(ctd.finite_vertices_end()); vt != vtEnd; ++vt) {
				Point vhp = vt->point();
				ip.first = sserialize::spatial::GeoPoint::toIntLat(CGAL::to_double(vhp.x()));
				ip.second = sserialize::spatial::GeoPoint::toIntLon(CGAL::to_double(vhp.y()));
				if (baseVerts.count(ip)) {
					contractVerts.emplace_back(vt);
				}
				else {
					baseVerts[ip] = vt;
				}
			}
			for(Vertex_handle & vh : contractVerts) {
				Point vhp = vh->point();
				ip.first = sserialize::spatial::GeoPoint::toIntLat(CGAL::to_double(vhp.x()));
				ip.second = sserialize::spatial::GeoPoint::toIntLon(CGAL::to_double(vhp.y()));
				contract(ctd, baseVerts.at(ip), vh);
			}
			nr2PhaseCVerts = contractVerts.size();
		}
		
		nr1PhaseTotalContractions += nr1PhaseCVerts;
		nr2PhaseTotalContractions += nr2PhaseCVerts;
	} while (nr1PhaseCVerts || nr2PhaseCVerts);
	std::cout << "sserialize::Static::Triangulation:prepare:\n\tPhase 1 contractions: ";
	std::cout << nr1PhaseTotalContractions << "\n\tPhase 2 contractions: ";
	std::cout  << nr2PhaseTotalContractions << "\n\tRounds: ";
	std::cout  << contractRounds << std::endl;
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
	

	//the reduced precision of the serialization may produce degenerate triangultions
	//or even triangulations that are incorrect (like self intersections due to rounding errors etc.)
	//This should not happen => unite nodes that get the same coordinates
	//Here we only check if the triangulation satisfies the condition

	auto toIntLat = [](double v) { return sserialize::spatial::GeoPoint::toIntLat(v); };
	auto toIntLon = [](double v) { return sserialize::spatial::GeoPoint::toIntLon(v); };
	
	for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
		auto p0 = fh->vertex(0)->point();
		auto p1 = fh->vertex(1)->point();
		auto p2 = fh->vertex(2)->point();
		if (p0 == p1 || p0 == p2 || p1 == p2) {
			throw sserialize::CreationException("Triangulation has degenerate face");
		}
		using CGAL::to_double;
		if (toIntLat(to_double(p0.x())) == toIntLat(to_double(p1.x())) || toIntLon(to_double(p0.y())) == toIntLon(to_double(p1.y())) ||
			toIntLat(to_double(p0.x())) == toIntLat(to_double(p2.x())) || toIntLon(to_double(p0.y())) == toIntLon(to_double(p2.y())) ||
			toIntLat(to_double(p1.x())) == toIntLat(to_double(p2.x())) || toIntLon(to_double(p1.y())) == toIntLon(to_double(p2.y())))
		{
			throw sserialize::CreationException("Triangulation has degenerate face after serialization");
		}
	}
	
	for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
		faceToFaceId[fh] = faceId;
		++faceId;
	}
	faceCount = faceId;
	
	dest.putUint8(1);//VERSION
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
		sserialize::MultiVarBitArrayCreator fa(bitConfig, dest);
	
		faceId = 0;
		for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
			SSERIALIZE_NORMAL_ASSERT(faceToFaceId.is_defined(fh) && faceToFaceId[fh] == faceId);
			
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
	return dest;
}


}}} //end namespace

#endif