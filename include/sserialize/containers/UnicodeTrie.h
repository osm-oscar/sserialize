#ifndef SSERIALIZE_UNICODE_TRIE_H
#define SSERIALIZE_UNICODE_TRIE_H
#include <string>
#include <unordered_map>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/Static/UnicodeTrie/Node.h>
#include <sserialize/Static/UnicodeTrie/Trie.h>

namespace sserialize {
namespace UnicodeTrie {

template<typename TValue>
class Node {
public:
	typedef TValue value_type;
	typedef uint32_t key_type;
	typedef Node* map_type;
	//currenty, ChildrenContainer has to be sorted my key_type
	typedef typename std::map<key_type, map_type> ChildrenContainer;
	typedef typename ChildrenContainer::iterator iterator;
	typedef typename ChildrenContainer::const_iterator const_iterator;
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
	typedef TValue value_type;
	typedef std::shared_ptr< sserialize::Static::UnicodeTrie::NodeCreator > NodeCreatorPtr;
	struct DefaultPayloadHandler {
		///@param node the currently handled node
		TValue operator()(Node * node);
	};
private:
	Node * m_root;
protected:
	template<typename T_OCTET_ITERATOR>
	Node * nodeAt(T_OCTET_ITERATOR begin, const T_OCTET_ITERATOR & end);


	bool checkTrieEqualityRec(const Node * node, const sserialize::Static::UnicodeTrie::Node & snode) const;
	
	///T_PAYLOAD_COMPARATOR: bool operator()(Trie::Node * srcNode, const Node & staticNode)
	template<typename T_PAYLOAD_COMPARATOR>
	bool checkPayloadEqualityRec(const Node * node, const sserialize::Static::UnicodeTrie::Node & rootNode, const T_PAYLOAD_COMPARATOR & payloadComparator) const;
	
	Trie(const Trie & o);
	Trie(const Trie && o);
	Trie & operator=(const Trie & o);
	Trie & operator=(const Trie && o);
public:
	Trie() : m_root(0) {}
	virtual ~Trie() { delete m_root;}
	
	///create a new node if needed
	template<typename T_OCTET_ITERATOR>
	TValue & at(T_OCTET_ITERATOR begin, const T_OCTET_ITERATOR & end) {
		return nodeAt(begin, end)->value();
	}
	
	///@param prefixMatch strIt->strEnd can be a prefix of the path
	template<typename T_OCTET_ITERATOR>
	bool count(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) const;
	
	///@param prefixMatch strIt->strEnd can be a prefix of the path
	template<typename T_OCTET_ITERATOR>
	const Node * findNode(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) const;
	
	///@param prefixMatch strIt->strEnd can be a prefix of the path
	template<typename T_OCTET_ITERATOR>
	const TValue & find(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) const;
	
	template<typename T_PH, typename T_STATIC_PAYLOAD = TValue>
	UByteArrayAdapter append(UByteArrayAdapter& d, T_PH payloadHandler, NodeCreatorPtr nodeCreator) const;
	
	bool checkTrieEquality(const sserialize::Static::UnicodeTrie::Node & rootNode) const {
		return checkTrieEqualityRec(m_root, rootNode);
	}
	///T_PAYLOAD_COMPARATOR: bool operator()(UnicodeTrie::Trie::Node * srcNode, const Static::UnicodeTrie::Trie::Node & staticNode) const
	template<typename T_PAYLOAD_COMPARATOR>
	bool checkPayloadEquality(const sserialize::Static::UnicodeTrie::Node & rootNode, T_PAYLOAD_COMPARATOR payloadComparator) const {
		return checkPayloadEqualityRec(m_root, rootNode, payloadComparator);
	}
};

template<typename TValue>
template<typename T_OCTET_ITERATOR>
typename Trie<TValue>::Node * Trie<TValue>::nodeAt(T_OCTET_ITERATOR begin, const T_OCTET_ITERATOR & end) {
	uint32_t strItUCode;
	
	if (!m_root) {
		m_root = new Node(0);
	}
	Node * current = m_root;
	Node * newNode = 0;

	while(begin != end) {
		//Find the first different character
		std::string::iterator cIt(current->str().begin());
		std::string::iterator cEnd(current->str().end());
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

			Node * oldStrNode = new Node(0);
			oldStrNode->str() = std::string(cIt, cEnd);
			std::swap(oldStrNode->children(), current->children());
			std::swap(oldStrNode->value(), current->value());

			//fix parent ptrs
			for(typename Node::iterator nit(oldStrNode->begin()), nend(oldStrNode->end()); nit != nend; ++nit) {
				nit->second->parent() = oldStrNode;
			}
			
			//save char for children
			uint32_t c = cItUCode;
			current->str().erase(cIt, cEnd);
			
			//insert old node and add value
			(current->children())[c] = oldStrNode;
			oldStrNode->parent() = current;
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
			for(typename Node::iterator nit(oldStrNode->begin()), nend(oldStrNode->end()); nit != nend; ++nit) {
				nit->second->parent() = oldStrNode;
			}

			current->str().erase(cIt, cEnd);

			//add pointer to node with the rest of the old node string
			current->children()[strItUCode] = newNode;
			newNode->parent() = current;
			//add pointer the node with the rest of the new string
			current->children()[cItUCode] = oldStrNode;
			oldStrNode->parent() = current;
			current = newNode;
			break;
		}
	} // end while
	return current;
}

template<typename TValue>
template<typename T_OCTET_ITERATOR>
const typename Trie<TValue>::Node * Trie<TValue>::findNode(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR & strEnd, bool prefixMatch) const {
	if (!m_root)
		return 0;

	Node * node = m_root;

	std::string::const_iterator nStrIt(node->str().cbegin());
	std::string::const_iterator nStrEnd(node->str().cend());
	
	//Find the end-node
	while(strIt != strEnd) {
		while(strIt != strEnd && nStrIt != nStrEnd) {
			if (*strIt == *nStrIt) {
				++strIt;
				++nStrIt;
			}
			else { //no common prefix
				return 0;
			}
		}

		if (nStrIt == nStrEnd && strIt != strEnd) { //node->c is real prefix, strIt points to new element
			uint32_t key = utf8::peek_next(strIt, strEnd);
			if (node->children().count(key)) {
				node = node->children().at(key);
				nStrIt = node->str().cbegin();
				nStrEnd = node->str().cend();
			}
			else {
				return 0;
			}
		}
	}
	
	if (nStrIt != nStrEnd && !prefixMatch) {
		return 0;
	}
	return node;
}

template<typename TValue>
template<typename T_OCTET_ITERATOR>
const TValue & Trie<TValue>::find(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR & strEnd, bool prefixMatch) const {
	Node * n = findNode(strIt, strEnd, prefixMatch);
	if (!n)
		throw sserialize::OutOfBoundsException("sserialize::UnicodeTrie::Trie::find");
	return n->value();
}

template<typename TValue>
template<typename T_OCTET_ITERATOR>
bool Trie<TValue>::count(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR & strEnd, bool prefixMatch) const {
	return findNode(strIt, strEnd, prefixMatch);
}


template<typename TValue>
template<typename T_PH, typename T_STATIC_PAYLOAD>
UByteArrayAdapter Trie<TValue>::append(UByteArrayAdapter& d, T_PH payloadHandler, NodeCreatorPtr nodeCreator) const {
	d.putUint8(1);
	sserialize::Static::DequeCreator<T_STATIC_PAYLOAD> payloadContainerCreator(d);
	
	std::vector<Node*> nodesInLevelOrder(1, m_root);
	for(uint32_t i(0); i < nodesInLevelOrder.size(); ++i) {
		Node * n = nodesInLevelOrder[i];
		for(typename Node::const_iterator cIt(n->cbegin()), cEnd(n->cend()); cIt != cEnd; ++cIt) {
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
			utf8::next(nodeInfo.strBegin, nodeInfo.strEnd);
		}
		
		nodeInfo.childKeyPtrOffsets.reserve(n->children().size());
		for(typename Node::const_iterator cIt(n->cbegin()), cEnd(n->cend()); cIt != cEnd; ++cIt) {
			nodeInfo.childKeyPtrOffsets.push_back(std::pair<uint32_t, uint32_t>(cIt->first, trieData.size()-nodeOffsets.at(cIt->second)));
		}
		
		nodeInfo.payloadPtr =  payloadContainerCreator.size();
		payloadContainerCreator.put( payloadHandler(n) );
		
		tmpData.clear();
		UByteArrayAdapter d(&tmpData, false);
		nodeCreator->append(nodeInfo, d);
		
		prependToDeque(tmpData, trieData);
		nodeOffsets[n] = trieData.size();
	}
	payloadContainerCreator.flush();
	d.putUint32(nodeCreator->type());
	d.put(trieData);
	return d;
}


template<typename TValue>
bool
Trie<TValue>::checkTrieEqualityRec(const Node * node, const sserialize::Static::UnicodeTrie::Node & snode) const {
	if (node) {
		std::string sstr(snode.str());
		if (node->str().size()) {
			std::string::const_iterator nstrIt(node->str().cbegin());
			std::string::const_iterator nstrEnd(node->str().cend());
			utf8::next(nstrIt, nstrEnd);
			if( sstr != std::string(nstrIt, nstrEnd) )
				return false;
		}
		else if (sstr.size()) {
			return false;
		}
		if (node->children().size() == snode.childSize()) {
			uint32_t schild = 0;
			for(typename Node::const_iterator cIt(node->cbegin()), cEnd(node->cend()); cIt != cEnd; ++cIt, ++schild) {
				if (cIt->first != snode.childKey(schild) || !checkTrieEqualityRec(cIt->second, snode.child(schild))) {
					return false;
				}
			}
		}
		else {
			return false;
		}
		return true;
	}
	return false;
}

template<typename TValue>
template<typename T_PAYLOAD_COMPARATOR>
bool Trie<TValue>::checkPayloadEqualityRec(const Node * node, const sserialize::Static::UnicodeTrie::Node & snode, const T_PAYLOAD_COMPARATOR & payloadComparator) const {
	if (node) {
		if (!payloadComparator(node, snode)) {
			return false;
		}
		if (node->children().size() == snode.childSize()) {
			uint32_t schild = 0;
			for(typename Node::const_iterator cIt(node->cbegin()), cEnd(node->cend()); cIt != cEnd; ++cIt, ++schild) {
				if (cIt->first != snode.childKey(schild) || ! checkPayloadEqualityRec(cIt->second, snode.child(schild), payloadComparator)) {
					return false;
				}
			}
		}
		else {
			return false;
		}
		return true;
	}
	return false;
}



}} //end namespace


#endif