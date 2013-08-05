#include <iostream>
#include <algorithm>
#include <sserialize/containers/GeneralizedTrie/MultiPassTrie.h>
#include "TestItemData.h"
#include "trietestfuncs.h"
#include "printfunctions.h"

using namespace sserialize;

bool populateTrie(GeneralizedTrie::MultiPassTrie & trie, std::deque<TestItemData> & items) {
	items = createSampleData();
	StringsItemDBWrapper<TestItemData> db( new StringsItemDBWrapperPrivateSIDB<TestItemData>() );
	for(size_t i = 0; i < items.size(); i++) {
		db.insert(items[i].strs, items[i]);
	}
	trie.setDB(db);
	return true;
}


bool
checkTrie(std::deque< std::deque<std::string> > & compStrs, GeneralizedTrie::MultiPassTrie & trie, std::deque<TestItemData> & items, bool caseSensitiveTrie, bool caseSensitiveSearch, unsigned int strMatchType) {
	if (!caseSensitiveSearch || !caseSensitiveTrie) {
		strMatchType |= sserialize::StringCompleter::QT_CASE_INSENSITIVE;
	}
	for(size_t i = 0; i < compStrs.size(); i++) {
		ItemIndex trieSet = trie.complete(compStrs.at(i), (sserialize::StringCompleter::QuerryType) strMatchType);
		std::set<unsigned int> trieIdsSet = itemIdsFromTrieSet(items, trieSet);
		std::set<unsigned int> realSet = getItemIdsWithString(compStrs.at(i), (sserialize::StringCompleter::QuerryType) strMatchType, items);
		if (trieIdsSet.size() != realSet.size() || !std::equal(trieIdsSet.begin(), trieIdsSet.end(), realSet.begin())) { 
			std::cout << "CompletionSets are not equal:" << std::endl;
			printCompletionQuery(compStrs.at(i));
			std::cout << toString((sserialize::StringCompleter::QuerryType) strMatchType);
			std::cout << " =>" << std::endl << "Should be (size=" << realSet.size() << "):" << std::endl;
			printCompletionSet(realSet, items);
			std::cout << "; Is (size=" << trieSet.size() << "): " << std::endl; 
			printCompletionSet(items, trieSet);
			std::cout << std::endl;
			trie.complete(compStrs.at(i), (sserialize::StringCompleter::QuerryType) strMatchType);
			getItemIdsWithString(compStrs.at(i), (sserialize::StringCompleter::QuerryType) strMatchType, items);
			return false;
		}
	}
	return true;
}

bool testTrieOptions(bool suffixTrie, bool caseSensitiveTrie, bool caseSensitiveSearch, bool reAssignItemIds, bool reAssignStrIds) {

	std::deque<sserialize::StringCompleter::QuerryType> testRuns;
	testRuns.push_back(sserialize::StringCompleter::QT_EXACT);
	testRuns.push_back(sserialize::StringCompleter::QT_PREFIX);
	if (suffixTrie) {
		testRuns.push_back(sserialize::StringCompleter::QT_SUFFIX);
		testRuns.push_back(sserialize::StringCompleter::QT_SUFFIX_PREFIX);
	}
	
	
	GeneralizedTrie::MultiPassTrie trie;
	trie.setCaseSensitivity(caseSensitiveTrie);
	trie.setSuffixTrie(suffixTrie);
	std::deque<TestItemData> items;
	if (populateTrie(trie, items)) {
		std::cout << "Populating trie successfull" << std::endl;
	}
	else {
		std::cout << "Failed to populate trie" << std::endl;
	}
	std::deque< std::deque<std::string> > compStrs = createSampleCompletionStrings();
	
	bool allOk = true;
	for(size_t i = 0; i < testRuns.size(); i++) {
		allOk = (allOk && checkTrie(compStrs, trie, items, caseSensitiveTrie, caseSensitiveSearch, testRuns[i]));
	}
	return allOk;
}

std::string toString(bool b) {
	return (b ? "true" : "false");
}

bool testTrie() {

	bool suffixTrieOpt[2] = {false, true};
	bool caseSensitiveTrieOpt[2] = {false, true};
	bool caseSensitiveSearchOpt[2] = {false, true};
	bool reAssignItemIds[2] = {false, true};
	bool reAssignStrIds[2] = {false, true};
	bool allOk = true;
	for(size_t i = 0; i < 2; i++) {
		for(size_t j = 0; j < 2; j++) {
			for(size_t k = 0; k < 2; k++) {
				for(size_t l = 0; l < 2; l++) {
					for(size_t m = 0; m < 2; m++) {
						if (testTrieOptions(suffixTrieOpt[i], caseSensitiveTrieOpt[j], caseSensitiveSearchOpt[k], reAssignItemIds[l], reAssignStrIds[m])) {
							std::cout << "Test passed";
						}
						else {
							std::cout << "Test FAILED";
							allOk = false;
						}
						std::cout << " with suffixTrie=" << toString(suffixTrieOpt[i]) << " and caseSensitiveTrie=";
						std::cout << toString(caseSensitiveTrieOpt[j]) << " and caseSensitiveSearch=";
						std::cout << toString(caseSensitiveSearchOpt[k]) << " and reAssignItemIds=";
						std::cout << toString(reAssignItemIds[l]) << " and reAssignStrIds";
						std::cout << toString(reAssignStrIds[m]) << std::endl;
					}
				}
			}
		}
	}
	return allOk;
}


int main() {
	
	bool allOk = testTrie();
	
	if (allOk) {
		std::cout << "All tests passed" << std::endl;
	}
	else {
		std::cout << "At least one test failed" << std::endl;
	}
	
	return 0;
}