#ifndef SSERIALIZE_GENERALIZED_TRIE_MULTI_PASS_TRIE_H
#define SSERIALIZE_GENERALIZED_TRIE_MULTI_PASS_TRIE_H
#include <sserialize/containers/GeneralizedTrie/SerializableTrie.h>

namespace sserialize {
namespace GeneralizedTrie {

class MultiPassTrie: public SerializableTrie< std::vector<uint32_t> > {
public:
	typedef SerializableTrie< std::vector<uint32_t> > MyBaseClass;
protected:
	class TemporalPrivateNodeStorage: public Node::TemporalPrivateStorage {
	public:
		TemporalPrivateNodeStorage() : TemporalPrivateStorage(), childrenCompleted(0), prefixComparisonCount(0), suffixComparisonCount(0) {}
		virtual ~TemporalPrivateNodeStorage() {}
		uint32_t childrenCompleted;
		std::map<uint32_t, uint32_t> remappedPrefixIdxIds;
		std::map<uint32_t, uint32_t> remappedSuffixIdxIds;
		std::deque<uint32_t> staticChildPointers;
		Node::ItemSetContainer prefixIndex; //same as curNode->insertExactValuesRecursive()
		Node::ItemSetContainer suffixIndex; //same as curNode->insertSubStrValuesRecursive()
		uint64_t prefixComparisonCount; //number of comparissons to create prefixIndex
		uint64_t suffixComparisonCount; // number of comparissons to create suffixIndex
	};
	
	struct NodeIndexSets {
		typedef std::vector<uint32_t> StorageType;
		NodeIndexSets() : mergeIndex(true), hasExactValues(false), hasPrefixValues(false), hasSuffixValues(false), hasSuffixPrefixValues(false) {}
		bool mergeIndex;
		bool hasExactValues;
		bool hasPrefixValues;
		bool hasSuffixValues;
		bool hasSuffixPrefixValues;
		std::vector<ItemIdType> exactValues;
		//same as prefixIndex from above minus exactValues
		std::vector<ItemIdType> prefixValues;
		//contains only the REAL suffix values
		std::vector<ItemIdType> suffixValues;
		//same as suffixIndex from above minus (suffixValues + exactValues);
		//BUT: if indirectIndex is set and node gets a full index, then this contains ALL, including suffixValue+prefixValues  
		std::vector<ItemIdType> suffixPrefixValues;
	};
private: //static trie creation functions
	bool handleNodeIndices(Node* curNode, uint8_t curLevel, bool delStorage, GeneralizedTrieCreatorConfig& config, Static::TrieNodeCreationInfo& nodeInfo);
	bool getItemIdsForNode(Node* curNode, uint8_t curLevel, const GeneralizedTrieCreatorConfig& config, NodeIndexSets & nodeIndexSet);

	void mergeChildrenIndices(uint32_t start, uint32_t end, const std::deque<Node*>& nodes, Node::ItemSetContainer & prefixIndex, Node::ItemSetContainer& suffixIndex, uint64_t& prefixComparisonCount, uint64_t& suffixComparisonCount);
	void mergeChildrenIndices(Node * curNode, NodeIndexSets & idxSet, uint64_t & prefixComparisonCount, uint64_t & suffixComparisonCount);

	template<class StaticTrieNodeT>
	void serializeTrieBottomUp(GeneralizedTrieCreatorConfig & config);
private:
	MultiPassTrie( const MultiPassTrie & other);
	MultiPassTrie & operator=(const MultiPassTrie & other);
public:
	MultiPassTrie();
	MultiPassTrie(bool caseSensitive, bool suffixTrie);
	virtual ~MultiPassTrie();
	void swap(MyBaseClass::MyBaseClass & baseTrie);
	
	void createStaticTrie(GeneralizedTrieCreatorConfig & config);
};

/**
 * This function serializes the trie in bottom-up fashion
 *
 */

template<class StaticTrieNodeT>
void
MultiPassTrie::
serializeTrieBottomUp(GeneralizedTrieCreatorConfig & config) {
	uint32_t depth = getDepth();
	if (depth == 0) {
		std::cout << "Trie is empty" << std::endl;
		return;
	}

	if (!consistencyCheck()) {
		std::cout << "Trie is broken!" << std::endl;
		return;
	}

	ProgressInfo progressInfo;
	progressInfo.begin(m_nodeCount);
	uint32_t finishedNodeCount = 0;
	std::cout << "Serializing Trie:" << std::endl;
	
	std::deque< std::deque< MultiTrieNode<unsigned int>* >  > trieNodeQue;
	if (m_nodeCount < 200 * 1000 * 1000)
		rootNode()->addNodesSorted(0, trieNodeQue);
	else
		trieNodeQue.resize(depth);
	for(int curLevel = depth-1; curLevel >= 0; curLevel--) {
		std::deque< MultiTrieNode<unsigned int>* > & nodeQue = trieNodeQue.at(curLevel);

		if (nodeQue.size() == 0)
			rootNode()->addNodesSortedInLevel(curLevel, 0, nodeQue);

		uint32_t curNodeCount = 0;
		for(std::deque< MultiTrieNode<unsigned int>* >::const_reverse_iterator it = nodeQue.rbegin(); it != nodeQue.rend(); ++it) {
			MultiTrieNode<unsigned int> * curNode = *it;
			
			Static::TrieNodeCreationInfo nodeInfo;
			if (curNode->c.size()) {
				std::string::const_iterator nodeStrIt(curNode->c.begin());
				utf8::next<std::string::const_iterator>(nodeStrIt, curNode->c.cend());
				nodeInfo.nodeStr = std::string(nodeStrIt, curNode->c.cend());
			}
			curNode->insertSortedChildChars(nodeInfo.childChars);

			if (nodeInfo.childChars.size() != curNode->children.size()) {
				std::cout << "Error:  nodeInfo.childChars.size()=" << nodeInfo.childChars.size() << "!= curNode->children.size()=" << curNode->children.size() << std::endl;
			}
			
			//Get our child ptrs
			if (curNode->temporalPrivateStorage) {
				std::deque<uint32_t> tmp;
				TemporalPrivateNodeStorage * tmpStorage = dynamic_cast<TemporalPrivateNodeStorage*>(curNode->temporalPrivateStorage);
				tmpStorage->staticChildPointers.swap(tmp);

				//now do the remapping for the relative ptrs. stored ptrs offsets from the end of the list
				for(size_t i = 0; i < tmp.size(); i++) {
					nodeInfo.childPtrs.push_back(config.trieList->size() - tmp[i]);
				}
			}
			
			if (nodeInfo.childChars.size() != nodeInfo.childPtrs.size()) {
				std::cout << "Error: (childChars.size() != childPtrs.size()) = (" << nodeInfo.childChars.size() << " != " << nodeInfo.childPtrs.size() << ")" << std::endl;
				curNode->dump();
			}

			if (!handleNodeIndices(curNode, curLevel, false, config, nodeInfo)) {
				std::cout <<  "Failed to handle node Item index" << std::endl;
			}

			unsigned int err =  StaticTrieNodeT::prependNewNode(nodeInfo, *(config.trieList));
			if (StaticTrieNodeT::isError(err)) {
				std::cout << "ERROR: " << StaticTrieNodeT::errorString(err) << std::endl;
			}

			//Add ourself to the child ptrs of our parent
			if (curNode->parent) {
				TemporalPrivateNodeStorage * parentTempStorage = dynamic_cast<TemporalPrivateNodeStorage*>(curNode->parent->temporalPrivateStorage);
				if (!parentTempStorage) {
					parentTempStorage = new TemporalPrivateNodeStorage();
					curNode->parent->temporalPrivateStorage = parentTempStorage;
				}
				uint32_t nodeOffset = config.trieList->size();
				parentTempStorage->staticChildPointers.push_front(nodeOffset);
			}

			
			//delete children if config says so
			if (config.deleteRootTrie) {
				curNode->deleteChildren();
			}
			else {//Delete all child temporal node storages as those are not needed anymore
				for(Node::ChildNodeIterator it = curNode->children.begin(); it != curNode->children.end(); ++it) {
					it->second->deleteTemporalPrivateStorage();
				}
			}

			curNodeCount++;
			finishedNodeCount++;
			progressInfo(finishedNodeCount, "BaseTrie::Bottom-up-Serialization");
		}
		trieNodeQue.pop_back();
	}
	progressInfo.end("BaseTrie::Bottom-up-Serialization");
	rootNode()->deleteTemporalPrivateStorage();
}

}}//end namespace

#endif