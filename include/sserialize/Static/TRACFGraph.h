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
	class Node final {
	public:
		Node() {}
		~Node() {}
		///this is the same as the correspondig faceId
		uint32_t id() const { return m_f.id(); }
		uint32_t neighborCount() const {
			uint32_t tmp = 0;
			for(int j(0); j < 3; ++j) {
				if (m_f.isNeighbor(j)) {
					++tmp;
				}
			}
			return tmp;
		}
		inline const Face & face() const { return m_f; }
		Node neighbor(uint32_t pos) const {
			uint32_t tmp = 0;
			for(int j(0); j < 3; ++j) {
				if (m_f.isNeighbor(j)) {
					if (tmp == pos) {
						return Node(m_f.neighbor(j));
					}
					else {
						++tmp;
					}
				}
			}
			return Node();
		}
	private:
		friend class TRACFGraph;
	private:
		Node(Face f) : m_f(f) {}
	private:
		Face m_f;
	};
public:
	TRACFGraph(const TRA * tra, const Face & rootFace);
	~TRACFGraph() {}
	uint32_t size() const;
	inline Node root() const { return m_root; }
	///visit every node exactly once
	template<typename T_OUT_ITERATOR>
	void visit(T_OUT_ITERATOR out) const {
		uint32_t myCellId = m_tra->cellIdFromFaceId(m_root.id());
		std::unordered_set<uint32_t> visitedFaces;
		std::vector<uint32_t> queuedFaces;
		queuedFaces.push_back(m_root.id());
		visitedFaces.insert(queuedFaces.back());
		while(queuedFaces.size()) {
			Face f( m_tra->tds().face(queuedFaces.back()) );
			queuedFaces.pop_back();
			(*out) = Node(f);
			++out;
			for(int j(0); j < 3; ++j) {
				uint32_t nId = f.neighborId(j);
				if (!visitedFaces.count(nId) && m_tra->cellIdFromFaceId(f.id()) == myCellId) {
					visitedFaces.insert(nId);
					queuedFaces.push_back(nId);
				}
			}
		}
	}
private:
	const TRA * m_tra;
	Node m_root;
	struct {
		uint32_t value:31;
		uint32_t cached:1;
	} m_size;
};

template<typename T_TRA>
TRACFGraph<T_TRA>::TRACFGraph(const T_TRA * tra, const typename TRACFGraph<T_TRA>::Face& rootFace) :
m_tra(tra),
m_root(rootFace)
{
	m_size.cached = 0;
}

template<typename T_TRA>
uint32_t TRACFGraph<T_TRA>::size() const {
	if (!m_size.cached) {
		uint32_t myCellId = m_tra->cellIdFromFaceId(m_root.id());
		std::unordered_set<uint32_t> visitedFaces;
		std::vector<uint32_t> queuedFaces;
		queuedFaces.push_back(m_root.id());
		visitedFaces.insert(queuedFaces.back());
		while(queuedFaces.size()) {
			Face f( m_tra->tds().face(queuedFaces.back()) );
			queuedFaces.pop_back();
			
			for(int j(0); j < 3; ++j) {
				uint32_t nId = f.neighborId(j);
				if (!visitedFaces.count(nId) && m_tra.cellIdFromFaceId(f.id()) == myCellId) {
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