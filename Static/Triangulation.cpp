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

void Triangulation::Face::dump(std::ostream& out) const {
	out << "Triangulation::Face {\n";
	out << "\tid=" << id() << "\n";
	out << "\tneighbor_valid=" << m_p->faceInfo().at(m_pos, FI_NEIGHBOR_VALID) << "\n";
	for(int j(0); j < 3; ++j) {
		out << "\tneighbor[" << j << "]=" << neighborId(j)  << "\n";
	}
	for(int j(0); j < 3; ++j) {
		out << "\tvertex[" << j << "]=" << vertexId(j)  << "\n";
	}
	for(int j(0); j < 3; ++j) {
		out << "\\point[" << j << "]=" << vertex(j).point()  << "\n";
	}
	out << "}";
}

void Triangulation::Face::dump() const {
	dump(std::cout);
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
	assert(selfCheck());
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

bool Triangulation::selfCheck() const {
	//check face-neigbor relations
	for(uint32_t i(0), s(faceCount()); i < s; ++i) {
		Face f(face(i));
		for(int j(0); j < 3; ++j) {
			if (!f.isNeighbor(j)) {
				continue;
			}
			Face fn(f.neighbor(j));
			bool ok = false;
			for(int k(0); k < 3; ++k) {
				if (fn.neighborId(k) == i) {
					ok = true;
				}
			}
			if (!ok) {
				return false;
			}
		}
	}
	//now check face-edge-face-neighbor relations. see http://doc.cgal.org/latest/Triangulation_2/ for the spec
	//The three vertices of a face are indexed with 0, 1 and 2 in counterclockwise order.
	//The neighbors of a face are also indexed with 0,1,2 in such a way that the neighbor indexed by i is opposite to the vertex with the same index.
	//See Figure 36.2, the functions ccw(i) and cw(i) shown on this figure compute respectively i+1 and i−1 modulo 3
	for(uint32_t faceId(0), s(faceCount()); faceId < s; ++faceId) {
		Face f(face(faceId));
		for(int j(0); j < 3; ++j) {
			Vertex vs(f.vertex(j)), ve(f.vertex(cw(j)));
			if (f.isNeighbor(ccw(j))) {
				Face fn(f.neighbor(ccw(j)));
				for(int k(0); k < 3; ++k) {
					if (fn.vertexId(k) == vs.id()) {
						bool ok = false;
						if (fn.vertexId(cw(k)) == ve.id()) {
							ok = fn.neighborId(ccw(k)) == f.id();
						}
						else if (fn.vertexId(ccw(k)) == ve.id()) {
							ok = fn.neighborId(cw(k)) == f.id();
						}
						if (!ok) {
							return false;
						}
					}
				}
			}
		}
	}
	return true;
}


}}}//end namespace