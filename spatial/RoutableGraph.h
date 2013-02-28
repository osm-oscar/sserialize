#ifndef SSERIALIZE_ROUTABLE_GRAPH_H
#define SSERIALIZE_ROUTABLE_GRAPH_H
#include <vector>
#include <unordered_map>
#include <set>
#include <stdint.h>
#include <sserialize/utility/DynamicArray.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include "RoutableGraphInterface.h"

namespace sserialize {
namespace spatial {
namespace routing {
namespace RoutableGraph {

typedef uint64_t NodeIdType;

template<typename EdgePayload>
class Edge {
public:
	Edge(NodeIdType id, const EdgePayload & payload) : id(id), payload(payload) {}
	virtual ~Edge() {}
	NodeIdType destination;
	EdgePayload payload;
};

template<typename NodePayload, typename EdgeType>
class Node {
public:
	typedef DynamicArray<EdgeType> EdgesContainerType;
private:
	NodeIdType m_id;
	NodePayload m_payload;
	EdgesContainerType m_edges;
public:
	Node() : m_id(0) {}
	Node(NodeIdType id) : m_id(id) {}
	virtual ~Node() {}
	void resize(size_t count) {
		m_edges.resize(count);
	}
	NodeIdType id() const { return m_id;}
	void setId(NodeIdType id) { m_id = id; }
	
	NodePayload & payload() { return m_payload; }
	
	DynamicArray<Edge> & edges() { return m_edges;}
};


template<typename NodeType, typename EdgeType>
class RoutableGraph: public sserialize::spatial::routing::RoutableGraphInterface<NodeIdType> {
	typedef NodeType Node;
	typedef std::unordered_map<NodeIdType, uint32_t> IdPosMapType;
	typedef std::vector<Node> GraphStorageType;
private:
	std::vector<Node> m_graph;
	IdPosMapType m_idPosMap;
	bool m_compactified;
public:
	RoutableGraph();
	virtual ~RoutableGraph();
	virtual NodeIdType expand(NodeIdType node, uint32_t edgePosition) const {
		return operator[](node).edges().at(edgePosition).destination;
	}

	void reserve(size_t nodeCount) {
		m_graph.reserve(nodeCount);
		m_idPosMap.reserve(nodeCount);
	}
	
	const Node & operator[](NodeIdType id) const;
	RoutableGraph::Node & operator[](Node::NodeIdType id);
	
	bool addNode(const Node & node);
	bool addEdge(sserialize::Routing::NodeIdType from, const EdgeType& edge, bool reverse);
	bool addWay(sserialize::Routing::NodeIdType from, const std::vector< EdgeType >& way, bool undirected);
	
	/** remaps node ids in such a way that there 0..n ascending ids,
		edges pointing to nodes that are not part of the graph are removed*/

	std::unordered_map<NodeIdType, uint32_t> remap();
	
	bool serialize() {}
};

template<typename NodeType, typename EdgeType>
RoutableGraph<NodeType, EdgeType>::RoutableGraph() {}
RoutableGraph<NodeType, EdgeType>::~RoutableGraph() {}


template<typename NodeType, typename EdgeType>
const Node& RoutableGraph<NodeType, EdgeType>::operator[](sserialize::Routing::NodeIdType id) const {
	return m_graph[ m_idPosMap[id] ];
}

template<typename NodeType, typename EdgeType>
Node& RoutableGraph<NodeType, EdgeType>::operator[](Node::NodeIdType id) {
	return m_graph[ m_idPosMap[id] ];
}


template<typename NodeType, typename EdgeType>
bool RoutableGraph<NodeType, EdgeType>::addNode(sserialize::Routing::NodeIdType id) {
	if (m_idPosMap.count(id) == 0) {
		m_idPosMap[id] = m_graph.size();
		m_graph.push_back(Node(id));
		return true;
	}
	return true;
}

template<typename NodeType, typename EdgeType>
bool RoutableGraph<NodeType, EdgeType>::addEdge(sserialize::Routing::NodeIdType from, const EdgeType & edge, bool reverse) {
	if (reverse) {
		if (m_idPosMap.count(edge.destination()) > 0) {
			uint32_t nodePos = m_idPosMap[edge.destination()];
			m_graph[nodePos].edges().push_back(edge);
			m_graph[nodePos].edges().back().destination() = from;
			return true;
		}
	}
	else {
		if (m_idPosMap.count(from) > 0) {
			uint32_t nodePos = m_idPosMap[from];
			m_graph[nodePos].edges().push_back(edge);
			return true;
		}
	}
	return false;
}


template<typename NodeType, typename EdgeType>
bool RoutableGraph<NodeType, EdgeType>::addWay(NodeIdType from, const std::vector<EdgeType> & way, bool undirected) {
	if (way.size() == 0)
		return false;

	bool allOk = true;
	std::vector<EdgeType>::const_iterator it(way.begin());
	std::vector<EdgeType>::const_iterator end( way.end() );
	for(; it != way.end(); ++it) {
		allOk = addEdge(from, *it, false) && allOk;
		if (undirected)
			allOk = addEdge(from, *it, true) && allOk;
		from = it->destination();
	}
	return allOk;
}

template<typename NodeType, typename EdgeType>
std::unordered_map<NodeIdType, uint32_t> RoutableGraph::compactify() {
	IdPosMapType newIdPosMap;
	
	GraphStorageType::iterator gIt = m_graph.begin();
	GraphStorageType::iterator gEnd = m_graph.end();
	
	for(; gIt != gEnd; ++gIt) {
		std::set<Node::NodeIdType, TComparator> edges;
		for(size_t eIt = 0; eIt < gIt->edges().size(); ++eIt) {
			if (m_idPosMap.count(gIt->edges()[eIt].id) > 0) {
				edges.insert(m_idPosMap[gIt->edges()[eIt]]);
			}
		}
		Node::EdgesContainerType newEdges;
		for(std::set<Node::NodeIdType, TComparator>::const_iterator neIt = edges.begin(); neIt != edges.end(); ++neIt) {
			newEdges.push_back(*neIt);
		}
		gIt->edges() = newEdges;
	}
	
	//create new identiy mapping
	m_idPosMap = IdPosMapType();
	m_idPosMap.reserve(m_graph.size());
	for(size_t i = 0; i < m_graph.size(); ++i)
		m_idPosMap[i] = i;
}

}}}}//end namespace

template<typename NodePayload, typename EdgeType>
sserialize::UByteArrayAdapter&
operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::Routing::Node<NodePayload, EdgeType> & node);

#endif