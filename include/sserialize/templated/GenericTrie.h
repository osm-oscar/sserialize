#ifndef SSERIALIZE_GENERIC_TRIE_H
#define SSERIALIZE_GENERIC_TRIE_H
#include <vector>
#include <map>

namespace sserialize {

template<typename AlphabetType, typename ItemType, typename AlphabetStorage = std::vector<AlphabetType> >
class GenericTrie {
	class Node {
		Node() {}
		virtual ~Node() {
			for(std::map<AlphabetType, Node*>::iterator it(m_children.begin()); it != m_children.end(); ++it)
				delete it->second;
		}
	
		AlphabetStorage m_storage;
		ItemType m_items;
		std::map<AlphabetType, Node*> m_children;
		const ItemType & items() const { return m_items; }
		ItemType & items() { return m_items; }
		const std::map<AlphabetType, Node*> & children() const { return m_children; }
		std::map<AlphabetType, Node*> & children() { return m_children; }
		const AlphabetStorage & string() const { return m_storage; }
		AlphabetStorage & string() { return m_storage; }
	};
private:
	Node * m_root;
public:
	GenericTrie() : m_root(0) {}
	virtual ~GenericTrie() { delete m_root; }
	template<typename AlphabetIterator>
	Node * operator[](const AlphabetIterator & strBegin, const AlphabetIterator & strEnd) {
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
			AlphabetStorage::const_iterator cIt = current->string().begin();
			AlphabetType cItUCode;
			strItUCode = ((strBegin != strEnd) ? *strIt : AlphabetType());
			cItUCode = ((current->c.size() > 0) ? *cIt : AlphabetType());
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
			if (cIt == current->c.end() && strIt != strEnd) { //node->c is real prefix, strIt points to new element
				strItUCode = *strIt;
				if (current->children.count(strItUCode) > 0) {
					current = current->children.at(strItUCode);
				}
				else {
					newNode = new Node;
					newNode->c = "";
					newNode->c.append(strIt, strEnd);
					(current->children)[strItUCode] = newNode;
					newNode->parent = current;
					current = newNode;
					break;
				}
			}
			else if (cIt == current->c.end() && strIt == strEnd) { //node->c is equal to str
				break;
			}
			else if (cIt != current->c.end() && strIt == strEnd) { //str is prefix of node->c
				cItUCode = *cIt;
				Node * oldStrNode = new Node;
				oldStrNode->string() = AlphabetStorage(cIt, current->c.end());
				oldStrNode->children().swap(current->children());

				oldStrNode->fixChildParentPtrRelation();
				
				//save char for children
				AlphabetType c = cItUCode;
				
				//clear data, adjust to new c
				current->items() = ItemType();
				current->children.clear();
				current->c.erase(cIt, current->string().end());
				
				//insert old node and add value
				(current->children())[c] = oldStrNode;
				oldStrNode->parent = current;
				break;
			}
			else if (cIt != current->c.end() && strIt != strEnd) { //possible common prefix, iterator at the different char

				strItUCode = *strIt;
				cItUCode = *cIt;

				Node * newNode = new Node;
				newNode->string() = AlphabetStorage(strIt, strEnd);
				
				Node * oldStrNode = new Node;
				oldStrNode->string() = AlphabetStorage(cIt, current->string().end());
				oldStrNode->children.swap(current->children);
				std::swap<ItemType>(oldStrNode->items(), current->items());
				oldStrNode->fixChildParentPtrRelation();
				
				current->children().clear();
				current->items() = ItemType();
				current->string().erase(cIt, current->c.end());

				//add pointer to node with the rest of the old node string
				(current->children())[strItUCode] = newNode;
				newNode->parent = current;
				//add pointer the node with the rest of the new string
				(current->children())[cItUCode] = oldStrNode;
				oldStrNode->parent = current;
				current = newNode;
				break;
			}
		} // end while
		return current;
	}
};


}//end namespace

#endif