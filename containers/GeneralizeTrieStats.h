#ifndef GENERALIZED_TRIE_STATS_H
#define GENERALIZED_TRIE_STATS_H
#include "GeneralizedTrie.h"

template <typename ItemType>
class GeneralizedTrieStats {
private:
	GeneralizedTrieStats<ItemType> & m_trie;

private:
	uint64_t minSizeRecurse(Node * node);
	void branchingFactorCountRecurse(Node * node, std::map<uint16_t, uint32_t> * m);
	void nodeStringCountRecurse(Node * node, std::map<std::string, uint32_t> * m);
	void nodeStringLenNodeCountRecurse(Node * node, uint32_t * cumStrLen, uint32_t * nodeCount);
	std::string getLongestStringRecurse(Node * node, std::string l);
	uint32_t getDepthRecurse(Node * node, uint32_t d);
	
public:
	GeneralizedTrieStats(GeneralizedTrie & trie) : m_trie(trie) {}
	~GeneralizedTrieStats();
public: //Statistics
	inline uint64_t minSize() { return minSizeRecurse(m_trie.rootNode()); };
	inline uint32_t getCount() { return count;}
	inline uint32_t getNodeCount() { return nodeCount; }
	uint64_t minMemoryUsage();
	std::map<uint16_t, uint32_t> branchingFactorCount();
	std::map<std::string, uint32_t> nodeStringCount();
	inline void nodeStringLenNodeCount(uint32_t * cumStrLen, uint32_t * nodeCount) {
		if (root) nodeStringLenNodeCountRecurse(root, cumStrLen, nodeCount);
	}
	inline uint32_t getDepth() { return (root ? getDepthRecurse(root, 0) : 0); }
	inline std::string getLongestString() { return (root ? getLongestStringRecurse(root, "") : ""); }
	
	Node * getRootNode() { return root;}
	static bool isValidString(const std::string & str);
	bool checkParentChildRelations();
};

template <typename ItemType, template <typename> class TNode >
uint64_t RadixCompletionTrie<ItemType, TNode>::minSizeRecurse(Node * node) {
	uint64_t size = 0;
	if (node) {
		// (uint8_t + displacement = 4 byte = uint32 + own character -1 as the first will be saved in parent + \0 + length-info + metadata pointer
		size += node->children.size()*(1+4) + (node->c.size()-1) + 1 + 1  + (node->partOfEnd ? 4 : 0); 
		if (node->children.size() > 0) {
			for(class std::unordered_map<uint32_t, Node*>::iterator i = node->children.begin(); i != node->children.end(); ++i) {
				size += minSizeRecurse(i->second); 
			}
		}
	}
	return size;
}

template <typename ItemType, template <typename> class TNode >
std::map<uint16_t, uint32_t> RadixCompletionTrie<ItemType, TNode>::branchingFactorCount() {
	std::map<uint16_t, uint32_t> m;
	branchingFactorCountRecurse(root, &m);
	return m;
}

template <typename ItemType, template <typename> class TNode >
void RadixCompletionTrie<ItemType, TNode>::branchingFactorCountRecurse(Node * node, std::map<uint16_t, uint32_t> * m) {
	if (node) {
		if (node->children.size() > 0) {
			if (m->count(node->children.size()) == 0) {
				(*m)[node->children.size()] = 0;
			}
			(*m)[node->children.size()] += 1;
			for(class std::unordered_map<uint32_t, Node*>::iterator i = node->children.begin(); i != node->children.end(); ++i) {
				branchingFactorCountRecurse(i->second, m); 
			}
		}
	}
}

template <typename ItemType, template <typename> class TNode >
std::string RadixCompletionTrie<ItemType, TNode>::getLongestStringRecurse(Node * node, std::string s) {
	if (node) {
		std::string mymaxstr = s;
		if (node->partOfEnd) mymaxstr.append(node->c);
		
		std::string cs;
		
		for(class std::unordered_map<uint32_t, Node*>::iterator i = node->children.begin(); i != node->children.end(); ++i) {
			cs = getLongestStringRecurse(i->second, s+node->c);
			if ( cs.length() > mymaxstr.length()) { 
				mymaxstr = cs;
			}
		}
		return mymaxstr;
	}
	return s;
}

template <typename ItemType, template <typename> class TNode >
std::map<std::string, uint32_t> RadixCompletionTrie<ItemType, TNode>::nodeStringCount() {
	std::map<std::string, uint32_t> m;
	nodeStringCountRecurse(root, &m);
	return m;
}

template <typename ItemType, template <typename> class TNode >
void RadixCompletionTrie<ItemType, TNode>::nodeStringCountRecurse(Node * node, std::map<std::string, uint32_t> * m) {
	if (node) {
		if (m->count(node->c) == 0) {
			(*m)[node->c] = 0;
		}
		(*m)[node->c] += 1;
		for(class std::unordered_map<uint32_t, Node*>::iterator i = node->children.begin(); i != node->children.end(); ++i) {
			nodeStringCountRecurse(i->second, m); 
		}
	}
}

template <typename ItemType, template <typename> class TNode >
void RadixCompletionTrie<ItemType, TNode>::nodeStringLenNodeCountRecurse(Node * node, uint32_t * cumStrLen, uint32_t * nodeCount) {
	if (node) {
		*cumStrLen += node->c.size();
		*nodeCount += 1;
		for(class std::unordered_map<uint32_t, Node*>::iterator i = node->children.begin(); i != node->children.end(); ++i) {
			nodeStringLenNodeCountRecurse(i->second, cumStrLen, nodeCount); 
		}
	}
}



#endif