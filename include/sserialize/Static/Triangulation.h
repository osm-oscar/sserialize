#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_VERSION 2
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <sserialize/containers/OOMArray.h>
#include <sserialize/algorithm/oom_algorithm.h>
#include <sserialize/spatial/DistanceCalculator.h>
#include <sserialize/Static/detail/Triangulation.h>

#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	#include <sserialize/algorithm/hashspecializations.h>
#endif

#include <queue>

namespace sserialize {
namespace Static {
namespace spatial {

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
	uint32_t traverse(const Point & target, uint32_t hint, TVisitor visitor, T_GEOMETRY_TRAITS traits = T_GEOMETRY_TRAITS()) const;
	///Locate the face the point lies in, need exact predicates, hint: id of start face
	template<typename T_GEOMETRY_TRAITS>
	uint32_t locate(const Point & target, uint32_t hint = 0, T_GEOMETRY_TRAITS traits = T_GEOMETRY_TRAITS()) const;
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

	///prepare triangulation for serialization (currently contracts faces that are not representable)
	///@param minEdgeLength the minimal length an edge has to have in order to be processed
	template<typename T_CTD, typename T_REMOVED_EDGES = detail::Triangulation::PrintRemovedEdges>
	inline static uint32_t prepare(T_CTD& ctd, T_REMOVED_EDGES re = T_REMOVED_EDGES(), double minEdgeLength = 0.0) {
		return detail::Triangulation::snap_vertices(ctd, re, minEdgeLength);
	}
	
	template<typename T_CGAL_TRIANGULATION_DATA_STRUCTURE, typename T_VERTEX_TO_VERTEX_ID_MAP, typename T_FACE_TO_FACE_ID_MAP>
	static sserialize::UByteArrayAdapter & append(T_CGAL_TRIANGULATION_DATA_STRUCTURE & src, T_FACE_TO_FACE_ID_MAP & faceToFaceId, T_VERTEX_TO_VERTEX_ID_MAP & vertexToVertexId, sserialize::UByteArrayAdapter & dest);
	
};

//TODO:add support for degenerate faces
template<typename TVisitor, typename T_GEOMETRY_TRAITS>
uint32_t Triangulation::traverse(const Point & target, uint32_t hint, TVisitor visitor, T_GEOMETRY_TRAITS traits) const {
	typedef T_GEOMETRY_TRAITS K;
	typedef typename K::Point_2 Point_2;
	typedef typename K::Orientation_2 Orientation_2;
	typedef typename K::Collinear_are_ordered_along_line_2 Collinear_are_ordered_along_line_2;
	if (!faceCount()) {
		return NullFace;
	}
	
	detail::Triangulation::Convert<Triangulation::Point, Point_2> mp2kp;
	
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
	
	Point_2 q( mp2kp(target) ); //target
	Point_2 p( mp2kp(circleVertex.point()) ); //start point
	
	//TODO: do initialization step with centroid of face instead of an initial circle step

	Point_2 rp,lp;
	Vertex rv, lv;
	Face curFace;
	
	while (true) {
		if (circleVertex.valid()) {
			cv = mp2kp(circleVertex.point());
			
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
				Point_2 myLP(mp2kp(myLv.point())), myRP(mp2kp(myRv.point()));
				
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
			
			Point_2 sp(mp2kp(sv.point()));
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
uint32_t Triangulation::locate(const Point & target, uint32_t hint, T_GEOMETRY_TRAITS traits) const {
	return traverse(target, hint, [](Face const &) {}, traits);
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


template<typename T_CGAL_TRIANGULATION_DATA_STRUCTURE, typename T_VERTEX_TO_VERTEX_ID_MAP, typename T_FACE_TO_FACE_ID_MAP>
sserialize::UByteArrayAdapter &
Triangulation::append(T_CGAL_TRIANGULATION_DATA_STRUCTURE & src, T_FACE_TO_FACE_ID_MAP & faceToFaceId, T_VERTEX_TO_VERTEX_ID_MAP & vertexToVertexId, sserialize::UByteArrayAdapter & dest) {
	typedef T_CGAL_TRIANGULATION_DATA_STRUCTURE TDS;
	typedef typename TDS::Face_handle Face_handle;
	typedef typename TDS::Vertex_handle Vertex_handle;
	typedef typename TDS::Finite_faces_iterator Finite_faces_iterator;
	typedef typename TDS::Finite_vertices_iterator Finite_vertices_iterator;
	typedef typename TDS::Face_circulator Face_circulator;
	typedef typename TDS::Point Point;
	
	detail::Triangulation::Convert<Point, Triangulation::Point> kp2mp;
	
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
	
	for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
		faceToFaceId[fh] = faceId;
		++faceId;
	}
	faceCount = faceId;
	
	dest.putUint8(2);//VERSION
	{ //put the points
		Triangulation::Point gp;
		sserialize::Static::ArrayCreator<sserialize::spatial::GeoPoint> va(dest);
		va.reserveOffsets(src.number_of_vertices());
		for(Finite_vertices_iterator vt(src.finite_vertices_begin()), vtEnd(src.finite_vertices_end()); vt != vtEnd; ++vt) {
			gp = kp2mp(vt->point());
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
				detail::Triangulation::IntPoint<Point> ip0(p0), ip1(p1), ip2(p2);
				if (ip0 == ip1 || ip0 == ip2 || ip1 == ip2) {
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