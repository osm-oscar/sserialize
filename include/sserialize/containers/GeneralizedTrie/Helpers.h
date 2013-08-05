#ifndef GENERALIZED_TRIE_HELPERS_H
#define GENERALIZED_TRIE_HELPERS_H
#include <algorithm>
#include <sserialize/vendor/utf8.h>
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
namespace GeneralizedTrie {


template<typename ItemType, typename TItemSetContainer = std::vector<ItemType> >
class MultiTrieNode {
public:
	typedef typename std::map<uint32_t, MultiTrieNode*>::iterator ChildNodeIterator;
	typedef typename std::map<uint32_t, MultiTrieNode*>::const_iterator ConstChildNodeIterator;
	typedef TItemSetContainer ItemSetContainer;
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
	
	template<class TMapper>
	void mapBreadthFirst(TMapper & mapper) {
		if (this) {
			std::deque<MultiTrieNode*> nodes;
			nodes.push_back(this);
			while(nodes.size()) {
				for(auto & x : nodes.front()->children)
					nodes.push_back( x.second );
				mapper(nodes.front());
				nodes.pop_front();
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

}}//end namespace

#endif