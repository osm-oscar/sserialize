#ifndef SSERIALIZE_STATIC_ROUTABLE_GRAPH_H
#define SSERIALIZE_STATIC_ROUTABLE_GRAPH_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/spatial/RoutableGraphInterface.h>
#include "Deque.h"


namespace sserialize {
namespace Static {
namespace Routing {

class EdgeInfo {
private:
public:
	EdgeInfo();
	EdgeInfo(const UByteArrayAdapter & data);
	~EdgeInfo();
};

template<typename TEdgeInfo>
class Edge {
private:
	uint32_t m_dest;
	TEdgeInfo m_edgeInfo;
public:
	Edge() : m_dest(0), m_edgeInfo() {}
	Edge(sserialize::UByteArrayAdapter data) : m_dest(data.getVlPackedUint32_t()), m_edgeInfo(data.shrinkToGetPtr()) {}
	~Edge() {}
	uint32_t destination() const { return m_dest; }
	
};

template<class TEdge>
class Node {
private:
	Static::Deque<TEdge> m_edges;
	uint32_t m_id;
public:
	Node() : m_id(0) {}
	Node(uint32_t id, const UByteArrayAdapter & data) : m_edges(data) {}
	~Node() {}
	uint32_t id() const { return m_id;}
	const Static::Deque<TEdge> & edges() const { return m_edges; }
};


template<class TEdge>
class RoutableGraph: public sserialize::spatial::routing::RoutableGraphInterface<uint32_t> {
private:
	Static::Deque< Node<TEdge> > m_graph;
public:
	RoutableGraph() {}
	RoutableGraph(const UByteArrayAdapter & data) : m_graph(data) {}
	virtual ~RoutableGraph() {}
	
	virtual 
	
	Node<TEdge> at(uint32_t id) const {
		if (id < size()) {
			return Node<TEdge>(id, m_graph.dataAt(id));
		}
		return Node<TEdge>();
	}
	uint32_t size() const { return m_graph.size();}
	
};

}}}//end namespace

#endif