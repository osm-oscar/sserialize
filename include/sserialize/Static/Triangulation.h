#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/GeoPoint.h>
#include <assert.h>
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_VERSION 1

#include <CGAL/number_utils.h>

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
		typedef enum {
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
		bool isNeighbor(uint8_t pos) const;
		uint32_t neighborId(uint8_t pos) const;
		Face neighbor(uint32_t pos) const;
		uint32_t vertexId(uint32_t pos) const;
		Vertex vertex(uint32_t pos) const;
		Point point(uint32_t pos) const;
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
	template<typename T_GEOMETRY_TRAITS>
	uint32_t locate(double lat, double lon, uint32_t hint = 0) const;
	template<typename T_CGAL_TRIANGULATION_DATA_STRUCTURE, typename T_VERTEX_TO_VERTEX_ID_MAP, typename T_FACE_TO_FACE_ID_MAP>
	static sserialize::UByteArrayAdapter & append(T_CGAL_TRIANGULATION_DATA_STRUCTURE & src, T_FACE_TO_FACE_ID_MAP & faceToFaceId, T_VERTEX_TO_VERTEX_ID_MAP & vertexToVertexId, sserialize::UByteArrayAdapter & dest);
	bool selfCheck() const;
	void printStats(std::ostream & out) const;
	//counter-clock-wise next vertex/neighbor as defined in cgal
	inline static int ccw(const int i) { return (i+1)%3; }
	//clock-wise next vertex/neighbor as defined in cgal
	inline static int cw(const int i) { return (i+2)%3; }
};

template<typename T_GEOMETRY_TRAITS>
uint32_t Triangulation::locate(double /*lat*/, double /*lon*/, uint32_t /*hint*/) const {
	return NullFace;
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
		bitConfig[Triangulation::Vertex::VI_FACES_BEGIN] = sserialize::CompactUintArray::minStorageBits(faceCount);
		bitConfig[Triangulation::Vertex::VI_FACES_END] = bitConfig[Triangulation::Vertex::VI_FACES_BEGIN];
		sserialize::MultiVarBitArrayCreator va(bitConfig, dest);
		for(Finite_vertices_iterator vt(src.finite_vertices_begin()), vtEnd(src.finite_vertices_end()); vt != vtEnd; ++vt) {
			assert(vertexToVertexId.is_defined(vt));
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
			assert(!src.is_infinite(fc));
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
			
			assert(!src.is_infinite(fcBegin));
			assert(!src.is_infinite(fcEnd));
			assert(faceToFaceId.is_defined(fcBegin));
			assert(faceToFaceId.is_defined(fcEnd));
			
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
		bitConfig[Triangulation::Face::FI_NEIGHBOR0] = sserialize::CompactUintArray::minStorageBits(faceCount);
		bitConfig[Triangulation::Face::FI_NEIGHBOR1] = bitConfig[Triangulation::Face::FI_NEIGHBOR0];
		bitConfig[Triangulation::Face::FI_NEIGHBOR2] = bitConfig[Triangulation::Face::FI_NEIGHBOR0];
		bitConfig[Triangulation::Face::FI_VERTEX0] = sserialize::CompactUintArray::minStorageBits(vertexCount);
		bitConfig[Triangulation::Face::FI_VERTEX1] = bitConfig[Triangulation::Face::FI_VERTEX0];
		bitConfig[Triangulation::Face::FI_VERTEX2] = bitConfig[Triangulation::Face::FI_VERTEX0];
		sserialize::MultiVarBitArrayCreator fa(bitConfig, dest);
	
		faceId = 0;
		for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
			assert(faceToFaceId.is_defined(fh) && faceToFaceId[fh] == faceId);
			
			uint8_t validNeighbors = 0;
			for(int j(0); j < 3; ++j) {
				Face_handle nfh = fh->neighbor(j);
				uint32_t nfhId = 0xFFFFFFFF;
				if (faceToFaceId.is_defined(nfh)) {
					nfhId = faceToFaceId[nfh];
					validNeighbors |= static_cast<uint8_t>(1) << j;
				}
				fa.set(faceId, Triangulation::Face::FI_NEIGHBOR_BEGIN+j, nfhId);
			}
			fa.set(faceId, Triangulation::Face::FI_NEIGHBOR_VALID, validNeighbors);
			
			for(int j(0); j < 3; ++j) {
				Vertex_handle vh = fh->vertex(j);
				assert(vertexToVertexId.is_defined(vh));
				uint32_t vertexId = vertexToVertexId[vh];
				fa.set(faceId, Triangulation::Face::FI_VERTEX_BEGIN+j, vertexId);
			}
			++faceId;
		}
		fa.flush();
		assert(faceId == faceCount);
	}
	return dest;
}


}}} //end namespace

#endif