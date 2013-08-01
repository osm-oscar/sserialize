#ifndef SSERIALIZE_GENERALIZED_TRIE_MULTI_PASS_TRIE_H
#define SSERIALIZE_GENERALIZED_TRIE_MULTI_PASS_TRIE_H
#include "BaseTrie.h"

namespace sserialize {
namespace GeneralizedTrie {

struct GeneralizedTrieCreatorConfig {
GeneralizedTrieCreatorConfig() :
		trieList(0),
		mergeIndex(true),
		maxPrefixMergeCount(-1),
		maxSuffixMergeCount(-1),
		deleteRootTrie(false),
		nodeType(Static::TrieNode::T_SIMPLE)
		{};
	GeneralizedTrieCreatorConfig(
		std::deque<uint8_t>* trieList,
		bool indirectIndex,
		bool mergeIndex,
		std::set<uint8_t> levelsWithoutFullIndex,
		int32_t maxPrefixMergeCount,
		int32_t maxSuffixMergeCount,
		bool deleteRootTrie,
		int8_t fixedBitWidth,
		Static::TrieNode::Types nodeType) :
			trieList(trieList),
			mergeIndex(mergeIndex),
			levelsWithoutFullIndex(levelsWithoutFullIndex),
			maxPrefixMergeCount(maxPrefixMergeCount),
			maxSuffixMergeCount(maxSuffixMergeCount),
			deleteRootTrie(deleteRootTrie),
			nodeType(nodeType)
			{};
	std::deque<uint8_t>* trieList;
	ItemIndexFactory indexFactory;
	bool mergeIndex;
	std::set<uint8_t> levelsWithoutFullIndex;
	//if the cumulated count of children itemindices is smaller than this threshold, then don't create a full index
	int32_t maxPrefixMergeCount;
	int32_t maxSuffixMergeCount;
	bool deleteRootTrie;
	Static::TrieNode::Types nodeType;
	bool isValid() {
		return nodeType == Static::TrieNode::T_SIMPLE || nodeType == Static::TrieNode::T_COMPACT;
	}
	UByteArrayAdapter trieListAdapter() {
		UByteArrayAdapter adap(trieList);
		adap.setPutPtr(trieList->size());
		return adap;
	}
};

class MultiPassTrie: public BaseTrie< std::vector<uint32_t> > {
public:
	typedef BaseTrie< std::vector<uint32_t> > MyBaseClass;
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
	bool checkTrieEqualityRecurse(Node * curNode, Static::TrieNode curStaticNode, std::string firstChar);
	bool checkIndexEqualityRecurse(Node* curNode, sserialize::Static::TrieNode curStaticNode, sserialize::Static::GeneralizedTrie& staticTrie, StringCompleter::SupportedQuerries sqtype);
private:
	MultiPassTrie( const MultiPassTrie & other);
	MultiPassTrie & operator=(const MultiPassTrie & other);
public:
	MultiPassTrie();
	MultiPassTrie(bool caseSensitive, bool suffixTrie);
	virtual ~MultiPassTrie();
	void swap(MyBaseClass & baseTrie);
	
	void createStaticTrie(GeneralizedTrieCreatorConfig & config);
	
	bool checkTrieEquality(GeneralizedTrieCreatorConfig config, sserialize::Static::GeneralizedTrie staticTrie);
	bool checkIndexEquality(GeneralizedTrieCreatorConfig config, Static::GeneralizedTrie staticTrie, StringCompleter::SupportedQuerries sqtype);
	
};

}}//end namespace

#endif