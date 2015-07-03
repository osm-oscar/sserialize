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
  *   VERTICES sserialize::Array<GeoPoint> gp
  *   FACES    sserialize::MultiVarBitArray
  *   {
  *      ValidNeighbor u3
  *      Neighbors 3*?
  *      Vertices 3*?
  *   }
  * }
  * Changelog:
  */

///Triangulation of the convex-hull of a set of points
class Triangulation final {
public:
	typedef sserialize::spatial::GeoPoint Point;
	struct Geom_traits {
		typedef Point Point_2;
	};

	class Face;
	class Vertex;
	
	class Vertex final {
	private:
		friend class Triangulation;
		friend class Face;
	private:
		const Triangulation * m_p;
		uint32_t m_pos;
	private:
		Vertex(const Triangulation * p, uint32_t pos);
	public:
		Vertex();
		~Vertex();
		Point point() const;
	};

	///A Face has up to 3 neighbors
	class Face final {
	private:
		friend class Triangulation;
		friend class Vertex;
	private:
		typedef enum {
			FI_NEIGHBOR_VALID=0,
			FI_NEIGHBOR0=1, FI_NEIGHBOR1=2, FI_NEIGHBOR2=3, FI_NEIGHBOR_BEGIN=FI_NEIGHBOR0, FI_NEIGHBOR_END=FI_NEIGHBOR2+1,
			FI_VERTEX0=4, FI_VERTEX1=5, FI_VERTEX2=6, FI_VERTEX_BEGIN=FI_VERTEX0, FI_VERTEX_END=FI_VERTEX2+1,
			FI__NUMBER_ENTRIES=FI_VERTEX_END+1
		} FaceInfo;
	private:
		const Triangulation * m_p;
		uint32_t m_pos;
	private:
		Face(const Triangulation * p, uint32_t pos);
	public:
		Face();
		~Face();
		bool valid() const;
		bool isNeighbor(uint8_t pos) const;
		uint32_t neighborId(uint8_t pos) const;
		Face neighbor(uint32_t pos) const;
		uint32_t vertexId(uint32_t pos) const;
		Vertex vertex(uint32_t pos) const;
	};
	typedef sserialize::Static::spatial::GeoPoint Point_2;
public:
	static constexpr uint32_t NullFace = 0xFFFFFFFF;
	static constexpr uint32_t NullVertex = 0xFFFFFFFF;
private:
	typedef sserialize::MultiVarBitArray FaceInfos;
	typedef sserialize::Static::Array<Point> VertexInfos;
private:
	VertexInfos m_v;
	FaceInfos m_fi;
protected:
	inline const FaceInfos & faceInfo() const { return m_fi; }
	inline const VertexInfos & vertexInfo() const { return m_v; }
public:
	Triangulation();
	Triangulation(const sserialize::UByteArrayAdapter & d);
	~Triangulation();
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t vertexCount() const { return m_v.size(); }
	uint32_t faceCount() const { return m_fi.size(); }
	Face face(uint32_t pos) const;
	Vertex vertex(uint32_t pos) const;
	template<typename T_GEOMETRY_TRAITS>
	uint32_t locate(double lat, double lon, uint32_t hint = 0) const;
	template<typename T_CGAL_TRIANGULATION_DATA_STRUCTURE, typename T_VERTEX_TO_VERTEX_ID_MAP, typename T_FACE_TO_FACE_ID_MAP>
	static sserialize::UByteArrayAdapter & append(T_CGAL_TRIANGULATION_DATA_STRUCTURE & src, T_FACE_TO_FACE_ID_MAP & faceToFaceId, T_VERTEX_TO_VERTEX_ID_MAP & vertexToVertexId, sserialize::UByteArrayAdapter & dest);
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
	
	faceToFaceId.clear();
	vertexToVertexId.clear();
	
	uint32_t faceId = 0;
	uint32_t vertexId = 0;
	
	for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
		faceToFaceId[fh] = faceId;
		++faceId;
	}
	
	dest.putUint8(1);//VERSION
	{ //put the vertices
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
	}
	{
		std::vector<uint8_t> bitConfig(Triangulation::Face::FI__NUMBER_ENTRIES);
		bitConfig[Triangulation::Face::FI_NEIGHBOR_VALID] = 3;
		bitConfig[Triangulation::Face::FI_NEIGHBOR0] = sserialize::CompactUintArray::minStorageBits(faceId);
		bitConfig[Triangulation::Face::FI_NEIGHBOR1] = bitConfig[Triangulation::Face::FI_NEIGHBOR0];
		bitConfig[Triangulation::Face::FI_NEIGHBOR2] = bitConfig[Triangulation::Face::FI_NEIGHBOR0];
		bitConfig[Triangulation::Face::FI_VERTEX0] = sserialize::CompactUintArray::minStorageBits(vertexId);
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
				fa.set(faceId, Triangulation::Face::FI_VERTEX_BEGIN+j, nfhId);
			}
			fa.set(faceId, Triangulation::Face::FI_NEIGHBOR_VALID, validNeighbors);
			
			for(int j(0); j < 3; ++j) {
				Vertex_handle vh = fh->vertex(0);
				assert(vertexToVertexId.is_defined(vh));
				fa.set(faceId, Triangulation::Face::FI_VERTEX_BEGIN+j, vertexToVertexId[vh]);
			}
			++faceId;
		}
		fa.flush();
	}
	return dest;
}


}}} //end namespace

#endif