#ifndef GENERAALIZED_TRIE_HELPERS_H
#define GENERAALIZED_TRIE_HELPERS_H
#include <algorithm>
#include <sserialize/vendor/libs/utf8/source/utf8.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/ProgressInfo.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/utility/stringfunctions.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/Static/StringCompleter.h>
#include <sserialize/Static/TrieNodePrivates/TrieNodePrivates.h>
#include <sserialize/Static/GeneralizedTrie.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/CompactUintArray.h>


namespace sserialize {


template<class ItemType>
class MultiTrieNode {
public:
	typedef typename std::map<uint32_t, MultiTrieNode*>::iterator ChildNodeIterator;
	typedef typename std::map<uint32_t, MultiTrieNode*>::const_iterator ConstChildNodeIterator;
	typedef typename std::vector<ItemType> ItemSetContainer;
	typedef typename ItemSetContainer::iterator ItemSetIterator;
	typedef typename ItemSetContainer::const_iterator ConstItemSetIterator;
	class TemporalPrivateStorage {
	public:
		TemporalPrivateStorage() {};
		virtual ~TemporalPrivateStorage() {};
	};
public:
	//One unicode point, only "real" characters are allowed
	std::string c;
	ItemSetContainer exactValues;
	ItemSetContainer subStrValues;
	//Map from uinicode code point to Node
	//no surogates
	MultiTrieNode * parent;
	std::map<uint32_t, MultiTrieNode*> children;
	TemporalPrivateStorage * temporalPrivateStorage;
public:
	MultiTrieNode() : parent(0), temporalPrivateStorage(0) {};
	virtual ~MultiTrieNode() {
		if (temporalPrivateStorage)
			delete temporalPrivateStorage;
		for(ChildNodeIterator i = this->children.begin(); i != this->children.end(); ++i) {
			delete i->second;
		}
	};
	void dump() {
		std::cout << "NODE-DEBUG-START" << std::endl;
		std::cout << "this=" << (uintptr_t)( static_cast<const void*>(this) ) << std::endl;
		std::cout << "c.size=" << c.size() << std::endl;
		std::cout << "c.data=" << c << std::endl;
		std::cout << "children: " << children.size();
		if (children.size()) {
			std::cout << "=>";
			std::deque<uint16_t> sortList = getSortedChildChars();
			for(std::deque<uint16_t>::iterator i = sortList.begin(); i != sortList.end(); ++i) {
				std::cout << static_cast<unsigned int>(*i) << ", ";
			}
		}
		std::cout << std::endl;
		std::cout << "NODE-DEBUG-END" << std::endl;
		std::cout << std::flush;
	}

	template<typename T_PUSH_BACK_CONTAINER>
	void insertSortedChildChars(T_PUSH_BACK_CONTAINER & destination) {
		for(ChildNodeIterator i = children.begin(); i != children.end(); ++i) {
			destination.push_back(i->first);
		}
	}
	
	std::deque<uint16_t> getSortedChildChars() {
		std::deque<uint16_t> childrenList;
		for(ChildNodeIterator i = children.begin(); i != children.end(); ++i) {
			if (i->first <= 0xFFFF)
				childrenList.push_back(i->first);
		}
		return childrenList;
	}

	std::deque< MultiTrieNode<unsigned int>* > getSortedChildPtrs() {
		std::deque< MultiTrieNode<unsigned int>* > tmp;
		for(ChildNodeIterator i = children.begin(); i != children.end(); ++i) {
				tmp.push_back(i->second);
		}
		return tmp;
	}
	
	std::vector< MultiTrieNode* > getSortedChildPtrsVec() {
		std::vector< MultiTrieNode* > ret;
		ret.reserve(children.size());
		for(ChildNodeIterator i = children.begin(); i != children.end(); ++i) {
				ret.push_back(i->second);
		}
		return ret;
	}

	void deleteTemporalPrivateStorage() {
		if (temporalPrivateStorage) {
			delete temporalPrivateStorage;
			temporalPrivateStorage = 0;
		}
	}

	void deleteChildren() {
		for(ChildNodeIterator i = children.begin(); i != children.end(); ++i) {
			delete i->second;
		}
		children.clear();
	}

	uint32_t depth() {
		if (this) {
			uint32_t maxDepth = 0;
			for(ChildNodeIterator it = children.begin(); it != children.end(); ++it) {
				uint32_t d = it->second->depth();
				if (d > maxDepth)
					maxDepth = d;
			}
			return maxDepth+1;
		}
		return 0;
	}

	void addNodesSortedInLevel(const uint16_t targetLevel, uint16_t curLevel, std::deque< MultiTrieNode* > & destination) {
		if (this) {
			if (curLevel == targetLevel) {
				destination.push_back(this);
			}
			else {
				for(ChildNodeIterator it = children.begin();  it != children.end(); ++it) {
					it->second->addNodesSortedInLevel(targetLevel, curLevel+1, destination);
				}
			}
		}
	}

	void addNodesSorted(uint16_t curLevel, std::deque< std::deque< MultiTrieNode* > > & destination) {
		if (this) {
			if (destination.size() <= curLevel)
				destination.push_back(std::deque< MultiTrieNode* >());
			destination[curLevel].push_back(this);
			for(ChildNodeIterator it = children.begin();  it != children.end(); ++it) {
				it->second->addNodesSorted(curLevel+1, destination);
			}
		}
	}

	void addNodesSorted(std::deque< MultiTrieNode* > & destination) {
		if (this) {
			size_t i = destination.size();
			destination.push_back(this);
			while (i < destination.size()) {
				MultiTrieNode * curNode = destination[i];
				for(ConstChildNodeIterator it = curNode->children.begin(); it != curNode->children.end(); ++it) {
					destination.push_back(it->second);
				}
				i++;
			}
		}
	}
	
	void addNodesSortedDepthFirst(std::deque< MultiTrieNode* > & destination) {
		if (this) {
			destination.push_back(this);
			for(ChildNodeIterator it = children.begin();  it != children.end(); ++it) {
				it->second->addNodesSortedDepthFirst(destination);
			}
		}
	}

	void addNodesSortedDepthFirst(std::vector< MultiTrieNode* > & destination) {
		if (this) {
			destination.push_back(this);
			for(ChildNodeIterator it = children.begin();  it != children.end(); ++it) {
				it->second->addNodesSortedDepthFirst(destination);
			}
		}
	}


	bool checkParentChildRelations() {
		if (!this) {
			return true;
		}
		for(ChildNodeIterator i = children.begin(); i != children.end(); ++i) {
			if (i->second->parent != this) {
				return false;
			}
		}
		for(ChildNodeIterator i = children.begin(); i != children.end(); ++i) {
			if (! i->second->checkParentChildRelations()) {
				return false;
			}
		}
		return true;
	}

	void fixChildParentPtrRelation() {
		if (this) {
			for(ChildNodeIterator i = children.begin(); i != children.end(); ++i) {
				i->second->parent = this;
			}
		}
	}

	void insertExactValuesRecursive(std::set<ItemType> & destination) {
		if (this) {
			destination.insert(exactValues.begin(), exactValues.end());
			for(ChildNodeIterator it = children.begin(); it != children.end(); ++it) {
				it->second->insertExactValuesRecursive(destination);
			}
		}
	}

	void insertSubStrValuesRecursive(std::set<ItemType> & destination) {
		if (this) {
			destination.insert(subStrValues.begin(), subStrValues.end());
			for(ChildNodeIterator it = children.begin(); it != children.end(); ++it) {
				it->second->insertSubStrValuesRecursive(destination);
			}
		}
	}

	void insertAllValuesRecursive(std::set<ItemType> & destination) {
		if (this) {
			destination.insert(exactValues.begin(), exactValues.end());
			destination.insert(subStrValues.begin(), subStrValues.end());
			for(ChildNodeIterator it = children.begin(); it != children.end(); ++it) {
				it->second->insertAllValuesRecursive(destination);
			}
		}
	}
	
	void getAllValuesRecursiveSetSize(uint32_t & size) {
		if (this) {
			size += exactValues.size() + subStrValues.size();
			for(ChildNodeIterator it = children.begin(); it != children.end(); ++it) {
				it->second->getAllValuesRecursiveSetSize(size);
			}
		}
	}

	void remapValues(const std::map<ItemType, ItemType> & oldToNew) {
		remapSorted(exactValues, exactValues, oldToNew);
		remapSorted(subStrValues, subStrValues, oldToNew);
	}
	
	void getSmallestValueRecursive(ItemType & value) {
		if (this) {
			if (exactValues.size())
				value = std::min<ItemType>(*(exactValues.begin()), value);
			if (subStrValues.size())
				value = std::min<ItemType>(*(subStrValues.begin()), value);
			for(ChildNodeIterator it = children.begin(); it != children.end(); ++it) {
				it->second->getSmallestValueRecursive(value);
			}
		}
	}

	void getLargestValueRecursive(ItemType & value) {
		if (this) {
			if (exactValues.size())
				value = std::max<ItemType>(*(exactValues.begin()), value);
			if (subStrValues.size())
				value = std::max<ItemType>(*(subStrValues.begin()), value);
			for(ChildNodeIterator it = children.begin(); it != children.end(); ++it) {
				it->second->getLargestValueRecursive(value);
			}
		}
	}

	template<class TMapper>
	void mapDepthFirst(TMapper & mapper) {
		if (this) {
			mapper(this);
			for(ChildNodeIterator it = children.begin(); it != children.end(); ++it) {
				it->second->mapDepthFirst(mapper);
			}
		}
	}
	
	static ItemSetContainer mergeExactIndices(const std::vector<MultiTrieNode*> & nodes, uint32_t begin, uint32_t end) {
		if (begin ==  end)
			return nodes[begin]->exactValues;
		else if (end-begin == 1) {
			ItemSetContainer ret;
			return mergeSortedContainer(ret, nodes[begin]->exactValues, nodes[end]->exactValues);
		}
		else {
			ItemSetContainer ret;
			mergeSortedContainer(ret, mergeExactIndices(nodes, begin, begin+(end-begin)/2), mergeExactIndices(nodes, begin+(end-begin)/2+1, end));
			return ret;
		}
	}

	static ItemSetContainer mergeSuffixIndices(const std::vector<MultiTrieNode*> & nodes, uint32_t begin, uint32_t end) {
		if (begin ==  end)
			return nodes[begin]->subStrValues;
		else if (end-begin == 1) {
			ItemSetContainer ret;
			return mergeSortedContainer(ret, nodes[begin]->subStrValues, nodes[end]->subStrValues);
		}
		else {
			ItemSetContainer ret;
			mergeSortedContainer(ret, mergeExactIndices(nodes, begin, begin+(end-begin)/2), mergeExactIndices(nodes, begin+(end-begin)/2+1, end));
			return ret;
		}
	}
};

	
	struct StringEntry {
		uint32_t stringId;
		uint16_t strBegin;
		uint16_t strLen;
	};
	struct IndexEntry {
		IndexEntry() : mergeIndex(true), itemIdIndex(true), exactValues(0), prefixValues(0), suffixValues(0), suffixPrefixValues(0)  {}
		bool mergeIndex;
		bool itemIdIndex;
		uint32_t exactValues;
		uint32_t prefixValues;
		uint32_t suffixValues;
		uint32_t suffixPrefixValues;
		
		uint32_t minId;
		uint32_t maxId;
	};
	
	struct FlatGSTConfig {
		FlatGSTConfig(UByteArrayAdapter & destination, sserialize::ItemIndexFactory & indexFactory) :
			destination(destination), indexFactory(indexFactory),
			withStringIds(false), maxSizeForItemIdIndex(0), minStrLenForItemIdIndex(0),
			deleteTrie(false), mergeIndex(true)  {}
		FlatGSTConfig(UByteArrayAdapter & destination, sserialize::ItemIndexFactory & indexFactory, bool withStringIds, uint32_t maxSizeForItemIdIndex, uint32_t minStrLenForItemIdIndex, bool deleteTrie, bool mergeIndex) : 
			destination(destination), indexFactory(indexFactory), withStringIds(withStringIds),
			maxSizeForItemIdIndex(maxSizeForItemIdIndex), minStrLenForItemIdIndex(minStrLenForItemIdIndex), deleteTrie(deleteTrie), mergeIndex(mergeIndex)  {}
		UByteArrayAdapter & destination;
		sserialize::ItemIndexFactory & indexFactory;
		bool withStringIds;
		uint32_t maxSizeForItemIdIndex;
		uint32_t minStrLenForItemIdIndex;
		bool deleteTrie;
		bool mergeIndex;
	};
	
	template<typename ItemIdType>
	class FlatTrieEntryConfig {
	public:
	private:
// 		FlatTrieEntryConfig & operator=(const FlatTrieEntry & f);
	public:
		FlatTrieEntryConfig(ItemIndexFactory & indexFactory) : indexFactory(indexFactory) {}
		 ~FlatTrieEntryConfig() {}
		std::deque<std::string> flatTrieStrings;
		std::unordered_map<MultiTrieNode<ItemIdType>*, StringEntry> stringEntries;
		ItemIndexFactory & indexFactory;
		uint32_t curStrId;
		std::string::const_iterator strIt;
		std::string::const_iterator strBegin;
		std::string::const_iterator strEnd;
	};

	inline bool isValidString(const std::string & str) {
		if (! utf8::is_valid(str.begin(), str.end())) {
			std::cout << "Invalid unicode string detected!" << std::endl;
			return false;
		}
		
		std::string::const_iterator strIt = str.begin();
		uint32_t strItUCode;
		//Check for unsupported characters (surrogates and everything not encodable in 16 bit, the last constraint is du to the static trie implementation
		while (strIt != str.end()) {
			strItUCode = utf8::next(strIt, str.end());
			if (strItUCode > 0xFFFF || (strItUCode >= 0xD800 && strItUCode && strItUCode <= 0xDFFF) ) {
				std::cout << "Unsupported unicode points detected!" << std::endl;
				return false;
			}
		}
		return true;
	}

	//stupid set intersection function
	template<typename ItemType>
	std::set<ItemType> intersectSets(std::deque< std::set<ItemType> > sets) {
		if (sets.size() == 0)
			return std::set<ItemType>();
		if (sets.size() == 1)
			return sets[0];
		std::set<ItemType> res;
		for(typename std::set<ItemType>::const_iterator it = sets[0].begin(); it != sets[0].end(); ++it) {
			bool insert = true;
			for(size_t j = 1; j < sets.size(); j++) {
				if (sets[j].count(*it) == 0) {
					insert = false;
					break;
				}
			}
			if (insert)
				res.insert(*it);
		}
		return res;
	}


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

	
}//end namespace

inline sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::StringEntry & source) {
	destination.putVlPackedUint32(source.stringId);
	destination.putVlPackedUint32(source.strBegin);
	destination.putVlPackedUint32(source.strLen);
	return destination;
}

inline sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::IndexEntry & source) {
	uint8_t header = 0;
	std::deque<uint32_t> indexPtrs;
	if (source.exactValues) {
		header |= 0x1;
		indexPtrs.push_back(source.exactValues);
	}
	if (source.prefixValues) {
		header |= 0x2;
		indexPtrs.push_back(source.prefixValues);
	}
	if (source.suffixValues) {
		header |= 0x4;
		indexPtrs.push_back(source.suffixValues);
	}
	if (source.suffixPrefixValues) {
		header |= 0x8;
		indexPtrs.push_back(source.suffixPrefixValues);
	}
	
	if (source.itemIdIndex)
		header |= 0x10;
	else {
		indexPtrs.push_back(source.minId);
		indexPtrs.push_back(source.maxId);
	}

	if (source.mergeIndex)
		header |= 0x80;
		
	uint32_t largestPtr = std::max(std::max(source.exactValues, source.prefixValues), std::max(source.suffixValues, source.suffixPrefixValues) );
	if (!source.itemIdIndex)
		largestPtr = std::max(largestPtr, source.maxId);
	uint8_t bpn = sserialize::CompactUintArray::minStorageBitsFullBytes(largestPtr);
	header |= ((bpn/8-1) & 0x3) << 5;
	

	std::deque<uint8_t> sidxPtrs; 
	sserialize::CompactUintArray::createFromSet(indexPtrs, sidxPtrs, bpn);
	
	destination << header;
	destination.put(sidxPtrs);
	return destination;
}


#endif