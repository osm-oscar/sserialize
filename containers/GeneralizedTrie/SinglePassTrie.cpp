#include <sserialize/containers/GeneralizedTrie/SinglePassTrie.h>

namespace sserialize {
namespace GeneralizedTrie {


SinglePassTrie::SinglePassTrie() {}

SinglePassTrie::SinglePassTrie(bool caseSensitive, bool suffixTrie) : MyBaseClass(caseSensitive, suffixTrie) {}

SinglePassTrie::~SinglePassTrie() {}

void SinglePassTrie::swap(MyBaseClass::MyBaseClass & other) {
	MyBaseClass::swap(other);
}

//The prefix/substr index is constructed out of the prefix/substr index ptrs of our children and our own exact/suffix indices. handle those first

#define WINDOWED_ARRAY_MERGE
#ifdef WINDOWED_ARRAY_MERGE
bool SinglePassTrie::handleNodeIndices(Node * node, GeneralizedTrieCreatorConfig & config, Static::TrieNodeCreationInfo& nodeInfo) {
	if (!node)
		return false;
	
	nodeInfo.mergeIndex = false;
	int idxTypes = Static::TrieNodePrivate::IT_NONE;
	bool ok = true;
	
	if (node->exactValues.size()) {
		idxTypes |= Static::TrieNodePrivate::IT_EXACT;
		nodeInfo.exactIndexPtr = config.indexFactory.addIndex(node->exactValues, &ok);
	}
	if (node->subStrValues.size()) {
		idxTypes |= Static::TrieNodePrivate::IT_SUFFIX;
		nodeInfo.suffixIndexPtr = config.indexFactory.addIndex(node->subStrValues, &ok);
	}
	
	if (node->children.size()) {
		{
			std::vector<ItemSetContainer> prefixIndices;
			prefixIndices.reserve(node->children.size()+1);
			if (idxTypes & Static::TrieNodePrivate::IT_EXACT)
					prefixIndices.push_back(node->exactValues);
			for(ChildNodeIterator it(node->children.begin()), end(node->children.end()); it != end; ++it) {
				prefixIndices.push_back( it->second->exactValues );
			}
			node->exactValues = ItemSetContainer::uniteSortedInPlace(prefixIndices.begin(), prefixIndices.end());
			nodeInfo.prefixIndexPtr = config.indexFactory.addIndex(node->exactValues, &ok);
			idxTypes |= Static::TrieNodePrivate::IT_PREFIX;
		}
		if (isSuffixTrie()) {
			std::vector<ItemSetContainer> subStrIndices;
			subStrIndices.reserve(node->children.size()+1);
			if (idxTypes & Static::TrieNodePrivate::IT_SUFFIX)
				subStrIndices.push_back(node->subStrValues);

			for(ChildNodeIterator it(node->children.begin()), end(node->children.end()); it != end; ++it) {
				subStrIndices.push_back( it->second->subStrValues );
			}
			node->subStrValues = ItemSetContainer::uniteSortedInPlace(subStrIndices.begin(), subStrIndices.end());
// 			std::vector<uint32_t> subStrIndex;
// 			mergeSortedContainer(subStrIndex, node->subStrValues, node->exactValues);
// 			nodeInfo.suffixPrefixIndexPtr = config.indexFactory.addIndex(subStrIndex, &ok);
			nodeInfo.suffixPrefixIndexPtr = config.indexFactory.addIndex(node->subStrValues, &ok);
			idxTypes |= Static::TrieNodePrivate::IT_SUFFIX_PREFIX;
		}
	}
	else {
		if (idxTypes & Static::TrieNodePrivate::IT_EXACT) {
			nodeInfo.prefixIndexPtr = nodeInfo.exactIndexPtr;
			idxTypes |= Static::TrieNodePrivate::IT_PREFIX;
		}
		if (idxTypes & Static::TrieNodePrivate::IT_SUFFIX) {
			nodeInfo.suffixPrefixIndexPtr = nodeInfo.suffixIndexPtr;
			idxTypes |= Static::TrieNodePrivate::IT_SUFFIX_PREFIX;
		}
		else if (isSuffixTrie()) {
			idxTypes |= Static::TrieNodePrivate::IT_SUFFIX_PREFIX;
			nodeInfo.suffixPrefixIndexPtr = nodeInfo.prefixIndexPtr;
		}
	}
	nodeInfo.indexTypes = (Static::TrieNodePrivate::IndexTypes) idxTypes;
	
	node->deleteTemporalPrivateStorage();
	return ok;
}
#else
bool SinglePassTrie::handleNodeIndices(Node * node, GeneralizedTrieCreatorConfig & config, Static::TrieNodeCreationInfo& nodeInfo) {
	if (!node)
		return false;
	
	nodeInfo.mergeIndex = false;
	int idxTypes = Static::TrieNodePrivate::IT_NONE;
	bool ok = true;
	
	OffsetType exactIndexOffset = 0;
	OffsetType suffixIndexOffset = 0;
	OffsetType prefixIndexOffset = 0;
	OffsetType subStrIndexOffset = 0;
	if (node->exactValues.size()) {
		idxTypes |= Static::TrieNodePrivate::IT_EXACT;
		nodeInfo.exactIndexPtr = config.indexFactory.addIndex(node->exactValues, &ok, &exactIndexOffset);
	}
	if (node->subStrValues.size()) {
		idxTypes |= Static::TrieNodePrivate::IT_SUFFIX;
		nodeInfo.suffixIndexPtr = config.indexFactory.addIndex(node->subStrValues, &ok, &suffixIndexOffset);
	}
	
	if (node->children.size()) {
		std::vector<uint64_t> prefixIndexOffsets, subStrIndexOffsets;
		if (idxTypes & Static::TrieNodePrivate::IT_EXACT)
			prefixIndexOffsets.push_back(exactIndexOffset);
		if (idxTypes & Static::TrieNodePrivate::IT_SUFFIX)
			subStrIndexOffsets.push_back(suffixIndexOffset);
		for(ChildNodeIterator it(node->children.begin()), end(node->children.end()); it != end; ++it) {
			ChildTPNS * ctpns = static_cast<ChildTPNS*>(it->second->temporalPrivateStorage);
			prefixIndexOffsets.push_back( ctpns->prefixIndexPtr );
			subStrIndexOffsets.push_back( ctpns->subStrIndexPtr );
		}
		nodeInfo.prefixIndexPtr = config.indexFactory.addMergedIndex(prefixIndexOffsets.begin(), prefixIndexOffsets.end(), prefixIndexOffset);
		idxTypes |= Static::TrieNodePrivate::IT_PREFIX;
		if (isSuffixTrie()) {
			nodeInfo.suffixPrefixIndexPtr = config.indexFactory.addMergedIndex(subStrIndexOffsets.begin(), subStrIndexOffsets.end(), subStrIndexOffset);
			idxTypes |= Static::TrieNodePrivate::IT_SUFFIX_PREFIX;
		}
	}
	else {
		if (idxTypes & Static::TrieNodePrivate::IT_EXACT) {
			nodeInfo.prefixIndexPtr = nodeInfo.exactIndexPtr;
			prefixIndexOffset = exactIndexOffset;
			idxTypes |= Static::TrieNodePrivate::IT_PREFIX;
		}
		if (idxTypes & Static::TrieNodePrivate::IT_SUFFIX) {
			nodeInfo.suffixPrefixIndexPtr = nodeInfo.suffixIndexPtr;
			subStrIndexOffset = suffixIndexOffset;
			idxTypes |= Static::TrieNodePrivate::IT_SUFFIX_PREFIX;
		}
		else if (isSuffixTrie()) {
			idxTypes |= Static::TrieNodePrivate::IT_SUFFIX_PREFIX;
			nodeInfo.suffixPrefixIndexPtr = nodeInfo.prefixIndexPtr;
			subStrIndexOffset = prefixIndexOffset;
		}
	}
	nodeInfo.indexTypes = (Static::TrieNodePrivate::IndexTypes) idxTypes;
	
	node->deleteTemporalPrivateStorage();
	node->temporalPrivateStorage = new ChildTPNS(prefixIndexOffset, subStrIndexOffset);
	return ok;
}
#endif


void SinglePassTrie::createStaticTrie(GeneralizedTrieCreatorConfig& config) {

	std::cout << "SinglePassTrie::createStaticTrie: consistencyCheck...";
	if (consistencyCheck()) {
		std::cout << "OK" << std::endl;
	}
	else {
		std::cout << "Failed" << std::endl;
	}

	if (m_root) {
		std::cout << "Fixing possible problems" << std::endl;
		MyBaseClass::trieSerializationProblemFixer();
		std::cout << "done" << std::endl;
		if (!consistencyCheck()) {
			std::cout << "Fixing problems caused problems broke trie!" << std::endl;
			return;
		}
	}
	
	sserialize::Static::GeneralizedTrie::HeaderInfo headerInfo;
	headerInfo.version = 1;
	headerInfo.depth = getDepth();
	headerInfo.trieOptions = Static::GeneralizedTrie::STO_NONE;
	if (m_caseSensitive)
		headerInfo.trieOptions |= Static::GeneralizedTrie::STO_CASE_SENSITIVE;
	if (m_isSuffixTrie)
		headerInfo.trieOptions |= Static::GeneralizedTrie::STO_SUFFIX;
	headerInfo.nodeType = config.nodeType;
	headerInfo.longestString = 0;
	headerInfo.numberOfNodes = m_nodeCount;
	headerInfo.nodeDataSize = 0;

	if (config.nodeType == Static::TrieNode::T_SIMPLE) {
		serializeTrieBottomUp<Static::SimpleStaticTrieCreationNode>(config);
	}
	else if (config.nodeType == Static::TrieNode::T_COMPACT) {
		serializeTrieBottomUp<Static::CompactStaticTrieCreationNode>(config);
	}
	else if (config.nodeType == Static::TrieNode::T_LARGE_COMPACT) {
		serializeTrieBottomUp<Static::LargeCompactTrieNodeCreator>(config);
	}
	else {
		std::cerr << "Unsupported node type" << std::endl;
		return;
	}

	{
		std::deque<uint8_t> headerData;
		sserialize::UByteArrayAdapter header(&headerData, false);
		headerInfo.nodeDataSize = config.trieList->size();
		header << headerInfo;
		prependToDeque(headerData, *(config.trieList));
	}
	{
		std::deque<uint8_t> headerData;
		sserialize::UByteArrayAdapter header(&headerData, false);
		header << sserialize::Static::StringCompleter::HeaderInfo(sserialize::Static::StringCompleter::T_TRIE, config.trieList->size());
		prependToDeque(headerData, *(config.trieList));
	}

	if (config.deleteRootTrie) {
		delete m_root;
		m_root = 0;
		m_count = 0;
		m_nodeCount = 0;
	}
}

}}//end namespace
