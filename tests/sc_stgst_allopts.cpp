#include <iostream>
#include <sstream>
#include <algorithm>
#include <trie/GeneralizedTrie.h>
#include <trie/privates/StaticTrie.h>
#include <trie/privates/StaticTrieNodes.h>
#include <containers/ItemSet.h>

#include "trietestfuncs.h"


using namespace sserialize;


std::string toString(bool b) {
	return (b ? "true" : "false");
}


bool populateTrie(GeneralizedTrie::MultiPassTrie & trie, std::deque<TestItemData> & items) {
	items = createSampleData();
	for(size_t i = 0; i < items.size(); i++) {
		trie.insert(items.at(i).strs, items.at(i));
	}
	return true;
}

struct TrieTestOptions {
	uint32_t curNum;
	bool disableSubTrie;
	TrieTestOptions() : curNum(0), disableSubTrie(false) {
		setOptions(curNum);
	}
	
	bool hasNextConfig() { return (curNum < 0xFFFF);}
	
	void setNextConfig() {
		do {
			curNum++;
			setOptions(curNum);
		}
		while(!validOptions() && hasNextConfig());
	}

	bool validOptions() {
		if (suffixTrie && withSubTries) return false;
		if (indirectIndex && withSubTries) return false;
		if (withSubTries && disableSubTrie) return false;
		return true;
	}

	void setOptions(uint32_t cn) {
		suffixTrie = cn & 0x1;
		caseSensitiveTrie = cn & 0x2;
		caseSensitiveSearch = cn & 0x4;
		reAssignItemIds = cn & 0x8;
		reAssignStrIds = cn & 0x10;
		withSubTries = cn & 0x20;
		normalizedSubTrieSearch = cn & 0x40;
		indirectIndex = cn & 0x80;
		deleteRootTrie = cn & 0x100;
		regressionLine = cn & 0x200;
		fixedBitWidth = cn & 0x400;
		levelsWithoutFullIndex = cn >> 11;
	}


	std::string toString(bool b) const {
		return (b ? "true" : "false");
	}


	std::string toString() const {
		std::stringstream ss;
		ss << std::endl << "curNum=" << curNum << std::endl;
		ss << "0x001 suffixTrie=" << toString(suffixTrie) << std::endl;
		ss << "0x002 caseSensitiveTrie=" << toString(caseSensitiveTrie) << std::endl;
		ss << "0x004 caseSensitiveSearch=" << toString(caseSensitiveSearch) << std::endl;
		ss << "0x008 reAssignItemIds=" << toString(reAssignItemIds) << std::endl;
		ss << "0x010 reAssignStrIds=" << toString(reAssignStrIds) << std::endl;
		ss << "0x020 withSubTries=" << toString(withSubTries) << std::endl;
		ss << "0x040 normalizedSubTrieSearch=" << toString(normalizedSubTrieSearch) << std::endl;
		ss << "0x080 indirectIndex=" << toString(indirectIndex) << std::endl;
		ss << "0x100 deleteRootTrie=" << toString(deleteRootTrie) << std::endl;
		ss << "0x200 regressionLine=" << toString(regressionLine) << std::endl;
		ss << "0xn00 levelsWithoutFullIndex=" << static_cast<uint32_t>(levelsWithoutFullIndex);
		return ss.str();
	}

	void dump() const {
		std::cout << toString() << std::endl;
	}

	GeneralizedTrieCreatorConfig getSerializerConfig() const {
		GeneralizedTrieCreatorConfig config;
		config.indirectIndex = indirectIndex;
		config.deleteRootTrie = deleteRootTrie;
		config.regressionLine = regressionLine;
		config.fixedBitWidth = (fixedBitWidth ? 32 : -1);
		for(int i = 0; i < levelsWithoutFullIndex; i++) {
			config.levelsWithoutFullIndex.insert(i);
		}
		return config;
	}

	bool suffixTrie; //[2];
	bool caseSensitiveTrie;//[2];
	bool caseSensitiveSearch;//[2];
	bool reAssignItemIds;//[2];
	bool reAssignStrIds;//[2];

	//Serializer tests
	bool withSubTries;//[2];
	bool normalizedSubTrieSearch;//[2];
	bool indirectIndex;//[2];
	bool deleteRootTrie;//[2];
	bool regressionLine;//[2];
	bool fixedBitWidth;//only test auto or 32 bit
	uint8_t levelsWithoutFullIndex;//[8]; //no need to test deeper, shouldn't make any difference
};

bool
checkTrie(std::deque< std::deque<std::string> > & compStrs, std::deque<TestItemData> & items, const TrieTestOptions & opts, Static::StringCompleter & trie, TestItemDataDB & db) {
	unsigned int strMatchType = 0;
	if (opts.suffixTrie) {
		strMatchType |= StringCompleter::;
	}
	else {
		strMatchType |= TestItemData::SMT_PREFIX;
	}
	
	if (!opts.caseSensitiveTrie || !opts.caseSensitiveSearch)
		strMatchType |= TestItemData::SMT_CASEINSENSITIVE;

	for(size_t i = 0; i < compStrs.size(); i++) {

		TrieDataItemSet itemSet(compStrs[i], trie, db);
		itemSet.execute();

		std::set<unsigned int> realSet = getItemIdsWithString(compStrs.at(i), (sserialize::StringCompleter::QuerryType)strMatchType, items);
		if (itemSet.size() != realSet.size())
			return false;

		//Check all items
		for(size_t i = 0; i < itemSet.size(); i++) {
			if ( realSet.count(itemSet.at(i).data().id) == 0) {
				return false;
			}
		}

// 			std::cout << "CompletionSets are not equal:" << std::endl;
// 			printCompletionQuery(compStrs.at(i));
// 			std::cout << " =>" << std::endl << "Should be (size=" << realSet.size() << "):" << std::endl;
// 			printCompletionSet(realSet, items);
// 			std::cout << "; Is: " << std::endl; 
// 			printCompletionSet(trieSet);

	}
	return true;
}

bool testTrieOptions(const TrieTestOptions & opts) {
	
	GeneralizedTrie::MultiPassTrie trie;
	trie.setCaseSensitivity(opts.caseSensitiveTrie);
	trie.setSuffixTrie(opts.suffixTrie);
	std::deque<TestItemData> items;
	if (populateTrie(trie, items)) {
		std::cout << "Populating trie successfull" << std::endl;
	}
	else {
		std::cout << "Failed to populate trie" << std::endl;
	}

	if (opts.reAssignItemIds)
		trie.reAssignItemIds();

	if (opts.reAssignStrIds)
		trie.db()->reAssignStringIds();

	GeneralizedTrieCreatorConfig stCreateConfig = opts.getSerializerConfig();
	std::deque<uint8_t> staticTrieList;
	std::deque<uint8_t> staticIndexList;
	std::deque<uint8_t> staticStringList;
	std::deque<uint8_t> staticDataBaseList;
	stCreateConfig.trieList = &staticTrieList;
	UByteArrayAdapter stringAdapter(&staticStringList);


	trie.createStaticTrie(stCreateConfig);
	trie.db()->createStaticStringTable(stringAdapter);
	UByteArrayAdapter staticDataBaseListAdapter(&staticDataBaseList);
	staticDataBaseListAdapter << *(trie.db());

	UByteArrayAdapter trieAdapter(&staticTrieList);
	UByteArrayAdapter indexAdapter(& (stCreateConfig.indexFactory.getIndexStore()) );
	UByteArrayAdapter databaseAdapter(&staticDataBaseList);

	Static::StringCompleter staticStringCompleter(trieAdapter, indexAdapter);
	Static::StringTable staticStringTable(stringAdapter);
	sserialize::Static::StringsItemDB<TestItemData> staticItemDatabase(databaseAdapter, staticStringTable);
	

	std::deque< std::deque<std::string> > compStrs = createSampleCompletionStrings();


	return checkTrie(compStrs, items, opts, staticStringCompleter, staticItemDatabase);

}


bool testTrie(bool disableSubTrie) {
	TrieTestOptions testOpts;
	testOpts.disableSubTrie = disableSubTrie;
	bool allOk = true;
	while(testOpts.hasNextConfig()) {
		if (testTrieOptions(testOpts)) {
			std::cout << "Test passed with" << testOpts.toString() << std::endl; 
		}
		else {
			std::cout << "Test FAILED with" << testOpts.toString() << std::endl;
			allOk = false;
		}
		testOpts.setNextConfig();
	}
	return allOk;
}


int main(int argc, char** argv) {
	bool disableSubTrie = true;
	if (argc > 0) {
		std::string str(argv[0]);
		if (str == "-s") {
			disableSubTrie = false;
		}
	}

	bool allOk = testTrie(disableSubTrie);
	
	if (allOk) {
		std::cout << "All tests passed" << std::endl;
	}
	else {
		std::cout << "At least one test failed" << std::endl;
	}
	
	return 0;
}