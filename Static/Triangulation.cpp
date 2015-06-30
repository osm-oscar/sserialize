#include <sserialize/Static/Triangulation.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {
namespace spatial {

constexpr uint32_t Triangulation::NullFace;
constexpr uint32_t Triangulation::NullVertex;

//inner classes
Triangulation::Face::Face() :
Face(0, Triangulation::NullFace)
{}

Triangulation::Face::Face(const Triangulation* p, uint32_t pos) :
m_p(p),
m_pos(pos)
{}

Triangulation::Face::~Face() {}

bool Triangulation::Face::valid() const {
	return m_p;
}

bool Triangulation::Face::isNeighbor(uint8_t pos) const {
	return (m_p->faceInfo().at(m_pos, FI_NEIGHBOR_VALID) & (static_cast<uint8_t>(1) << pos));
}

uint32_t Triangulation::Face::neighborId(uint8_t pos) const {
	if (!isNeighbor(pos)) {
		return Triangulation::NullFace;
	}
	return m_p->faceInfo().at(m_pos, FI_NEIGHBOR_BEGIN+pos);
}

Triangulation::Face Triangulation::Face::neighbor(uint32_t pos) const {
	uint32_t nId = neighborId(pos);
	if (nId != Triangulation::NullFace) {
		return Face(m_p, nId);
	}
	return Face();
}

uint32_t Triangulation::Face::vertexId(uint32_t pos) const {
	if (pos > 2) {
		throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::Face::vertexId");
	}
	return m_p->faceInfo().at(m_pos, FI_VERTEX_BEGIN+pos);
}

Triangulation::Vertex Triangulation::Face::vertex(uint32_t pos) const {
	return Vertex(m_p, vertexId(pos));
}

//Vertex definitions

Triangulation::Vertex::Vertex() :
Vertex(0, Triangulation::NullVertex)
{}


Triangulation::Vertex::Vertex(const Triangulation* p, uint32_t pos) :
m_p(p),
m_pos(pos)
{}

Triangulation::Vertex::~Vertex() {}

Triangulation::Point Triangulation::Vertex::point() const {
	return m_p->vertexInfo().at(m_pos);
}

Triangulation::Triangulation() {}

Triangulation::Triangulation(const UByteArrayAdapter& d) :
m_v(d+1),
m_fi(d+(1+m_v.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_VERSION, d.at(0), "sserialize::Static::spatial::Triangulation::Triangulation");
}

Triangulation::~Triangulation() {}

UByteArrayAdapter::OffsetType Triangulation::getSizeInBytes() const {
	return 1+m_v.getSizeInBytes()+m_fi.getSizeInBytes();
}

Triangulation::Face Triangulation::face(uint32_t pos) const {
	if (pos >= faceCount()) {
		throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::face");
	}
	return Face(this, pos); 
}

Triangulation::Vertex Triangulation::vertex(uint32_t pos) const {
	if (pos >= vertexCount()) {
		throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::vertex");
	}
	return Vertex(this, pos); 
}


}}}//end namespace