#ifndef SSERIALIZE_STATIC_TRACF_GRAPH_H
#define SSERIALIZE_STATIC_TRACF_GRAPH_H
#include <unordered_set>
#include <vector>

#include <sserialize/utility/assert.h>

namespace sserialize {
namespace Static {
namespace spatial {

/** This is a graph that represents the graph of faces of single cell of triangulated region arrangement
  * The name stems from Triangulated Region Arrangement Cell Face Graph
  *
  * T_TRA needs to provide 
  *
  */
template<typename T_TRA>
class TRACFGraph final {
public:
	typedef T_TRA TRA;
	typedef typename TRA::Triangulation::Face Face;
public:
	TRACFGraph();
	TRACFGraph(const TRA * tra, const Face & rootFace);
	TRACFGraph(const TRACFGraph &) = default;
	~TRACFGraph() {}
	TRACFGraph & operator=(const TRACFGraph &) = default;
	uint32_t size() const;
	uint32_t cellId() const;
	double area() const;
	template<typename T_CALLBACK>
	void visitCB(T_CALLBACK cb) const {
		uint32_t myCellId = cellId();
		std::unordered_set<uint32_t> visitedFaces;
		std::vector<uint32_t> queuedFaces;
		queuedFaces.push_back(m_rootFace.id());
		visitedFaces.insert(queuedFaces.back());
		while(queuedFaces.size()) {
			Face f( m_tra->tds().face(queuedFaces.back()) );
			queuedFaces.pop_back();
			cb(f);
			for(int j(0); j < 3; ++j) {
				uint32_t nId = f.neighborId(j);
				if (!visitedFaces.count(nId) && m_tra->cellIdFromFaceId(nId) == myCellId) {
					visitedFaces.insert(nId);
					queuedFaces.push_back(nId);
				}
			}
		}
	}
	///visit every face exactly once
	template<typename T_OUT_ITERATOR>
	void visit(T_OUT_ITERATOR out) const {
		visitCB([&out](const Face & f) {
			*out = f;
			++out;
		});
	}
private:
	const TRA * m_tra;
	Face m_rootFace;
	mutable struct {
		uint32_t value:31;
		uint32_t cached:1;
	} m_size;
};

template<typename T_TRA>
TRACFGraph<T_TRA>::TRACFGraph() :
m_tra(0),
m_rootFace()
{
	m_size.cached = 1;
	m_size.value = 0;
}

template<typename T_TRA>
TRACFGraph<T_TRA>::TRACFGraph(const T_TRA * tra, const typename TRACFGraph<T_TRA>::Face& rootFace) :
m_tra(tra),
m_rootFace(rootFace)
{
	SSERIALIZE_CHEAP_ASSERT(m_tra);
	m_size.cached = 0;
}

template<typename T_TRA>
uint32_t TRACFGraph<T_TRA>::cellId() const {
	SSERIALIZE_CHEAP_ASSERT(m_tra);
	return m_tra->cellIdFromFaceId(m_rootFace.id());
}

template<typename T_TRA>
uint32_t TRACFGraph<T_TRA>::size() const {
	if (!m_size.cached) {
		uint32_t s = 0;
		this->visitCB([&s](auto) {++s;});
		m_size.cached = 1;
		m_size.value = s;
	}
	return m_size.value;
}

template<typename T_TRA>
double TRACFGraph<T_TRA>::area() const {
	SSERIALIZE_CHEAP_ASSERT(m_tra);
	double cellArea = 0.0;
	this->visitCB([&cellArea](Triangulation::Face const & face) {
		cellArea += face.area();
	});
	return cellArea;
}


}}}//end namespace


#endif
