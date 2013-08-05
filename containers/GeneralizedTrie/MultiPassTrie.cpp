#include <sserialize/containers/GeneralizedTrie/MultiPassTrie.h>

namespace sserialize {
namespace GeneralizedTrie {


MultiPassTrie::MultiPassTrie() {}
MultiPassTrie::MultiPassTrie(bool caseSensitive, bool suffixTrie) : MyBaseClass(caseSensitive, suffixTrie) {}
MultiPassTrie::~MultiPassTrie() {}

void MultiPassTrie::swap( MyBaseClass::MyBaseClass & baseTrie ) {
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

}}//end namespace