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
	using FaceId = typename TRA::Triangulation::FaceId;
	using cellid_type = typename TRA::cellid_type;
public:
	TRACFGraph();
	TRACFGraph(const TRA * tra, const Face & rootFace);
	TRACFGraph(const TRACFGraph &) = default;
	~TRACFGraph() {}
	TRACFGraph & operator=(const TRACFGraph &) = default;
	std::size_t size() const;
	cellid_type cellId() const;
	double area() const;
	template<typename T_CALLBACK>
	void visitCB(T_CALLBACK cb) const {
		uint32_t myCellId = cellId();
		std::unordered_set<FaceId> visitedFaces;
		std::vector<FaceId> queuedFaces;
		queuedFaces.push_back(m_rootFace.id());
		visitedFaces.insert(queuedFaces.back());
		while(queuedFaces.size()) {
			Face f( m_tra->tds().face(queuedFaces.back()) );
			queuedFaces.pop_back();
			cb(f);
			for(int j(0); j < 3; ++j) {
				auto nId = f.neighborId(j);
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
	static constexpr int size_digits = std::numeric_limits<std::size_t>::digits-1;
	mutable struct {
		std::size_t value:size_digits;
		bool cached:1;
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
typename TRACFGraph<T_TRA>::cellid_type
TRACFGraph<T_TRA>::cellId() const {
	SSERIALIZE_CHEAP_ASSERT(m_tra);
	return m_tra->cellIdFromFaceId(m_rootFace.id());
}

template<typename T_TRA>
std::size_t TRACFGraph<T_TRA>::size() const {
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
