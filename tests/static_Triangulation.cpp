#include <sserialize/Static/Triangulation.h>
#include <sserialize/utility/printers.h>

//CGAL
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/Unique_hash_map.h>

#include "TestBase.h"

typedef CGAL::Exact_predicates_exact_constructions_kernel K;
typedef CGAL::Triangulation_vertex_base_2<K> Vb;
typedef CGAL::Constrained_triangulation_face_base_2<K> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb,Fb> TDS;
typedef CGAL::Constrained_triangulation_2<K, TDS> CDT;
typedef CDT CGALTriangulation;

CGALTriangulation::Point centroid(const CGALTriangulation::Face_handle & fh) {
	return CGAL::centroid(fh->vertex(0)->point(), fh->vertex(1)->point(), fh->vertex(2)->point());
}

using StaticTriangulation = sserialize::Static::spatial::Triangulation;

template<uint32_t NUM_TRIANG_POINTS, uint32_t NUM_TEST_POINTS, StaticTriangulation::GeometryCleanType T_GCT>
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
	struct FaceOp {
		void operator()(StaticTriangulation::Face const &){}
	};
private:
	double m_discDiameter;
	std::vector<CGALTriangulation::Point> m_pts;
	CGALTriangulation m_ctr;
	sserialize::UByteArrayAdapter m_strData;
	StaticTriangulation m_str;
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
		
		for(const Point & p : m_pts) {
			assert(sserialize::spatial::GeoPoint(CGAL::to_double(p.x()), CGAL::to_double(p.y())).valid());
		}
		
		m_ctr.insert(m_pts.begin(), m_pts.end());
		
		m_strData = sserialize::UByteArrayAdapter::createCache(4, sserialize::MM_PROGRAM_MEMORY);
		
		if (m_ctr.number_of_vertices() && T_GCT != StaticTriangulation::GCT_NONE) {
			StaticTriangulation::prepare(m_ctr, sserialize::Static::spatial::detail::Triangulation::PrintRemovedEdges(), T_GCT, 0.1);
		}
		StaticTriangulation::append(m_ctr, m_face2FaceId, m_vertex2VertexId, m_strData, T_GCT);
		m_str = StaticTriangulation(m_strData);
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
			StaticTriangulation::Face sf = m_str.face(fId);
			//check the faces
			for(int j(0); j < 3; ++j) {
				CGALTriangulation::Face_handle nfh = fh->neighbor(j);
				CPPUNIT_ASSERT(m_face2FaceId.is_defined(nfh) || m_ctr.is_infinite(nfh));
				uint32_t nfId = (m_face2FaceId.is_defined(nfh) ? m_face2FaceId[nfh] : StaticTriangulation::NullFace);
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
			StaticTriangulation::Vertex sv(m_str.vertex(vId));
			cgFaces.clear();
			sfFaces.clear();
			{
				StaticTriangulation::FaceCirculator fcBegin(sv.facesBegin()), fcEnd(sv.facesEnd());
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
			CPPUNIT_ASSERT(cgFaces == sfFaces);
		}
	}
	
	void testLocateInStartFace() {
		for(uint32_t faceId(0), s(m_str.faceCount()); faceId < s; ++faceId) {
			StaticTriangulation::Point ct(m_str.face(faceId).centroid());
			uint32_t zzfId = m_str.locate(ct, faceId, StaticTriangulation::TT_ZIG_ZAG);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("locate zigzag", faceId, zzfId);
			if (T_GCT == StaticTriangulation::GCT_SNAP_VERTICES) {
				uint32_t sfId = m_str.locate(ct, faceId, StaticTriangulation::TT_STRAIGHT);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("locate straight", faceId, sfId);
			}
		}
	}
	
	void testLocateVertexOfStartFace() {
		for(uint32_t vertexId(0), s(m_str.vertexCount()); vertexId < s; ++vertexId) {
			StaticTriangulation::Vertex v(m_str.vertex(vertexId));
			StaticTriangulation::Point vp(v.point());
			uint32_t zzfId = m_str.locate(vp, v.facesBegin().face().id(), StaticTriangulation::TT_ZIG_ZAG);
			CPPUNIT_ASSERT_MESSAGE("locate zigzag", m_str.face(zzfId).index(v) != -1);
			
			if (T_GCT == StaticTriangulation::GCT_SNAP_VERTICES) {
				uint32_t sfId = m_str.locate(vp, v.facesBegin().face().id(), StaticTriangulation::TT_ZIG_ZAG);
				CPPUNIT_ASSERT_MESSAGE("locate straight", m_str.face(sfId).index(v) != -1);
			}
		}
	}
	
	void testLocateFaceCentroidsFromNeighbors() {
		for(uint32_t targetFaceId(0), s(m_str.faceCount()); targetFaceId < s; ++targetFaceId) {
			StaticTriangulation::Face f(m_str.face(targetFaceId));
			StaticTriangulation::Point ct(f.centroid());
			for(int j(0); j < 3; ++j) {
				if (f.isNeighbor(j)) {
					uint32_t zzlId = m_str.locate(ct, f.neighborId(j), StaticTriangulation::TT_ZIG_ZAG);
					CPPUNIT_ASSERT_EQUAL_MESSAGE("locate zigzag", targetFaceId, zzlId);
					
					if (T_GCT == StaticTriangulation::GCT_SNAP_VERTICES) {
						uint32_t slId = m_str.locate(ct, f.neighborId(j), StaticTriangulation::TT_ZIG_ZAG);
						CPPUNIT_ASSERT_EQUAL_MESSAGE("locate straight", targetFaceId, slId);
					}
				}
			}
		}
	}
	
	void testLocateFaceCentroids() {
		for(uint32_t targetFaceId(0), s(m_str.faceCount()); targetFaceId < s; ++targetFaceId) {
			StaticTriangulation::Face f(m_str.face(targetFaceId));
			StaticTriangulation::Point ct(f.centroid());
			for(uint32_t startFaceId(0); startFaceId < s; ++startFaceId) {
				uint32_t zzlId = m_str.locate(ct, startFaceId, StaticTriangulation::TT_ZIG_ZAG);
				CPPUNIT_ASSERT_EQUAL_MESSAGE("locate zigzag", targetFaceId, zzlId);
				
				if (T_GCT == StaticTriangulation::GCT_SNAP_VERTICES) {
					uint32_t slId = m_str.locate(ct, startFaceId, StaticTriangulation::TT_ZIG_ZAG);
					CPPUNIT_ASSERT_EQUAL_MESSAGE("locate straight", targetFaceId, slId);
				}
			}
		}
	}
	
	void testLocateVertices() {
		for(uint32_t vertexId(0), s(m_str.vertexCount()); vertexId < s; ++vertexId) {
			StaticTriangulation::Vertex v(m_str.vertex(vertexId));
			StaticTriangulation::Point vp(v.point());
			for(uint32_t startFaceId(0), faceCount(m_str.faceCount()); startFaceId < faceCount; ++startFaceId) {
				uint32_t zzfId = m_str.locate(vp, startFaceId, StaticTriangulation::TT_ZIG_ZAG);
				CPPUNIT_ASSERT_MESSAGE("locate zigzag", m_str.face(zzfId).index(v) != -1);
				
				if (T_GCT == StaticTriangulation::GCT_SNAP_VERTICES) {
					uint32_t sfId = m_str.locate(vp, startFaceId, StaticTriangulation::TT_ZIG_ZAG);
					CPPUNIT_ASSERT_MESSAGE("locate straight", m_str.face(sfId).index(v) != -1);
				}
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
				fhId = StaticTriangulation::NullFace;
			}
			uint32_t zzfId = m_str.locate(sserialize::spatial::GeoPoint(x, y), m_str.NullFace, StaticTriangulation::TT_ZIG_ZAG);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("locate zigzag: faces at ", i), fhId, zzfId);
			
			if (T_GCT == StaticTriangulation::GCT_SNAP_VERTICES) {
				uint32_t sfId = m_str.locate(sserialize::spatial::GeoPoint(x, y), m_str.NullFace, StaticTriangulation::TT_STRAIGHT);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("locate straight: faces at ", i), fhId, sfId);
			}
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TriangulationTest<10, 10, StaticTriangulation::GCT_SNAP_VERTICES>::suite() );
	runner.addTest(  TriangulationTest<1000, 1000, StaticTriangulation::GCT_SNAP_VERTICES>::suite() );
	runner.addTest(  TriangulationTest<10000, 1000, StaticTriangulation::GCT_SNAP_VERTICES>::suite() );
	runner.eventManager().popProtector();
	bool ok = runner.run();
	return ok ? 0 : 1;
}
