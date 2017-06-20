#ifndef SSERIALIZE_STATIC_TRAC_GRAPH_H
#define SSERIALIZE_STATIC_TRAC_GRAPH_H
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/containers/MultiVarBitArray.h>
#define SSERIALIZE_STATIC_TRAC_GRAPH_VERSION 1

namespace sserialize {
namespace Static {
namespace spatial {

/** File format:
  *
  * {
  *    VERSION     u8
  *    Nodes       sserialize::MultiVarBitArray
  *    Edges       sserialize::BoundedCompactUintArray
  */


class TracGraph final {
public:
	class Node final {
	public:
		typedef sserialize::ReadOnlyAtStlIterator<const Node*, Node, int32_t> const_iterator;
	public:
		Node() : m_p(0), m_pos(0xFFFFFFFF) {}
		~Node() {}
		///identical with cellId
		inline uint32_t id() const { return m_pos; }
		inline uint32_t cellId() const { return id(); }
		inline uint32_t size() const { return neighborCount(); }
		inline uint32_t neighborCount() const { return m_p->nodeInfo().at(id(), NI_NEIGHBOR_COUNT); }
		inline Node neighbor(uint32_t pos) const { return Node(m_p, neighborId(pos)); }
		inline uint32_t neighborId(uint32_t pos) const { return m_p->edgeInfo().at( m_p->nodeInfo().at(id(), NI_NEIGHBOR_BEGIN)+pos); }
		inline Node at(uint32_t pos) const { return neighbor(pos); }
	public:
		const_iterator cbegin() const { return const_iterator(0, this); }
		const_iterator begin() const { return const_iterator(0, this); }
		
		const_iterator cend() const { return const_iterator(size(), this); }
		const_iterator end() const { return const_iterator(size(), this); }
	private:
		friend class TracGraph;
	private:
		typedef enum {NI_NEIGHBOR_COUNT=0x0, NI_NEIGHBOR_BEGIN=0x1, NI__ENTRIES=0x2} NodeInfo;
	private:
		Node(const TracGraph * p, uint32_t pos) : m_p(p), m_pos(pos) {}
	private:
		const TracGraph * m_p;
		uint32_t m_pos;
	};
public:
	TracGraph();
	TracGraph(const sserialize::UByteArrayAdapter & d);
	~TracGraph() {}
	inline uint32_t size() const { return nodeInfo().size(); }
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	inline Node node(uint32_t cellId) const { return Node(this, cellId); }
	///proc(const Node & n)
	template<typename TProcessor>
	void explore(uint32_t cellId, TProcessor proc) const;
private:
	friend class Node;
	typedef sserialize::MultiVarBitArray NodeInfo;
	typedef sserialize::BoundedCompactUintArray EdgeInfo;
private:
	inline const NodeInfo & nodeInfo() const { return m_nodeInfo; }
	inline const EdgeInfo & edgeInfo() const { return m_edgeInfo; }
private:
	NodeInfo m_nodeInfo;
	EdgeInfo m_edgeInfo;
};

template<typename TProcessor>
void TracGraph::explore(uint32_t cellId, TProcessor proc) const {
	std::vector<uint32_t> queue;
	std::unordered_set<uint32_t> visited;
	
	queue.push_back(cellId);
	visited.insert(cellId);
	
	while(queue.size()) {
		uint32_t nId = queue.back();
		queue.pop_back();
		
		Node n(node(nId));
		proc(n);
		
		for(uint32_t i(0), s(n.size()); i < s; ++i) {
			uint32_t nnId = n.neighborId(i);
			if (!visited.count(nnId)) {
				queue.push_back(nnId);
				visited.insert(nnId);
			}
		}
	}
}

}}}//end namespace

#endif