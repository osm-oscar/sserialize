#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/GeoPoint.h>

namespace sserialize {
namespace Static {

class Triangulation {
private:
	typedef sserialize::MultiVarBitArray FaceInfo;
public:
	///A Face has up to 3 neighbors
	class Face {
	private:
		FaceInfo m_d;
	private:
		Face(const FaceInfo & fi, uint32_t pos);
	public:
		Face();
		uint32_t isNeighbor(uint32_t pos) const;
		Face neighbor(uint32_t pos) const;
		uint32_t vertex(uint32_t pos) const;
	};
	typedef sserialize::Static::spatial::GeoPoint Vertex;
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
	inline Vertex vertex(uint32_t pos) const { return m_v.at(pos);}
};


}}

#endif