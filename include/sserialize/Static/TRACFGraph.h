#ifndef SSERIALIZE_STATIC_TRACF_GRAPH_H
#define SSERIALIZE_STATIC_TRACF_GRAPH_H
#include <unordered_set>
#include <vector>

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
	TRACFGraph(const TRA * tra, const Face & rootFace);
	~TRACFGraph() {}
	uint32_t size() const;
	uint32_t cellId() const;
	///visit every face exactly once
	template<typename T_OUT_ITERATOR>
	void visit(T_OUT_ITERATOR out) const {
		uint32_t myCellId = cellId();
		std::unordered_set<uint32_t> visitedFaces;
		std::vector<uint32_t> queuedFaces;
		queuedFaces.push_back(m_rootFace.id());
		visitedFaces.insert(queuedFaces.back());
		while(queuedFaces.size()) {
			Face f( m_tra->tds().face(queuedFaces.back()) );
			queuedFaces.pop_back();
			(*out) = f;
			++out;
			for(int j(0); j < 3; ++j) {
				uint32_t nId = f.neighborId(j);
				if (!visitedFaces.count(nId) && m_tra->cellIdFromFaceId(nId) == myCellId) {
					visitedFaces.insert(nId);
					queuedFaces.push_back(nId);
				}
			}
		}
	}
private:
	const TRA * m_tra;
	Face m_rootFace;
	struct {
		uint32_t value:31;
		uint32_t cached:1;
	} m_size;
};

template<typename T_TRA>
TRACFGraph<T_TRA>::TRACFGraph(const T_TRA * tra, const typename TRACFGraph<T_TRA>::Face& rootFace) :
m_tra(tra),
m_rootFace(rootFace)
{
	m_size.cached = 0;
}

template<typename T_TRA>
uint32_t TRACFGraph<T_TRA>::cellId() const {
	return m_tra->cellIdFromFaceId(m_rootFace.id());
}

template<typename T_TRA>
uint32_t TRACFGraph<T_TRA>::size() const {
	if (!m_size.cached) {
		uint32_t myCellId = cellId();
		std::unordered_set<uint32_t> visitedFaces;
		std::vector<uint32_t> queuedFaces;
		queuedFaces.push_back(m_rootFace.id());
		visitedFaces.insert(queuedFaces.back());
		while(queuedFaces.size()) {
			Face f( m_tra->tds().face(queuedFaces.back()) );
			queuedFaces.pop_back();
			
			for(int j(0); j < 3; ++j) {
				uint32_t nId = f.neighborId(j);
				if (!visitedFaces.count(nId) && m_tra->cellIdFromFaceId(nId) == myCellId) {
					visitedFaces.insert(nId);
					queuedFaces.push_back(nId);
				}
			}
		}
		m_size.cached = 1;
		m_size.value = visitedFaces.size();
	}
	return m_size.value;
}


}}}//end namespace


#endif