#include <sserialize/Static/Triangulation.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/printers.h>
#include <sserialize/utility/assert.h>
#include <sserialize/spatial/GeoPolygon.h>


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
	return m_p && id() != Triangulation::NullFace;
}

bool Triangulation::Face::isDegenerate() const {
	return m_p->faceInfo().at(m_pos, FI_IS_DEGENERATE);
}

bool Triangulation::Face::isNeighbor(uint32_t pos) const {
	return (m_p->faceInfo().at(m_pos, FI_NEIGHBOR_VALID) & (static_cast<uint8_t>(1) << pos));
}

uint32_t Triangulation::Face::neighborId(uint32_t pos) const {
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

uint32_t Triangulation::Face::vertexId(const Point & p) const {
	for(uint32_t i(0); i < 3; ++i) {
		if (p.equal(point(i), 0)) {
			return i;
		}
	}
	throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::Face::vertexId");
}

Triangulation::Vertex Triangulation::Face::vertex(uint32_t pos) const {
	return Vertex(m_p, vertexId(pos));
}

Triangulation::Vertex Triangulation::Face::vertex(const Point & p) const {
	return vertex( vertexId(p) );
}

Triangulation::Point Triangulation::Face::point(uint32_t pos) const {
	return m_p->points().at(vertexId(pos));
}

bool Triangulation::Face::isVertex(const Point & p) const {
	for(uint32_t i(0); i < 3; ++i) {
		if (p.equal(point(i), 0)) {
			return true;
		}
	}
	return false;
}

bool Triangulation::Face::contains(const Triangulation::Point & p) const {
	detail::Triangulation::Orientation<Triangulation::Point> ot;
	CGAL::Sign ot0cw = ot(point(0), point(Triangulation::cw(0)), p);
	CGAL::Sign ot0ccw = ot(point(0), point(Triangulation::ccw(0)), p);
	CGAL::Sign otcwccw = ot(point(Triangulation::cw(0)), point(Triangulation::ccw(0)), p);
	//CGAL::_TURN != x is the same as (CGAL::OPPOSITE_TURN == x || CGAL::COLLINEAR == x)
	//strongly inside is:
// 	(CGAL::RIGHT_TURN == ot0cw && CGAL::LEFT_TURN == ot0ccw && CGAL::RIGHT == otcwccw) 
	return (CGAL::LEFT_TURN != ot0cw && CGAL::LEFT_TURN == ot0ccw && CGAL::RIGHT_TURN == otcwccw) ||
			(CGAL::RIGHT_TURN == ot0cw && CGAL::RIGHT_TURN != ot0ccw && CGAL::RIGHT_TURN == otcwccw) ||
			(CGAL::RIGHT_TURN == ot0cw && CGAL::LEFT_TURN == ot0ccw && CGAL::LEFT_TURN != otcwccw) ||
			//bool -> int = (0|1); if 2 are collinear then p is at the corner of these two, the corner is a vertex of the face
			(((CGAL::COLLINEAR == ot0cw)+(CGAL::COLLINEAR == ot0ccw)+(CGAL::COLLINEAR == otcwccw)) == 2);
}

bool Triangulation::Face::intersects(const Triangulation::Point& p, const Triangulation::Point& q) const {
	detail::Triangulation::Do_intersect<Triangulation::Point> ci;
	return ci(p, q, point(0), point(1)) ||
			ci(p, q, point(1), point(2)) ||
			ci(p, q, point(2), point(0));
}

Triangulation::Point Triangulation::Face::centroid() const {
	detail::Triangulation::Compute_centroid<Triangulation::Point> cc;
	return cc(point(0), point(1), point(2) );
}

int Triangulation::Face::index(const Triangulation::Vertex& v) const {
	uint32_t vertexId = v.id();
	for(uint32_t j(0); j < 3; ++j) {
		if (this->vertexId(j) == vertexId) {
			return j;
		}
	}
	return -1;
}

bool Triangulation::Face::operator==(const Triangulation::Face& other) const {
	return m_pos == other.m_pos && m_p == other.m_p;
}

bool Triangulation::Face::operator!=(const Triangulation::Face& other) const {
	return m_pos != other.m_pos || m_p != other.m_p;
}

void Triangulation::Face::dump(std::ostream& out) const {
	out << "Triangulation::Face {\n";
	out << "\tid=" << id() << "\n";
	out << "\tneighbor_valid=" << m_p->faceInfo().at(m_pos, FI_NEIGHBOR_VALID) << "\n";
	for(uint32_t j(0); j < 3; ++j) {
		out << "\tneighbor[" << j << "]=";
		if (isNeighbor(j)) {
			out << neighborId(j);
		}
		else {
			out << "invalid";
		}
		out << "\n";
	}
	for(uint32_t j(0); j < 3; ++j) {
		out << "\tvertex[" << j << "]=" << vertexId(j)  << "\n";
	}
	for(uint32_t j(0); j < 3; ++j) {
		out << "\tpoint[" << j << "]=" << vertex(j).point()  << "\n";
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

bool Triangulation::Vertex::valid() const {
	return m_p && id() != Triangulation::NullVertex;
}

Triangulation::Point Triangulation::Vertex::point() const {
	return m_p->points().at(m_pos);
}

uint32_t Triangulation::Vertex::beginFaceId() const {
	return m_p->vertexInfos().at(m_pos, VI_FACES_BEGIN);
}

uint32_t Triangulation::Vertex::endFaceId() const {
	return m_p->vertexInfos().at(m_pos, VI_FACES_END);
}

Triangulation::Face Triangulation::Vertex::beginFace() const {
	return m_p->face(beginFaceId());
}

Triangulation::Face Triangulation::Vertex::endFace() const {
	return m_p->face(endFaceId());
}

void Triangulation::Vertex::dump(std::ostream& out) const {
	out << "Triangulation::Vertex {\n";
	out << "\tid=" << id() << "\n";
	out << "\tpoint=" << point() << "\n";
	out << "\tbeginFaceId=" << beginFaceId() << "\n";
	out << "\tendFaceId=" << endFaceId() << "\n";
	FaceCirculator fB(facesBegin()), fE(facesEnd());
	for(uint32_t i(0);fB != fE; ++fB, ++i) {
		out << "\tface[" << i << "]=" << fB.face().id() << std::endl;
	}
	out << "}";
}

void Triangulation::Vertex::dump() const {
	dump(std::cout);
}

Triangulation::FaceCirculator Triangulation::Vertex::facesBegin() const {
	return FaceCirculator(*this, beginFace());
}

Triangulation::FaceCirculator Triangulation::Vertex::facesEnd() const {
	return FaceCirculator(*this, endFace());
}

Triangulation::FaceCirculator Triangulation::Vertex::faces() const {
	return FaceCirculator(*this, m_p->face(m_p->vertexInfos().at(m_pos, VI_FACES_BEGIN)));
}

bool Triangulation::Vertex::operator!=(const Triangulation::Vertex& other) const {
	return m_pos != other.m_pos || m_p != other.m_p;
}

bool Triangulation::Vertex::operator==(const Triangulation::Vertex& other) const {
	return m_pos == other.m_pos && m_p == other.m_p;
}

Triangulation::FaceCirculator::FaceCirculator(const Triangulation::Vertex& v, const Triangulation::Face& f) :
m_f(f),
m_v(v)
{
	SSERIALIZE_NORMAL_ASSERT(m_f.index(m_v) != -1);
}

Triangulation::FaceCirculator::~FaceCirculator() {}

Triangulation::Face Triangulation::FaceCirculator::operator*() const {
	return m_f;
}

Triangulation::Face Triangulation::FaceCirculator::operator->() const {
	return m_f;
}

bool Triangulation::FaceCirculator::operator==(const Triangulation::FaceCirculator& other) const {
	return m_f == other.m_f && m_v == other.m_v;
}

bool Triangulation::FaceCirculator::operator!=(const Triangulation::FaceCirculator& other) const {
	return m_f != other.m_f || m_v != other.m_v;
}

Triangulation::FaceCirculator& Triangulation::FaceCirculator::operator++() {
	if (m_f.id() != m_v.endFaceId()) {
		int i = m_f.index(m_v);
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(i, 0);
		m_f = m_f.neighbor((uint32_t) Triangulation::ccw(i));
	}
	else {
		m_f = m_v.beginFace();
	}
	return *this;
}

Triangulation::FaceCirculator Triangulation::FaceCirculator::operator++(int) {
	FaceCirculator tmp(*this);
	this->operator++();
	return tmp;
}

Triangulation::FaceCirculator& Triangulation::FaceCirculator::operator--() {
	if (m_f.id() != m_v.beginFaceId()) {
		int i = m_f.index(m_v);
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(i, 0);
		m_f = m_f.neighbor((uint32_t)Triangulation::cw(i));
	}
	else {
		m_f = m_v.endFace();
	}
	return *this;
}

Triangulation::FaceCirculator Triangulation::FaceCirculator::operator--(int) {
	FaceCirculator tmp(*this);
	this->operator--();
	return tmp;
}

const Triangulation::Face& Triangulation::FaceCirculator::face() const {
	return m_f;
}

const Triangulation::Vertex& Triangulation::FaceCirculator::vertex() const {
	return m_v;
}

Triangulation::Triangulation() :
m_features(Triangulation::F_CLEAN_GEOMETRY)
{}

Triangulation::Triangulation(const UByteArrayAdapter& d) :
m_features(d.at(1)),
m_p(d+SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_FIXED_HEADER_SIZE),
m_vi(d+(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_FIXED_HEADER_SIZE+m_p.getSizeInBytes())),
m_fi(d+(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_FIXED_HEADER_SIZE+m_p.getSizeInBytes()+m_vi.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_VERSION, d.at(0), "sserialize::Static::spatial::Triangulation::Triangulation");
	SSERIALIZE_EQUAL_LENGTH_CHECK(m_p.size(), m_vi.size(), "sserialize::Static::spatial::Triangulation::Triangulation: m_vi != m_p");
	SSERIALIZE_VERY_EXPENSIVE_ASSERT(selfCheck());
}

Triangulation::~Triangulation() {}

UByteArrayAdapter::OffsetType Triangulation::getSizeInBytes() const {
	return SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_FIXED_HEADER_SIZE+m_vi.getSizeInBytes()+m_p.getSizeInBytes()+m_fi.getSizeInBytes();
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
	for(uint32_t faceId(0), s(faceCount()); faceId < s; ++faceId) {
		Face f(face(faceId));
		for(uint32_t j(0); j < 3; ++j) {
			if (!f.isNeighbor(j)) {
				continue;
			}
			Face fn(f.neighbor(j));
			bool ok = false;
			for(uint32_t k(0); k < 3; ++k) {
				if (fn.neighborId(k) == faceId) {
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
		for(uint32_t j(0); j < 3; ++j) {
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
	for(uint32_t faceId(0), s(faceCount()); faceId < s; ++faceId) {
		Face f(face(faceId));
		Vertex v0(f.vertex(0)), v1(f.vertex(1)), v2(f.vertex(2));
		using sserialize::spatial::equal;
		if (v0.id() == v1.id() || v0.id() == v2.id() || v1.id() == v2.id() ||
			equal(v0.point(), v1.point(), 0.0) || equal(v0.point(), v2.point(), 0.0) || equal(v1.point(), v2.point(), 0.0))
		{
			return false;
		}
	}
	for(uint32_t vertexId(0), s(vertexCount()); vertexId < s; ++vertexId) {
		Vertex v(vertex(vertexId));
		FaceCirculator fc(v.faces());
		FaceCirculator fcIt(fc);
		if (fcIt.face().index(v) == -1) {
			return false;
		}
		for(++fcIt; fcIt != fc; ++fcIt) {
			if (fcIt.face().index(v) == -1) {
				return false;
			}
		}
	}
	return true;
}


void Triangulation::printStats(std::ostream& out) const {
	out << "sserialize::Static::spatial::Triangulation::stats {\n";
	out << "\t#vertices=" << m_p.size() << "\n";
	out << "\t#faces=" << m_fi.size() << "\n";
	out << "\tTotal size=" << sserialize::prettyFormatSize(getSizeInBytes()) << "\n";
	out << "}";
}

uint32_t Triangulation::locate(const Point& target, uint32_t hint, TraversalType tt) const {
	if (!faceCount()) {
		return NullFace;
	}
	if (hint >= faceCount()) {
		hint = 0;
	}
	return traverse(target, face(hint).centroid(), [](Face const &) {}, hint, tt);
}

}}}//end namespace
