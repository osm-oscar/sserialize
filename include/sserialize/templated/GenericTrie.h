#ifndef SSERIALIZE_GENERIC_TRIE_H
#define SSERIALIZE_GENERIC_TRIE_H
#include <deque>
#include "TreeNode.h"

namespace sserialize {

template<typename AlphabetType, typename ItemIdType>
class GenericTrieNode: public TreeNode<AlphabetType, std::set<ItemIdType> > {
private:
	std::deque<AlphabetType> m_str;
public:
	GenericTrieNode() : TreeNode() {}
	virtual ~GenericTrieNode() {}
	std::deque<AlphabetType> str() { return m_str;}
};

template<typename AlphabetType, typename ItemType>
class GenericTrie {
public:
	typedef unsigned int ItemIdType;
	typedef GenericTrieNode<AlphabetType, ItemIdType> Node;
private:
	Node * m_root;
	std::deque<ItemType> m_values;
protected:
	virtual Node * createNode() {
		return new Node();
	}
public:
	GenericTrie();
	virtual ~GenericTrie();
	virtual bool insert(const std::deque<AlphabetType> & path, const ItemType & value);
	virtual GenericTrieNode<AlphabetType> * operator[](const std::deque< AlphabetType >& path);
	ItemType & fromId(ItemIdType id) { return m_values[id];}
	const ItemType & fromId(ItemIdType id) const { return m_values[id];}
};

template<typename AlphabetType, typename ItemType>
GenericTrie<AlphabetType, ItemType>::GenericTrie() : m_root(0) {}

template<typename ChildKeyType, typename ItemType>
GenericTrie<AlphabetType, ItemType>::~GenericTrie() {}

template<typename AlphabetType, typename ItemType>
bool
GenericTrie<AlphabetType, ItemType>::insert(const std::deque<AlphabetType> & path, const ItemType & value) {
	Node * node = operator[](path);
	if (!node)
		return false;

	ItemIdType itemId = m_values.size();
	node->value().insert(itemId);
	return true;
}

template<typename AlphabetType, typename ItemType>
GenericTrie<AlphabetType, ItemType>::Node *
GenericTrie<AlphabetType, ItemType>::operator[](const std::deque<AlphabetType> & path) {
	if (!m_root) {
		m_root = createNode();
		m_root->str() = path;
		return m_root;
	}
	else {

	strIt = str.begin();
	while(strIt != str.end()) {
		//Find the first different character
		std::string::iterator cIt = current->c.begin();
		uint32_t cItUCode;
		strItUCode = ((str.size() > 0) ? utf8::peek_next(strIt, str.end()) : 0);
		cItUCode = ((current->c.size() > 0) ? utf8::peek_next(cIt, current->c.end()) : 0);
		while (cIt != current->c.end() && strIt != str.end()) {
			strItUCode = utf8::peek_next(strIt, str.end());
			cItUCode = utf8::peek_next(cIt, current->c.end());
			if (strItUCode == cItUCode) {
				utf8::next(strIt, str.end());
				utf8::next(cIt, current->c.end());
			}
			else {
				break;
			}
		}
		//cIt=end or strIt=end or strIt != cIt
// 		strItUCode = ((str.size() > 0) ? utf8::peek_next(strIt, str.end()) : 0);
// 		cItUCode = ((current->c.size() > 0) ? utf8::peek_next(cIt, current->c.end()) : 0);
		if (cIt == current->c.end() && strIt != str.end()) { //node->c is real prefix, strIt points to new element
// 			std::cout << "case0 node->c real prefix" << std::endl;
			strItUCode = utf8::peek_next(strIt, str.end());
			if (current->children.count(strItUCode) > 0) {
				current = current->children.at(strItUCode);
			}
			else {
				newNode = new Node;
				m_nodeCount++;
				newNode->c = "";
				newNode->c.append(strIt, str.end());
				(current->children)[strItUCode] = newNode;
				newNode->parent = current;
				current = newNode;
				break;
			}
		}
		else if (cIt == current->c.end() && strIt == str.end()) { //node->c is equal to str
// 			std::cout << "case1 node->c = str" << std::endl;
// 			(current->values).push_back(value);
			break;
		}
		else if (cIt != current->c.end() && strIt == str.end()) { //str is prefix of node->c
// 			std::cout << "case2 str is prefix of node->c" << std::endl;
			cItUCode = utf8::peek_next(cIt, current->c.end());

			Node * oldStrNode = new Node;
			oldStrNode->c = "";
			oldStrNode->c.append(cIt, current->c.end());
			oldStrNode->children.swap(current->children);
			oldStrNode->partOfEnd = current->partOfEnd;
			oldStrNode->exactValues.swap(current->exactValues);
			oldStrNode->subStrValues.swap(current->subStrValues);

			oldStrNode->fixChildParentPtrRelation();
			
			//save char for children
			uint32_t c = cItUCode;
			
			//clear data, adjust to new c
			current->exactValues.clear();
			current->subStrValues.clear();
			current->children.clear();
			current->c.erase(cIt, current->c.end());
			current->partOfEnd = false; //will be set to true after while loop
			
			//insert old node and add value
			(current->children)[c] = oldStrNode;
			oldStrNode->parent = current;
// 			current->values.push_back(value);
			break;
		}
		else if (cIt != current->c.end() && strIt != str.end()) { //possible common prefix, iterator at the different char
// 			std::cout << "case3 node->c and string have common prefix" << std::endl;

			strItUCode = utf8::peek_next(strIt, str.end());
			cItUCode = utf8::peek_next(cIt, current->c.end());

			Node * newNode = new Node;
			newNode->c = "";
			newNode->c.append(strIt, str.end());
// 			newNode->values.push_back(value);
			
			Node * oldStrNode = new Node;
			oldStrNode->c = "";
			oldStrNode->c.append(cIt, current->c.end());
			oldStrNode->children.swap(current->children);
			oldStrNode->partOfEnd = current->partOfEnd;
			oldStrNode->exactValues.swap(current->exactValues);
			oldStrNode->subStrValues.swap(current->subStrValues);
			oldStrNode->fixChildParentPtrRelation();
			
			current->children.clear();
			current->exactValues.clear();
			current->subStrValues.clear();
			current->partOfEnd = false;
			current->c.erase(cIt, current->c.end());

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
	}
}

}//end namespace

#endif