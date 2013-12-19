#ifndef SSERIALIZE_EDGE_LIST_H
#define SSERIALIZE_EDGE_LIST_H
#include <vector>
#include <stdint.h>

namespace sserialize {

/** This Class abstracts access to a edge list which maps consecutive nodeids to their respective edges
  *
  *
  *
  *
  *
  */
template<typename TEDGE_PAYLOAD>
class EdgeList {
public:
	struct Edge {
		uint32_t target;
		TEDGE_PAYLOAD payload;
	};
	typedef std::vector<Edge> EdgeListStorageContainer;
	typedef typename EdgeListStorageContainer::const_iterator const_iterator;
	typedef typename EdgeListStorageContainer::iterator iterator;
private:
	typedef std::vector<uint32_t> NodeToEdgeOffsetList;
private:
	NodeToEdgeOffsetList m_edgeStarts;
	EdgeListStorageContainer m_edges;
public:
	EdgeList();
	virtual ~EdgeList();
	const_iterator cbegin(uint32_t node) const;
	const_iterator cend(uint32_t node) const;
	///@return nodeid
	uint32_t push_node(uint32_t edgecount);
	std::vector<uint32_t> reordered(uint32_t startNode, bool levelOrder) const;
};

template<typename TEDGE_PAYLOAD>
EdgeList<TEDGE_PAYLOAD>::EdgeList() {}

template<typename TEDGE>
EdgeList<TEDGE>::~EdgeList() {}

template<typename TEDGE_PAYLOAD>
typename EdgeList<TEDGE_PAYLOAD>::const_iterator
EdgeList<TEDGE_PAYLOAD>::cbegin(uint32_t node) const {
	return m_edges.cbegin()+m_edges.at(node);
}

template<typename TEDGE_PAYLOAD>
typename EdgeList<TEDGE_PAYLOAD>::const_iterator
EdgeList<TEDGE_PAYLOAD>::cend(uint32_t node) const {
	if (node+1 < m_edges.size()) {
		return cbegin(node+1);
	}
	return m_edges.cend();
}

template<typename TEDGE_PAYLOAD>
uint32_t EdgeList<TEDGE_PAYLOAD>::push_node(uint32_t edgecount) {
	uint32_t nodeId = m_edgeStarts.size();
	m_edgeStarts.push_back(m_edges.size());
	m_edges.resize(m_edges.size()+edgecount);
	return nodeId;
}

///This currently only works for trees!!
template<typename TEDGE_PAYLOAD>
std::vector<uint32_t> EdgeList<TEDGE_PAYLOAD>::reordered(uint32_t startNode, bool levelOrder) const {
	std::vector<uint32_t> res;
	res.push_back(startNode);
	if (levelOrder) {
		std::size_t i = 0;
		while (i < res.size()) {
			uint32_t node = res[i];
			for(const_iterator it(this->cbegin(node)), end(this->cend(node)); it != end; ++it) {
				res.push_back(it->target);
			}
			++i;
		}
	}
	else {
		typedef std::pair<const_iterator, const_iterator> MyStackEl;
		std::vector<MyStackEl> myStack;
		myStack.push_back( MyStackEl(cbegin(res[0]), cend(res[0])) );
		while (myStack.size()) {
			MyStackEl & n = myStack.back();
			if (n.first != n.second) {
				uint32_t nextNode = *n.first;
				++n.first;
				res.push_back(nextNode);
				myStack.push_back( MyStackEl(cbegin(nextNode), cend(nextNode)) );
			}
			else {
				myStack.pop_back();
			}
		}
	}
	return res;
}

}//end namespace

#endif