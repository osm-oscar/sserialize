#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/GeoPoint.h>

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>

namespace sserialize {
namespace Static {

class Triangulation;

namespace detail {
namespace Triangulation {

}}//end namespace detail::Triangulation

///Triangulation of the convex-hull of a set of points
class Triangulation {
private:
	typedef sserialize::MultiVarBitArray FaceInfo;
public:
	typedef Triangulation Triangulation;
	struct Geom_traits {
		typedef sserialize::spatial::GeoPoint Point_2;
	};

	///A Face has up to 3 neighbors
	class Face {
	private:
		FaceInfo m_d;
	private:
		Face(const FaceInfo & fi, uint32_t pos);
	public:
		Face();
		virtual ~Face();
		bool isNeighbor(uint32_t pos) const;
		Face neighbor(uint32_t pos) const;
		uint32_t vertex(uint32_t pos) const;
	};
	typedef sserialize::Static::spatial::GeoPoint Point_2;

	class Finite_vertices_iterator {
	private:
		Triangulation * m_d;
		uint32_t m_pos;
	public:
		Finite_vertices_iterator();
		Finite_vertices_iterator(Triangulation * d, uint32_t pos);
		~Finite_vertices_iterator();
		Finite_vertices_iterator & operator++();
		Finite_vertices_iterator operator++(int);
		inline Finite_vertices_iterator & operator*() { return *this; }
		inline Finite_vertices_iterator & operator->() { return *this; }
		bool isNeighbor(uint32_t pos);
		Finite_vertices_iterator neighbor();
	};
	
	class Finite_faces_iterator {
	
	};
	typedef Finite_faces_iterator Face_handle;
	typedef Finite_vertices_iterator Vertex_handle;
public:
	static constexpr uint32_t NullFace = 0xFFFFFFFF;
private:
	FaceInfo m_fi;
	sserialize::Static::Array<Vertex> m_v;
public:
	Triangulation() {}
	Triangulation(const sserialize::UByteArrayAdapter & d);
	virtual ~Triangulation() {}
	uint32_t vertexCount() const { return m_v.size(); }
	uint32_t faceCount() const { return m_fi.size(); }
	Face face(uint32_t pos) const;
	template<typename T_GEOMETRY_TRAITS = CGAL::Exact_predicates_exact_constructions_kernel>
	uint32_t locate(double lat, double lon, uint32_t hint = 0) const;
	inline Vertex_handle vertex(uint32_t pos) const { return m_v.at(pos);}
	Finite_vertices_iterator finite_vertices_begin() const;
	Finite_vertices_iterator finite_vertices_end() const;
	Finite_faces_iterator finite_faces_begin() const;
	Finite_faces_iterator finite_faces_end() const;
};

template<typename T_GEOMETRY_TRAITS = CGAL::Exact_predicates_exact_constructions_kernel>
uint32_t locate(double lat, double lon, uint32_t hint = 0) const {
	
}



}} //end namespace

#endif