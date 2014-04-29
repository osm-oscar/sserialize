#ifndef SSERIALIZE_GENERIC_TRIE_H
#define SSERIALIZE_GENERIC_TRIE_H
#include <vector>
#include <map>

namespace sserialize {

///AlphabetStorage has to provide size() and const_iterator
template<typename AlphabetType, typename TValue, typename AlphabetStorage = std::vector<AlphabetType> >
class GenericTrie {
protected:
	typedef typename AlphabetStorage::iterator AlphabetStorageIterator;
	typedef typename AlphabetStorage::const_iterator AlphabetStorageConstIterator;
	
	class Node {
	public:
		typedef std::map<AlphabetType, Node*> ChildrenMap;
		typedef typename ChildrenMap::iterator ChildrenIterator;
		typedef typename ChildrenMap::const_iterator ChildrenConstIterator;
	protected:
		AlphabetStorage m_storage;
		TValue m_value;
		ChildrenMap m_children;
		Node * m_parent;
	public:
		Node() {}
		virtual ~Node() {
			for(ChildrenIterator it(children().begin()), end(children().end()); it != end; ++it)
				delete it->second;
		}
		Node* & parent() { return m_parent; }
		const Node* const & parent()  const { return m_parent; }
	
		const TValue & value() const { return m_value; }
		TValue & value() { return m_value; }
		const ChildrenMap & children() const { return m_children; }
		ChildrenMap & children() { return m_children; }
		const AlphabetStorage & string() const { return m_storage; }
		AlphabetStorage & string() { return m_storage; }
		void fixChildParentPtrRelation() {
			for(ChildrenIterator it(children().begin()), end(children().end()); it != end; ++it) {
				it->second->parent() = this;
			}
		}
	};
	
	typedef typename Node::ChildrenMap NodeChildrenMap;
	typedef typename Node::ChildrenIterator NodeChildrenIterator;
	typedef typename Node::ChildrenConstIterator NodeChildrenConstIterator;
	
private:
	Node * m_root;
public:
	GenericTrie() : m_root(0) {}
	virtual ~GenericTrie() { delete m_root; }
	template<typename AlphabetIterator>
	TValue & at(const AlphabetIterator & strBegin, const AlphabetIterator & strEnd) {
		using std::swap;
		AlphabetIterator strIt = strBegin;
		AlphabetType strItUCode;
		
		if (!m_root) {
			m_root = new Node();
		}
		Node * current = m_root;
		Node * newNode = 0;

		strIt = strBegin;
		while(strIt != strEnd) {
			//Find the first different character
			AlphabetStorageIterator cIt = current->string().begin();
			AlphabetType cItUCode;
			strItUCode = ((strBegin != strEnd) ? *strIt : AlphabetType());
			cItUCode = ((current->string().size() > 0) ? *cIt : AlphabetType());
			while (cIt != current->string().end() && strIt != strEnd) {
				strItUCode = *strIt;
				cItUCode = *cIt;
				if (strItUCode == cItUCode) {
					++strIt;
					++cIt;
				}
				else {
					break;
				}
			}
			//cIt=end or strIt=end or strIt != cIt
			if (cIt == current->string().end() && strIt != strEnd) { //node->c is real prefix, strIt points to new element
				strItUCode = *strIt;
				if (current->children().count(strItUCode) > 0) {
					current = current->children().at(strItUCode);
				}
				else {
					newNode = new Node;
					newNode->string().insert(newNode->string().end(), strIt, strEnd);
					(current->children())[strItUCode] = newNode;
					newNode->parent() = current;
					current = newNode;
					break;
				}
			}
			else if (cIt == current->string().end() && strIt == strEnd) { //node->c is equal to str
				break;
			}
			else if (cIt != current->string().end() && strIt == strEnd) { //str is prefix of node->c
				cItUCode = *cIt;
				Node * oldStrNode = new Node;
				oldStrNode->string() = AlphabetStorage(cIt, current->string().end());
				swap(oldStrNode->children(), current->children());

				oldStrNode->fixChildParentPtrRelation();
				
				//save char for children
				AlphabetType c = cItUCode;
				
				//clear data, adjust to new c
				current->value() = TValue();
				current->children().clear();
				current->string().erase(cIt, current->string().end());
				
				//insert old node and add value
				(current->children())[c] = oldStrNode;
				oldStrNode->parent() = current;
				break;
			}
			else if (cIt != current->string().end() && strIt != strEnd) { //possible common prefix, iterator at the different char

				strItUCode = *strIt;
				cItUCode = *cIt;

				Node * newNode = new Node;
				newNode->string() = AlphabetStorage(strIt, strEnd);
				
				Node * oldStrNode = new Node;
				oldStrNode->string() = AlphabetStorage(cIt, current->string().end());
				swap(oldStrNode->children(), current->children());
				swap(oldStrNode->value(), current->value());
				oldStrNode->fixChildParentPtrRelation();
				
				current->children().clear();
				current->value() = TValue();
				current->string().erase(cIt, current->string().end());

				//add pointer to node with the rest of the old node string
				(current->children())[strItUCode] = newNode;
				newNode->parent() = current;
				//add pointer the node with the rest of the new string
				(current->children())[cItUCode] = oldStrNode;
				oldStrNode->parent() = current;
				current = newNode;
				break;
			}
		} // end while
		return current->value();
	}
	
	template<typename AlphabetIterator>
	int count(const AlphabetIterator & strBegin, const AlphabetIterator & strEnd, bool prefixMatch) {
		if (!m_root)
			return 0;
		
		Node * node = m_root;
		AlphabetStorageConstIterator cIt(node->string().begin());
		AlphabetIterator strIt(strBegin);
		while (strIt != strEnd) {
			while(cIt != node->string().end() && strIt != strEnd) {
				if (*cIt == *strIt) {
					++cIt;
					++strIt;
				}
				else {
					return 0;
				}
			}
			if (strIt != strEnd) {
				NodeChildrenConstIterator it = node->children().find(*strIt);
				if (it != node->children().end()) {
					node = it->second;
					cIt = node->string().begin();
				}
				else {
					return 0;
				}
			}
		}
		
		if (cIt != node->string().end()) {
			if (prefixMatch)
				return 1;
			else
				return 0;
		}
		else {
			return 1;
		}
	}
};


}//end namespace

#endif