#ifndef SSERIALIZE_GENERALIZED_TRIE_SERIALIZABLE_TRIE_H
#define SSERIALIZE_GENERALIZED_TRIE_SERIALIZABLE_TRIE_H
#include <sserialize/containers/GeneralizedTrie/BaseTrie.h>
#include <sserialize/utility/printers.h>

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
		bool mergeIndex,
		std::set<uint8_t> levelsWithoutFullIndex,
		int32_t maxPrefixMergeCount,
		int32_t maxSuffixMergeCount,
		bool deleteRootTrie,
		Static::TrieNode::Types nodeType) :
			trieList(trieList),
			mergeIndex(mergeIndex),
			levelsWithoutFullIndex(levelsWithoutFullIndex),
			maxPrefixMergeCount(maxPrefixMergeCount),
			maxSuffixMergeCount(maxSuffixMergeCount),
			deleteRootTrie(deleteRootTrie),
			nodeType(nodeType)
			{};
	std::deque<uint8_t> * trieList;
	ItemIndexFactory * indexFactory;
	bool mergeIndex;
	std::set<uint8_t> levelsWithoutFullIndex;
	//if the cumulated count of children itemindices is smaller than this threshold, then don't create a full index
	int32_t maxPrefixMergeCount;
	int32_t maxSuffixMergeCount;
	bool deleteRootTrie;
	Static::TrieNode::Types nodeType;
	bool isValid() {
		return nodeType == Static::TrieNode::T_SIMPLE || nodeType == Static::TrieNode::T_COMPACT || nodeType == Static::TrieNode::T_LARGE_COMPACT;
	}
	UByteArrayAdapter trieListAdapter() {
		UByteArrayAdapter adap(trieList);
		adap.setPutPtr(trieList->size());
		return adap;
	}
};

template<typename IndexStorageContainer = std::vector<uint32_t> >
class SerializableTrie: public BaseTrie<IndexStorageContainer> {
public:
	typedef BaseTrie<IndexStorageContainer> MyBaseClass;
	typedef typename MyBaseClass::ItemIdType ItemIdType;
	typedef typename MyBaseClass::Node Node;
	typedef typename MyBaseClass::ChildNodeIterator ChildNodeIterator;
	typedef typename MyBaseClass::ConstChildNodeIterator ConstChildNodeIterator;
	typedef typename MyBaseClass::ItemSetIterator ItemSetIterator;
	typedef typename MyBaseClass::ConstItemSetIterator ConstItemSetIterator;
	typedef typename MyBaseClass::ItemSetContainer ItemSetContainer;
protected:
	bool checkTrieEqualityRecurse(Node * curNode, Static::TrieNode curStaticNode, std::string firstChar);
	bool checkIndexEqualityRecurse(Node* curNode, sserialize::Static::TrieNode curStaticNode, sserialize::Static::GeneralizedTrie& staticTrie, StringCompleter::SupportedQuerries sqtype);
public:
	SerializableTrie() {}
	SerializableTrie(bool caseSensitive) : MyBaseClass(caseSensitive) {}
	virtual ~SerializableTrie() {}
	void swap(MyBaseClass & other) { MyBaseClass::swap(other); }
	
	bool checkTrieEquality(GeneralizedTrieCreatorConfig /*config*/, sserialize::Static::GeneralizedTrie staticTrie) {
		return checkTrieEqualityRecurse(MyBaseClass::rootNode(), staticTrie.getRootNode(), "");
	}
	bool checkIndexEquality(GeneralizedTrieCreatorConfig config, Static::GeneralizedTrie staticTrie, StringCompleter::SupportedQuerries sqtype);
	
	//for debugging only
	void trieSerializationProblemFixer() { MyBaseClass::trieSerializationProblemFixer(MyBaseClass::rootNode());}
};

template<typename IndexStorageContainer>
bool
SerializableTrie<IndexStorageContainer>::
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

template<typename IndexStorageContainer>
bool
SerializableTrie<IndexStorageContainer>::
checkIndexEqualityRecurse(Node* curNode, sserialize::Static::TrieNode curStaticNode, sserialize::Static::GeneralizedTrie& staticTrie, StringCompleter::SupportedQuerries sqtype) {

	if (sqtype & sserialize::StringCompleter::SQ_EXACT) {
		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_EXACT));
		if (idx != curNode->exactValues) {
			std::cout << "FATAL: ExactIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			std::cout << "SHOULD: " << curNode->exactValues << std::endl;
			std::cout << "IS: " << idx << std::endl;
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
			std::cout << "SHOULD: " << destination << std::endl;
			std::cout << "IS: " << idx << std::endl;
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
			std::cout << "SHOULD: " << s << std::endl;
			std::cout << "IS: " << idx << std::endl;
			return false;
		}
	}
	if (sqtype & sserialize::StringCompleter::SQ_SUBSTRING) {
		std::set<ItemIdType> destination;
		curNode->insertAllValuesRecursive(destination);

		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_SUBSTRING));
		if (idx != destination) {
			std::cout << "FATAL: SuffixPrefixIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			std::cout << "SHOULD: " << destination << std::endl;
			std::cout << "IS: " << idx << std::endl;
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

template<typename IndexStorageContainer>
bool
SerializableTrie<IndexStorageContainer>::
checkIndexEquality(GeneralizedTrieCreatorConfig /*config*/, sserialize::Static::GeneralizedTrie staticTrie, sserialize::StringCompleter::SupportedQuerries sqtype) {
	Node * rootNode = MyBaseClass::m_root;
	return checkIndexEqualityRecurse(rootNode, staticTrie.getRootNode(), staticTrie, sqtype);
}

}}//end namespace


#endif