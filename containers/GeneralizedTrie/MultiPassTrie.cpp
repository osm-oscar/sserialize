#include <sserialize/containers/GeneralizedTrie/MultiPassTrie.h>

namespace sserialize {
namespace GeneralizedTrie {


MultiPassTrie::MultiPassTrie() {}
MultiPassTrie::MultiPassTrie(bool caseSensitive, bool suffixTrie) : MyBaseClass(caseSensitive, suffixTrie) {}
MultiPassTrie::~MultiPassTrie() {}

void MultiPassTrie::swap( MyBaseClass & baseTrie ) {
	MyBaseClass::swap(baseTrie);
}

void MultiPassTrie::createStaticTrie(GeneralizedTrieCreatorConfig& config) {

	std::cout << "BaseTrie<IndexStorageContainer>::createStaticTrie: consistencyCheck...";
	if (consistencyCheck()) {
		std::cout << "OK" << std::endl;
	}
	else {
		std::cout << "Failed" << std::endl;
	}

	if (m_root) {
		std::cout << "Compactifying...";
		compactify(m_root);
		trieSerializationProblemFixer(m_root);
		std::cout << "done" << std::endl;
		if (!consistencyCheck()) {
			std::cout << "Compactifying broke trie!" << std::endl;
			return;
		}
	}

	if (config.nodeType == Static::TrieNode::T_SIMPLE) {
		serializeTrieBottomUp<Static::SimpleStaticTrieCreationNode>(config);
	}
	else if (config.nodeType == Static::TrieNode::T_COMPACT) {
		serializeTrieBottomUp<Static::CompactStaticTrieCreationNode>(config);
	}

	//Add the stats
	uint8_t trieType = 0;
	trieType |= Static::GeneralizedTrie::STO_NORMALIZED;
	if (m_caseSensitive)
		trieType |= Static::GeneralizedTrie::STO_CASE_SENSITIVE;
	if (m_isSuffixTrie)
		trieType |= Static::GeneralizedTrie::STO_SUFFIX;
		
	uint8_t nodeType = config.nodeType;
	uint16_t longestString = 0;
	uint16_t depth = getDepth();
	std::deque<uint8_t> header;
	uint8_t strCompleterType = sserialize::Static::StringCompleter::T_TRIE;
	
	Static::StringCompleter::addHeader(strCompleterType, header);
	
	Static::GeneralizedTrie::addHeader(trieType, nodeType, longestString, depth, header);
	prependToDeque(header, *(config.trieList));

	if (config.deleteRootTrie) {
		delete m_root;
		m_root = 0;
		m_count = 0;
		m_nodeCount = 0;
	}
}

void
MultiPassTrie::
mergeChildrenIndices(uint32_t start, uint32_t end, const std::deque< Node* >& nodes, Node::ItemSetContainer & prefixIndex, Node::ItemSetContainer& suffixIndex, uint64_t& prefixComparisonCount, uint64_t& suffixComparisonCount) {
	if (start == end) {
		TemporalPrivateNodeStorage * privateStorage = dynamic_cast<TemporalPrivateNodeStorage*>(nodes.at(start)->temporalPrivateStorage);
		prefixComparisonCount += privateStorage->prefixComparisonCount;
		suffixComparisonCount += privateStorage->suffixComparisonCount;
		prefixIndex.swap(privateStorage->prefixIndex);
		suffixIndex.swap(privateStorage->suffixIndex);
	}
	else {
		uint32_t mid = start + (end-start)/2;

		//do the recursive merge
		Node::ItemSetContainer tmpPIdx1, tmpSIdx1, tmpPIdx2, tmpSIdx2;
		mergeChildrenIndices(start, mid, nodes, tmpPIdx1, tmpSIdx1, prefixComparisonCount, suffixComparisonCount);
		mergeChildrenIndices(mid+1, end, nodes, tmpPIdx2, tmpSIdx2, prefixComparisonCount, suffixComparisonCount);

		mergeSortedContainer(prefixIndex, tmpPIdx1, tmpPIdx2, prefixComparisonCount);
		mergeSortedContainer(suffixIndex, tmpSIdx1, tmpSIdx2, suffixComparisonCount);
	}
}


/** @return returns the amount of comparissons to create this index 
 *  @param result sorted range of all children item ids
 *  
 *  This will delete all full indices in the children nodes as those are not needed anymore
 */
void
MultiPassTrie::mergeChildrenIndices(Node * curNode, NodeIndexSets & idxSet, uint64_t & prefixComparisonCount, uint64_t & suffixComparisonCount) {
	std::deque< Node *> childPtrs = curNode->getSortedChildPtrs();
	if (childPtrs.size() > 0) {
		Node::ItemSetContainer pV, spV;
		mergeChildrenIndices(0, childPtrs.size()-1, childPtrs, pV, spV, prefixComparisonCount, suffixComparisonCount);
		mergeSortedContainer(idxSet.prefixValues, idxSet.prefixValues, pV);
		mergeSortedContainer(idxSet.suffixPrefixValues, idxSet.suffixPrefixValues, spV);
	}
	mergeSortedContainer(idxSet.prefixValues, idxSet.prefixValues, curNode->exactValues, prefixComparisonCount);
	mergeSortedContainer(idxSet.suffixPrefixValues, idxSet.suffixPrefixValues, curNode->exactValues, suffixComparisonCount);
	mergeSortedContainer(idxSet.suffixPrefixValues, idxSet.suffixPrefixValues, curNode->subStrValues, suffixComparisonCount);


	//Cache indices
	TemporalPrivateNodeStorage * privateStorage = dynamic_cast<TemporalPrivateNodeStorage*>(curNode->temporalPrivateStorage);
	if (!privateStorage) {
		privateStorage = new TemporalPrivateNodeStorage();
		curNode->temporalPrivateStorage = privateStorage;
	}
	privateStorage->prefixComparisonCount = prefixComparisonCount;
	privateStorage->suffixComparisonCount = suffixComparisonCount;
	privateStorage->prefixIndex = Node::ItemSetContainer(idxSet.prefixValues.begin(), idxSet.prefixValues.end());
	privateStorage->suffixIndex = Node::ItemSetContainer(idxSet.suffixPrefixValues.begin(), idxSet.suffixPrefixValues.end());

	for(size_t i = 0; i < childPtrs.size(); i++) {
		TemporalPrivateNodeStorage * privateStorage = dynamic_cast<TemporalPrivateNodeStorage*>(childPtrs[i]->temporalPrivateStorage);
		if (privateStorage) {
			privateStorage->prefixIndex = Node::ItemSetContainer();
			privateStorage->suffixIndex = Node::ItemSetContainer();
		}
	}
}

/** This function creates all necessary indices. */
bool MultiPassTrie::getItemIdsForNode(
    Node* curNode, uint8_t curLevel, const GeneralizedTrieCreatorConfig& config, NodeIndexSets & nodeIndexSet) {

	nodeIndexSet.hasExactValues = (curNode->exactValues.size() > 0);
	nodeIndexSet.hasSuffixValues = (m_isSuffixTrie && (curNode->subStrValues.size() > 0 || curNode->exactValues.size() > 0) );
	nodeIndexSet.exactValues = typename NodeIndexSets::StorageType(curNode->exactValues.begin(), curNode->exactValues.end());
	nodeIndexSet.suffixValues = typename NodeIndexSets::StorageType(curNode->subStrValues.begin(), curNode->subStrValues.end());
	if (!config.mergeIndex)
		mergeSortedContainer(nodeIndexSet.suffixValues, nodeIndexSet.suffixValues, curNode->exactValues);
	
	TemporalPrivateNodeStorage * curNodePrivateStorage = dynamic_cast<TemporalPrivateNodeStorage*>(curNode->temporalPrivateStorage);
	if (!curNodePrivateStorage) {
		curNodePrivateStorage = new TemporalPrivateNodeStorage();
		curNode->temporalPrivateStorage = curNodePrivateStorage;
	}
	uint64_t prefixComparisonCount = 0;
	uint64_t suffixComparisonCount = 0;
	mergeChildrenIndices(curNode, nodeIndexSet, prefixComparisonCount, suffixComparisonCount);

	bool clearPrefixIndexSet = (((int64_t)prefixComparisonCount < config.maxPrefixMergeCount) || config.levelsWithoutFullIndex.count(curLevel) > 0);
	if (!clearPrefixIndexSet) { //full prefixIndex
		nodeIndexSet.hasPrefixValues = true;
		curNodePrivateStorage->prefixComparisonCount = 0;
		prefixComparisonCount = 0;
		if (config.mergeIndex)
			inplaceDiffSortedContainer(nodeIndexSet.prefixValues, nodeIndexSet.exactValues);
	}
	int64_t totalSuffixComparisonCount = (int64_t)(suffixComparisonCount + (clearPrefixIndexSet ? prefixComparisonCount : nodeIndexSet.prefixValues.size()));
	if (totalSuffixComparisonCount < config.maxSuffixMergeCount || config.levelsWithoutFullIndex.count(curLevel) > 0) { // no full index
		nodeIndexSet.suffixPrefixValues = typename NodeIndexSets::StorageType();
	}
	else {
		nodeIndexSet.hasSuffixPrefixValues = true;
		curNodePrivateStorage->suffixComparisonCount = 0;
		if (config.mergeIndex) {
			if (!clearPrefixIndexSet)
				inplaceDiffSortedContainer(nodeIndexSet.suffixPrefixValues, nodeIndexSet.exactValues);
			
			inplaceDiffSortedContainer(nodeIndexSet.suffixPrefixValues, nodeIndexSet.prefixValues);
			inplaceDiffSortedContainer(nodeIndexSet.suffixPrefixValues, nodeIndexSet.suffixValues);
		}
	}

	if (clearPrefixIndexSet)
		nodeIndexSet.prefixValues = typename NodeIndexSets::StorageType();

	return true;
}

/** Creates all necessary item indices, put them into the ItemFactory, sets the index pointers in nodeInfo and sets the IndexTypes in nodeInfo */
bool MultiPassTrie::handleNodeIndices(
    Node* curNode, uint8_t curLevel, bool delStorage, GeneralizedTrieCreatorConfig& config, sserialize::Static::TrieNodeCreationInfo& nodeInfo) {
    
    nodeInfo.mergeIndex = config.mergeIndex;

	NodeIndexSets nodeIdcs;

	getItemIdsForNode(curNode, curLevel, config, nodeIdcs);

	bool ok;
	uint8_t indexTypes = 0;
	if (nodeIdcs.hasExactValues) {
		nodeInfo.exactIndexPtr = config.indexFactory.addIndex(nodeIdcs.exactValues, &ok);
		if (!ok) {
			std::cerr << "FATAL: Failed to add Index to index factory" << std::endl;
			nodeInfo.exactIndexPtr = 0xFFFFFFFF;
		}
		else
			indexTypes |= Static::TrieNodePrivate::IT_EXACT;
	}
	if (nodeIdcs.hasPrefixValues) {
		nodeInfo.prefixIndexPtr = config.indexFactory.addIndex(nodeIdcs.prefixValues, &ok);
		if (!ok) {
			std::cerr << "FATAL: Failed to add Index to index factory" << std::endl;
			nodeInfo.prefixIndexPtr = 0xFFFFFFFF;
		}
		else
			indexTypes |= Static::TrieNodePrivate::IT_PREFIX;
	}
	if (nodeIdcs.hasSuffixValues) {
		nodeInfo.suffixIndexPtr = config.indexFactory.addIndex(nodeIdcs.suffixValues, &ok);
		if (!ok) {
			std::cerr << "FATAL: Failed to add Index to index factory" << std::endl;
			nodeInfo.suffixIndexPtr = 0xFFFFFFFF;
		}
		else
			indexTypes |= Static::TrieNodePrivate::IT_SUFFIX;
	}
	if (nodeIdcs.hasSuffixPrefixValues) {
		nodeInfo.suffixPrefixIndexPtr = config.indexFactory.addIndex(nodeIdcs.suffixPrefixValues, &ok);
		if (!ok) {
			std::cerr << "FATAL: Failed to add Index to index factory" << std::endl;
			nodeInfo.suffixPrefixIndexPtr = 0xFFFFFFFF;
		}
		else
			indexTypes |= Static::TrieNodePrivate::IT_SUFFIX_PREFIX;
	}

	nodeInfo.indexTypes = (Static::TrieNodePrivate::IndexTypes) indexTypes;

	return true;

}



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

			nodeInfo.nodeStr = curNode->c;
			curNode->insertSortedChildChars(nodeInfo.childChars);
			nodeInfo.charWidth = 1;
			if (nodeInfo.childChars.size() > 0) {
				nodeInfo.charWidth = (nodeInfo.childChars.back() > 0xFF ? 2 : 1);
			}

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

bool
MultiPassTrie::
checkTrieEqualityRecurse(Node * curNode, Static::TrieNode curStaticNode, std::string firstChar) {
	std::string staticNodeStr = firstChar + curStaticNode.str();
	if (curNode->c != staticNodeStr) {
		std::cout << "FATAL: node.str() is wrong:"  << std::endl;
		curNode->dump();
		curStaticNode.dump();
		return false;
	}
	if (curNode->children.size() != curStaticNode.childCount()) {
		std::cout << "FATAL: node.children.size() is wrong:"  << std::endl;
		curNode->dump();
		curStaticNode.dump();
		return false;
	}

	for(size_t i = 0; i < curStaticNode.childCount(); i++) {
		uint16_t childChar = curStaticNode.childCharAt(i);
		std::string childCharStr = "";
		utf8::append(childChar, std::back_inserter(childCharStr));
		if (curNode->children.count(childChar) == 0) {
			std::cout << "FATAL: node.children[" << childChar << "] is wrong:"  << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
		if (!checkTrieEqualityRecurse(curNode->children.at(childChar), curStaticNode.childAt(i), childCharStr) ) {
			return false;
		}
	}
	return true;
}

bool
MultiPassTrie::
checkTrieEquality(GeneralizedTrieCreatorConfig config, Static::GeneralizedTrie staticTrie) {
	return checkTrieEqualityRecurse(rootNode(), staticTrie.getRootNode(), "");
}

bool
MultiPassTrie::
checkIndexEqualityRecurse(Node* curNode, sserialize::Static::TrieNode curStaticNode, sserialize::Static::GeneralizedTrie& staticTrie, StringCompleter::SupportedQuerries sqtype) {

	if (sqtype & sserialize::StringCompleter::SQ_EXACT) {
		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_EXACT));
		if (idx != curNode->exactValues) {
			std::cout << "FATAL: ExactIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
	}
	if (sqtype & sserialize::StringCompleter::SQ_PREFIX) {
		std::set<ItemIdType> destination;
		curNode->insertExactValuesRecursive(destination);
		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_PREFIX));
		if (idx != destination) {
			std::cout << "FATAL: PrefixIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
	}
	if (sqtype & sserialize::StringCompleter::SQ_SUFFIX) {
		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_SUFFIX));
		std::set<ItemIdType> s(curNode->subStrValues.begin(), curNode->subStrValues.end());
		s.insert(curNode->exactValues.begin(), curNode->exactValues.end());
		if (idx != s) {
			std::cout << "FATAL: SuffixIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
	}
	if (sqtype & sserialize::StringCompleter::SQ_SUFFIX_PREFIX) {
		std::set<ItemIdType> destination;
		curNode->insertAllValuesRecursive(destination);

		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_SUFFIX_PREFIX));
		if (idx != destination) {
			std::cout << "FATAL: SuffixPrefixIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
	}

	for(size_t i = 0; i < curStaticNode.childCount(); i++) {
		uint16_t childChar = curStaticNode.childCharAt(i);
		if (curNode->children.count(childChar) == 0) {
			return false;
		}
		if (!checkIndexEqualityRecurse(curNode->children.at(childChar), curStaticNode.childAt(i), staticTrie, sqtype) ) {
			return false;
		}
	}
	return true;
}


bool
MultiPassTrie::
checkIndexEquality(GeneralizedTrieCreatorConfig config, sserialize::Static::GeneralizedTrie staticTrie, sserialize::StringCompleter::SupportedQuerries sqtype) {
	Node * rootNode = m_root;
	return checkIndexEqualityRecurse(rootNode, staticTrie.getRootNode(), staticTrie, sqtype);
}

}}//end namespace