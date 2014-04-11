#ifndef SSERIALIZE_GENERALIZED_TRIE_NEW_TRIE_H
#define SSERIALIZE_GENERALIZED_TRIE_NEW_TRIE_H
#include <string>
#include <unordered_map>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/Static/UnicodeTrie/Node.h>
#include <sserialize/Static/Deque.h>

namespace sserialize {
namespace UnicodeTrie {

template<typename TValue>
class Node {
public:
	typedef TValue value_type;
	typedef uint32_t key_type;
	typedef Node* map_type;
	typedef std::unordered_map<key_type, map_type> ChildrenContainer;
	typedef ChildrenContainer::iterator iterator;
	typedef ChildrenContainer::const_iterator const_iterator;
private:
	std::string m_str;
	Node * m_parent;
	ChildrenContainer m_children;
	TValue m_value;
public:
	Node(Node * parent) : m_parent(parent) {}
	virtual ~Node() {
		for(auto & x : m_children) {
			delete x.second;
		}
	}
	const Node* & parent() const { return m_parent; }
	Node* & parent() { return m_parent; }
	const std::string & str() const { return m_str; }
	std::string & str() { return m_str; }
	const ChildrenContainer & children() const { return m_children;}
	ChildrenContainer & children() { return m_children;}
	const TValue & value() const { return m_value;}
	TValue & value() { return m_value; }
	
	iterator begin() { return children().begin();}
	const_iterator cbegin() const { return children().cbegin();}
	iterator end() { return children().end();}
	const_iterator cend() const { return children().cend();}
};

template<typename TValue>
class Trie {
public:
	typedef sserialize::UnicodeTrie::Node<TValue> Node;
	typedef std::shared_ptr< sserialize::Static::UnicodeTrie::detail::NodeCreator > NodeCreatorPtr;
	struct DefaultPayloadHandler {
		///@param node the currently handled node
		TValue operator()(Node * node);
	};
private:
	Node * m_root;
protected:
	template<typename T_OCTET_ITERATOR>
	Node nodeAt(T_OCTET_ITERATOR begin, const T_OCTET_ITERATOR end);

public:
	Trie() : m_root(0) {}
	virtual Trie() { delete m_root;}
	template<typename T_OCTET_ITERATOR>
	TValue & at(T_OCTET_ITERATOR begin, const T_OCTET_ITERATOR end) {
		return nodeAt(begin, end)->value();
	}
	
	template<typename T_PH>
	UByteArrayAdapter serialize(UByteArrayAdapter& d, T_PH payloadHandler, NodeCreatorPtr nodeCreator) const;
};

template<typename TValue>
template<typename T_OCTET_ITERATOR>
Trie<TValue>::Node * Trie<TValue>::nodeAt(T_OCTET_ITERATOR begin, const T_OCTET_ITERATOR end) {
	uint32_t strItUCode;
	
	if (!m_root) {
		m_root = new Node(0);
	}
	Node * current = m_root;
	Node * newNode = 0;

	while(begin != end) {
		//Find the first different character
		std::string::const_iterator cIt(current->str().cbegin());
		std::string::const_iterator cEnd(current->str().cend());
		uint32_t cItUCode;
		strItUCode = ((begin != end) ? utf8::peek_next(begin, end) : 0);
		cItUCode = ((current->str().size() > 0) ? utf8::peek_next(cIt, cEnd) : 0);
		while (cIt != cEnd && begin != end) {
			strItUCode = utf8::peek_next(begin, end);
			cItUCode = utf8::peek_next(cIt, cEnd);
			if (strItUCode == cItUCode) {
				utf8::next(begin, end);
				utf8::next(cIt, cEnd);
			}
			else {
				break;
			}
		}
		//cIt=end or strIt=end or strIt != cIt
		if (cIt == cEnd && begin != end) { //node->c is real prefix, strIt points to new element
			strItUCode = utf8::peek_next(begin, end);
			if (current->children().count(strItUCode) > 0) {
				current = current->children().at(strItUCode);
			}
			else {
				newNode = new Node(current);
				newNode->str() = std::string(begin, end);
				(current->children())[strItUCode] = newNode;
				current = newNode;
				break;
			}
		}
		else if (cIt == cEnd && begin == end) { //node->c is equal to str
			break;
		}
		else if (cIt != cEnd && begin == end) { //str is prefix of node->c
			cItUCode = utf8::peek_next(cIt, cEnd);

			Node * oldStrNode = new Node;
			oldStrNode->str() = std::string(cIt, cEnd);
			std::swap(oldStrNode->children(), current->children());
			std::swap(oldStrNode->value(), current->value());

			//fix parent ptrs
			for(Node::iterator nit(oldStrNode->begin()), nend(oldStrNode->end()); nit != nend; ++nit) {
				nit->second->parent() = oldStrNode;
			}
			
			//save char for children
			uint32_t c = cItUCode;
			current->c.erase(cIt, current->c.end());
			
			//insert old node and add value
			(current->children())[c] = oldStrNode;
			oldStrNode->parent = current;
			break;
		}
		else if (cIt != cEnd && begin != end) { //possible common prefix, iterator at the different char

			strItUCode = utf8::peek_next(begin, end);
			cItUCode = utf8::peek_next(cIt, cEnd);

			Node * newNode = new Node(0);
			newNode->str() = std::string(begin, end);
			
			Node * oldStrNode = new Node(0);
			oldStrNode->str() = std::string(cIt, cEnd);
			std::swap(oldStrNode->children(), current->children());
			std::swap(oldStrNode->value(), current->value());
			//fix parent ptrs
			for(Node::iterator nit(oldStrNode->begin()), nend(oldStrNode->end()); nit != nend; ++nit) {
				nit->second->parent() = oldStrNode;
			}

			current->str().erase(cIt, cEnd);

			//add pointer to node with the rest of the old node string
			(current->children)[strItUCode] = newNode;
			newNode->parent = current;
			//add pointer the node with the rest of the new string
			(current->children)[cItUCode] = oldStrNode;
			oldStrNode->parent = current;
			current = newNode;
			break;
		}
	} // end while
	return current;
}


template<typename TValue>
template<typename T_PH>
UByteArrayAdapter Trie<TValue>::serialize(UByteArrayAdapter& d, T_PH payloadHandler, NodeCreatorPtr nodeCreator) const {
	sserialize::Static::DequeCreator<TValue> payloadContainerCreator(d);
	
	std::vector<Node*> nodesInLevelOrder(1, m_root);
	for(uint32_t i(0); i < nodesInLevelOrder.size(); ++i) {
		Node * n = nodesInLevelOrder[i];
		for(Node::const_iterator cIt(n->cbegin()), cEnd(n->cend()); cIt != cEnd; ++cIt) {
			nodesInLevelOrder.push_back(cIt->second);
		}
	}
	
	std::unordered_map<Node*, uint32_t> nodeOffsets;
	std::deque<uint8_t> trieData;
	std::vector<uint8_t> tmpData;
	
	while (nodesInLevelOrder.size()) {
		Node * n = nodesInLevelOrder.back();
		nodesInLevelOrder.pop_back();
		
		Static::UnicodeTrie::detail::NodeSerializationInfo nodeInfo;
		nodeInfo.strBegin = n->str().cbegin();
		nodeInfo.strEnd = n->str().cend();
		if (n->str().size()) { //skip the first char as it is already stored in parent
			utf8::next(nodeInfo.strBegin, nodeInfo);
		}
		
		nodeInfo.childKeyPtrOffsets.reserve(n->children().size());
		for(Node::const_iterator cIt(n->cbegin()), cEnd(n->cend()); cIt != cEnd; ++cIt) {
			nodeInfo.childKeyPtrOffsets.push_back(std::pair(cIt->first, trieData.size()-nodeOffsets.at(cIt->second)));
		}
		
		nodeInfo.payloadPtr =  payloadContainerCreator.size();
		payloadContainerCreator.push_back( payloadHandler(n) );
		
		tmpData.clear();
		UByteArrayAdapter d(&tmpData, false);
		nodeCreator->serialize(nodeInfo, d);
		
		prependToDeque(tmpData, trieData);
		nodeOffsets[n] = trieData.size();
	}
	payloadContainerCreator.flush();
	d.put(trieData);
	return d;
}

}} //end namespace


#endif