#include <sserialize/Static/Triangulation.h>
#include <sserialize/utility/printers.h>

//CGAL
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Triangulation_euclidean_traits_2.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/Unique_hash_map.h>

#include "TestBase.h"

typedef CGAL::Exact_predicates_exact_constructions_kernel K;
typedef CGAL::Triangulation_vertex_base_2<K> Vb;
typedef CGAL::Triangulation_face_base_2<K> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb,Fb> TDS;
typedef CGAL::Delaunay_triangulation_2<K, TDS> CDT;
typedef CDT CGALTriangulation;

CGALTriangulation::Point centroid(const CGALTriangulation::Face_handle & fh) {
	return CGAL::centroid(fh->vertex(0)->point(), fh->vertex(1)->point(), fh->vertex(2)->point());
}


template<uint32_t NUM_TRIANG_POINTS, uint32_t NUM_TEST_POINTS>
class TriangulationTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( TriangulationTest );
CPPUNIT_TEST( staticSelfCheck );
CPPUNIT_TEST( testSerialization );
CPPUNIT_TEST( testLocateInStartFace );
CPPUNIT_TEST( testLocateVertexOfStartFace );
CPPUNIT_TEST( testLocateFaceCentroidsFromNeighbors );
CPPUNIT_TEST( testLocateFaceCentroids );
CPPUNIT_TEST( testLocateVertices );
CPPUNIT_TEST( testLocate );
CPPUNIT_TEST_SUITE_END();
private:
	double m_discDiameter;
	std::vector<CGALTriangulation::Point> m_pts;
	CGALTriangulation m_ctr;
	sserialize::UByteArrayAdapter m_strData;
	sserialize::Static::spatial::Triangulation m_str;
	CGAL::Unique_hash_map<CGALTriangulation::Face_handle, uint32_t> m_face2FaceId;
	CGAL::Unique_hash_map<CGALTriangulation::Vertex_handle, uint32_t> m_vertex2VertexId;
public:
	TriangulationTest() : m_discDiameter(30.0) {}
	virtual void setUp() {
		typedef CGALTriangulation::Point Point;
		typedef CGAL::Creator_uniform_2<double,Point> Creator;
		m_pts.reserve(NUM_TRIANG_POINTS);
		CGAL::Random_points_in_disc_2<Point, Creator> g(m_discDiameter);
		CGAL::cpp11::copy_n(g, NUM_TRIANG_POINTS, std::back_inserter(m_pts));
		
		m_ctr.insert(m_pts.begin(), m_pts.end());
		

		m_strData = sserialize::UByteArrayAdapter::createCache(4, sserialize::MM_PROGRAM_MEMORY);
		sserialize::Static::spatial::Triangulation::append(m_ctr, m_face2FaceId, m_vertex2VertexId, m_strData);
		m_str = sserialize::Static::spatial::Triangulation(m_strData);
	}
	virtual void tearDown() {}
	void staticSelfCheck() {
		CPPUNIT_ASSERT(m_str.selfCheck());
	}
	void testSerialization() {
		uint32_t numFiniteFaces = 0;
		uint32_t numFiniteVertices = 0;
		for(CGALTriangulation::Finite_faces_iterator fIt(m_ctr.finite_faces_begin()), fEnd(m_ctr.finite_faces_end()); fIt != fEnd; ++fIt) {
			++numFiniteFaces;
		}
		for(CGALTriangulation::Finite_vertices_iterator vIt(m_ctr.finite_vertices_begin()), vEnd(m_ctr.finite_vertices_end()); vIt != vEnd; ++vIt) {
			++numFiniteVertices;
		}
		CPPUNIT_ASSERT_EQUAL_MESSAGE("faceCount", numFiniteFaces, m_str.faceCount());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("vertexCount", numFiniteVertices, m_str.vertexCount());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("sizeInBytes", m_strData.size(), m_str.getSizeInBytes());
	
		for(CGALTriangulation::Finite_faces_iterator fIt(m_ctr.finite_faces_begin()), fEnd(m_ctr.finite_faces_end()); fIt != fEnd; ++fIt) {
			CGALTriangulation::Face_handle fh = fIt;
			CPPUNIT_ASSERT(m_face2FaceId.is_defined(fIt));
			uint32_t fId = m_face2FaceId[fIt];
			sserialize::Static::spatial::Triangulation::Face sf = m_str.face(fId);
			//check the faces
			for(int j(0); j < 3; ++j) {
				CGALTriangulation::Face_handle nfh = fh->neighbor(j);
				CPPUNIT_ASSERT(m_face2FaceId.is_defined(nfh) || m_ctr.is_infinite(nfh));
				uint32_t nfId = (m_face2FaceId.is_defined(nfh) ? m_face2FaceId[nfh] : sserialize::Static::spatial::Triangulation::NullFace);
				CPPUNIT_ASSERT_EQUAL(nfId, sf.neighborId(j));
			}
			//check the vertices
			for(int j(0); j < 3; ++j) {
				CGALTriangulation::Vertex_handle vh = fh->vertex(j);
				CPPUNIT_ASSERT(m_vertex2VertexId.is_defined(vh));
				CPPUNIT_ASSERT_EQUAL(m_vertex2VertexId[vh], sf.vertexId(j));
			}
		}
		std::unordered_set<uint32_t> cgFaces, sfFaces;
		//check vertex circulators
		for(CGALTriangulation::Finite_vertices_iterator vIt(m_ctr.finite_vertices_begin()), vEnd(m_ctr.finite_vertices_end()); vIt != vEnd; ++vIt) {
			CPPUNIT_ASSERT(m_vertex2VertexId.is_defined(vIt));
			uint32_t vId = m_vertex2VertexId[vIt];
			sserialize::Static::spatial::Triangulation::Vertex sv(m_str.vertex(vId));
			cgFaces.clear();
			sfFaces.clear();
			{
				sserialize::Static::spatial::Triangulation::FaceCirculator fcBegin(sv.facesBegin()), fcEnd(sv.facesEnd());
				while (true) {
					sfFaces.insert(fcBegin.face().id());
					if (fcBegin == fcEnd) {
						break;
					}
					else {
						++fcBegin;
					}
				}
			}
			{
				CGALTriangulation::Face_circulator fc(m_ctr.incident_faces(vIt));
				CGALTriangulation::Face_circulator fcIt(fc);
				CPPUNIT_ASSERT(m_face2FaceId.is_defined(fcIt) || m_ctr.is_infinite(fcIt));
				if (m_face2FaceId.is_defined(fcIt)) {
					cgFaces.insert(m_face2FaceId[fcIt]);
				}
				for(++fcIt; fcIt != fc; ++fcIt) {
					CPPUNIT_ASSERT(m_face2FaceId.is_defined(fcIt) || m_ctr.is_infinite(fcIt));
					if (m_face2FaceId.is_defined(fcIt)) {
						cgFaces.insert(m_face2FaceId[fcIt]);
					}
				}
				
			}
			CPPUNIT_ASSERT_EQUAL(cgFaces, sfFaces);
		}
	}
	
	void testLocateInStartFace() {
		for(uint32_t faceId(0), s(m_str.faceCount()); faceId < s; ++faceId) {
			sserialize::Static::spatial::Triangulation::Point ct(m_str.face(faceId).centroid());
			uint32_t sfId = m_str.locate<K>(ct.lat(), ct.lon(), faceId);
			CPPUNIT_ASSERT_EQUAL(faceId, sfId);
		}
	}
	
	void testLocateVertexOfStartFace() {
		for(uint32_t vertexId(0), s(m_str.vertexCount()); vertexId < s; ++vertexId) {
			sserialize::Static::spatial::Triangulation::Vertex v(m_str.vertex(vertexId));
			sserialize::Static::spatial::Triangulation::Point vp(v.point());
			uint32_t fId = m_str.locate<K>(vp.lat(), vp.lon(), v.facesBegin().face().id());
			CPPUNIT_ASSERT(m_str.face(fId).index(v) != -1);
		}
	}
	
	void testLocateFaceCentroidsFromNeighbors() {
		for(uint32_t targetFaceId(0), s(m_str.faceCount()); targetFaceId < s; ++targetFaceId) {
			sserialize::Static::spatial::Triangulation::Face f(m_str.face(targetFaceId));
			sserialize::Static::spatial::Triangulation::Point ct(f.centroid());
			for(int j(0); j < 3; ++j) {
				if (f.isNeighbor(j)) {
					uint32_t lId = m_str.locate<K>(ct.lat(), ct.lon(), f.neighborId(j));
					if (lId != targetFaceId) {
						lId = m_str.locate<K>(ct.lat(), ct.lon(), f.neighborId(j));
					}
					CPPUNIT_ASSERT_EQUAL(targetFaceId, lId);
				}
			}
		}
	}
	
	void testLocateFaceCentroids() {
		for(uint32_t targetFaceId(0), s(m_str.faceCount()); targetFaceId < s; ++targetFaceId) {
			sserialize::Static::spatial::Triangulation::Face f(m_str.face(targetFaceId));
			sserialize::Static::spatial::Triangulation::Point ct(f.centroid());
			for(uint32_t startFaceId(0); startFaceId < s; ++startFaceId) {
				uint32_t lId = m_str.locate<K>(ct.lat(), ct.lon(), startFaceId);
				CPPUNIT_ASSERT_EQUAL(targetFaceId, lId);
			}
		}
	}
	
	void testLocateVertices() {
		for(uint32_t vertexId(0), s(m_str.vertexCount()); vertexId < s; ++vertexId) {
			sserialize::Static::spatial::Triangulation::Vertex v(m_str.vertex(vertexId));
			sserialize::Static::spatial::Triangulation::Point vp(v.point());
			for(uint32_t startFaceId(0), faceCount(m_str.faceCount()); startFaceId < faceCount; ++startFaceId) {
				uint32_t fId = m_str.locate<K>(vp.lat(), vp.lon(), startFaceId);
				CPPUNIT_ASSERT(m_str.face(fId).index(v) != -1);
			}
		}
	}
	
	void testLocate() {
		typedef CGALTriangulation::Point Point;
		typedef CGAL::Creator_uniform_2<double,Point> Creator;
		std::vector<Point> testPoints;
		testPoints.reserve(NUM_TEST_POINTS);
		CGAL::Random_points_in_disc_2<Point, Creator> g(m_discDiameter);
		CGAL::cpp11::copy_n(g, NUM_TEST_POINTS, std::back_inserter(testPoints));
		
		for(uint32_t i(0), s((uint32_t) testPoints.size()); i < s; ++i) {
			double x = CGAL::to_double(testPoints[i].x());
			double y = CGAL::to_double(testPoints[i].x());
			CGALTriangulation::Face_handle fh = m_ctr.locate(Point(x, y));
			uint32_t fhId;
			if (m_face2FaceId.is_defined(fh)) {
				fhId = m_face2FaceId[fh];
			}
			else {
				CPPUNIT_ASSERT(!m_ctr.number_of_faces() || m_ctr.is_infinite(fh));
				fhId = sserialize::Static::spatial::Triangulation::NullFace;
			}
			uint32_t sfId = m_str.locate<K>(x, y);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("faces at ", i), fhId, sfId);
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TriangulationTest<0, 10>::suite() );
	runner.addTest(  TriangulationTest<1000, 1000>::suite() );
	runner.addTest(  TriangulationTest<10000, 1000>::suite() );
// 	runner.eventManager().popProtector();
	bool ok = runner.run();
	return ok ? 0 : 1;
}