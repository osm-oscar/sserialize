#ifndef SSERIALIZE_GENERALIZED_TRIE_SINGLE_PASS_TRIE_H
#define SSERIALIZE_GENERALIZED_TRIE_SINGLE_PASS_TRIE_H
#include "BaseTrie.h"

namespace sserialize {
namespace GeneralizedTrie {

class SinglePassTrie: protected BaseTrie< WindowedArray<uint32_t> > {
public:
	typedef BaseTrie< WindowedArray<uint32_t> > MyBaseClass;
private:
	SinglePassTrie(const SinglePassTrie & other);
	SinglePassTrie & operator=(const SinglePassTrie & other);
public:
	SinglePassTrie();
	virtual ~SinglePassTrie();
	void swap(MyBaseClass & other);
	
	template<typename T_ITEM_FACTORY, typename T_ITEM>
	bool staticFromStringsFactory(const T_ITEM_FACTORY & stringsFactory, sserialize::UByteArrayAdapter & dest);
};

template<typename T_ITEM_FACTORY, typename T_ITEM>
bool SinglePassTrie::staticFromStringsFactory(const T_ITEM_FACTORY & stringsFactory, UByteArrayAdapter & dest) {
	std::unordered_map<std::string, std::unordered_set<Node*> > strIdToSubStrNodes;
	std::unordered_map<std::string, std::unordered_set<Node*> > strIdToExactNodes;
	uint64_t exactStorageNeed, suffixStorageNeed;
	
	if (trieFromStringsFactory(stringsFactory, strIdToSubStrNodes, strIdToExactNodes)) {
		return false;
	}
	
	//Now add the items
	ProgressInfo progressInfo;
	progressInfo.begin(stringsFactory.end()-stringsFactory.begin(), "BaseTrie::fromStringsFactory::calculating storage need");
	uint32_t count = 0;
	for(typename T_ITEM_FACTORY::const_iterator itemIt(stringsFactory.begin()), itemEnd(stringsFactory.end()); itemIt != itemEnd; ++itemIt) {
		std::unordered_set<Node*> exactNodes;
		std::unordered_set<Node*> suffixNodes;
		T_ITEM item = *itemIt;
		for(typename T_ITEM::const_iterator itemStrsIt(item.begin()), itemStrsEnd(item.end()); itemStrsIt != itemStrsEnd; ++itemStrsIt) {
			if (strIdToExactNodes.count(*itemStrsIt)) {
				exactNodes.insert(strIdToExactNodes[*itemStrsIt].begin(), strIdToExactNodes[*itemStrsIt].end());
			}
			else {
				std::cout << "ERROR: No exact node for item string" << std::endl;
			}
			if (strIdToSubStrNodes.count(*itemStrsIt)) {
				suffixNodes.insert(strIdToSubStrNodes[*itemStrsIt].begin(), strIdToSubStrNodes[*itemStrsIt].end());
			}
		}
		
		exactStorageNeed += exactNodes.size();
		suffixStorageNeed += suffixNodes.size();

		for(std::unordered_set<Node*>::iterator esit = exactNodes.begin(); esit != exactNodes.end(); ++esit) {
			(*esit)->exactValues.reserve((*esit)->exactValues.capacity()+1);
		}
		for(std::unordered_set<Node*>::iterator it = suffixNodes.begin(); it != suffixNodes.end(); ++it) {
			(*it)->subStrValues.reserve((*it)->subStrValues.capacity()+1);
		}


		progressInfo(++count);
	}
	progressInfo.end();
	std::cout << std::endl;
	
	progressInfo.begin(stringsFactory.end()-stringsFactory.begin(), "BaseTrie::fromStringsFactory::popluating exact/suffix index");
	count = 0;
	for(typename T_ITEM_FACTORY::const_iterator itemIt(stringsFactory.begin()), itemEnd(stringsFactory.end()); itemIt != itemEnd; ++itemIt) {
		std::unordered_set<Node*> exactNodes;
		std::unordered_set<Node*> suffixNodes;
		T_ITEM item = *itemIt;
		for(typename T_ITEM::const_iterator itemStrsIt(item.begin()), itemStrsEnd(item.end()); itemStrsIt != itemStrsEnd; ++itemStrsIt) {
			if (strIdToExactNodes.count(*itemStrsIt)) {
				exactNodes.insert(strIdToExactNodes[*itemStrsIt].begin(), strIdToExactNodes[*itemStrsIt].end());
			}
			else {
				std::cout << "ERROR: No exact node for item string" << std::endl;
			}
			if (strIdToSubStrNodes.count(*itemStrsIt)) {
				suffixNodes.insert(strIdToSubStrNodes[*itemStrsIt].begin(), strIdToSubStrNodes[*itemStrsIt].end());
			}
		}
		
		for(std::unordered_set<Node*>::iterator esit = exactNodes.begin(); esit != exactNodes.end(); ++esit) {
			(*esit)->exactValues.insert((*esit)->exactValues.end(), count);
		}
		for(std::unordered_set<Node*>::iterator it = suffixNodes.begin(); it != suffixNodes.end(); ++it) {
			(*it)->subStrValues.insert((*it)->subStrValues.end(), count);
		}

		progressInfo(++count);
	}
	progressInfo.end();
	
	if (!consistencyCheck()) {
		std::cout << "Trie is broken after BaseTrie::fromStringsFactory::insertItems" << std::endl;
		return false;
	}

	return true;
}


}}//end namespace

#endif