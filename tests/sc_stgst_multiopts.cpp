#include <sserialize/Static/GeneralizedTrie.h>
#include <sserialize/containers/GeneralizedTrie.h>
#include <sserialize/containers/ItemSet.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/filewriter.h>
#include "trietestfuncs.h"
#include "TestItemData.h"
#include "printfunctions.h"

using namespace sserialize;

// #define TRIE_INSERT

bool createAndCheckTries(std::set<uint8_t> levelsWithoutFullIndex) {
	std::deque<TestItemData> data = createSampleData();
	
	StringsItemDB<TestItemData> db;
	StringsItemDBWrapper<TestItemData> dbWrapper(new StringsItemDBWrapperPrivateSIDB<TestItemData>(db));
	
#ifdef TRIE_INSERT
	GeneralizedTrie::MultiPassTrie mtrie;
	mtrie.setCaseSensitivity(false);
	mtrie.setSuffixTrie(true);
	mtrie.setDB( dbWrapper );
	for(std::deque<TestItemData>::iterator it = data.begin(); it != data.end(); it++) {
		mtrie.insert(it->strs, *it);
	}
#else
	for(std::deque<TestItemData>::iterator it = data.begin(); it != data.end(); it++) {
		dbWrapper.insert(it->strs, *it);
	}
	GeneralizedTrie::MultiPassTrie mtrie(false, true);
	mtrie.setDB(dbWrapper);
#endif
	if (!mtrie.consistencyCheck()) {
		std::cout << "Trie consistencyCheck failed!" << std::endl;
		return false;
	}

	std::deque<uint8_t> staticTrieList;
	std::deque<uint8_t> staticStringList;
	std::deque<uint8_t> staticDataBaseList;

	UByteArrayAdapter stableAdap(&staticStringList);

	sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig config;
	config.trieList = &staticTrieList;
	config.levelsWithoutFullIndex = levelsWithoutFullIndex;
	config.nodeType = Static::TrieNode::T_SIMPLE;
	mtrie.createStaticTrie(config);
	db.createStaticStringTable(stableAdap);
	UByteArrayAdapter staticDataBaseListAdapter(&staticDataBaseList);
	staticDataBaseListAdapter << db;


	std::cout << "Serializing ItemFactory...";
	if (config.indexFactory.flush()) {
		std::cout << "OK";
	}
	else {
		std::cout << "FAILED";
	}
	std::cout  << std::endl;

	UByteArrayAdapter indexAdap(config.indexFactory.getFlushedData());

	UByteArrayAdapter trieAdap(&staticTrieList);
	UByteArrayAdapter dbAdap( (&staticDataBaseList) );
	
	Static::ItemIndexStore idxStore(indexAdap);

	StringCompleter scompleter( new Static::StringCompleter(trieAdap, idxStore) );
	Static::StringTable stable(stableAdap);
	TestItemDataDB itemDataBase(dbAdap, stable);

	std::cout << "Checking Trie equality...";
	{
		Static::GeneralizedTrie staticTrie(*(dynamic_cast<Static::GeneralizedTrie*>(scompleter.getPrivate())));
		if (mtrie.checkTrieEquality(config, staticTrie)) {
			std::cout << "Passed.";
		}
		else {
			std::cout << "Failed.";
		}
		std::cout << std::endl;
		std::cout << "Checking Index equality...";
		if (mtrie.checkIndexEquality(config, staticTrie, sserialize::StringCompleter::SQ_EXACT)) {
			std::cout << "EXACT=OK. ";
		}
		else {
			std::cout << "EXACT=Failed. ";
		}
		if (mtrie.checkIndexEquality(config, staticTrie, sserialize::StringCompleter::SQ_PREFIX)) {
			std::cout << "PREFIX=OK. ";
		}
		else {
			std::cout << "PREFIX=Failed. ";
		}
		if (mtrie.checkIndexEquality(config, staticTrie, sserialize::StringCompleter::SQ_SUFFIX)) {
			std::cout << "SUFFIX=OK. ";
		}
		else {
			std::cout << "SUFFIX=Failed. ";
		}
		if (mtrie.checkIndexEquality(config, staticTrie, sserialize::StringCompleter::SQ_SUFFIX_PREFIX)) {
			std::cout << "SUFFIX_PREFIX=OK. ";
		}
		else {
			std::cout << "SUFFIX_PREFIX=Failed. ";
		}

		std::cout << std::endl;
	}

	std::deque< std::deque<std::string> > compStrs = createSampleCompletionStrings();

	sserialize::StringCompleter::QuerryType strMatchType = (sserialize::StringCompleter::QuerryType) (sserialize::StringCompleter::QT_SUFFIX_PREFIX | sserialize::StringCompleter::QT_CASE_INSENSITIVE);

	//Check completionsets for equality
	bool allOk = true;
	for(std::deque< std::deque<std::string> >::iterator it = compStrs.begin(); it != compStrs.end(); it++) {
		TestItemDataItemSet itemSet(*it, scompleter, itemDataBase, SetOpTree::SOT_COMPLEX); itemSet.execute();
		ItemIndex cset = mtrie.complete(*it, strMatchType);
		std::set<unsigned int> trieIdsSet = itemIdsFromTrieSet(dbWrapper, cset);
		std::set<unsigned int> realSet = getItemIdsWithString(*it, strMatchType, data);
		if (trieIdsSet.size() != realSet.size() || !std::equal(trieIdsSet.begin(), trieIdsSet.end(), realSet.begin())) { 
			std::cout << "MultiRadixTrie completion-set is broken!" << std::endl;
			allOk = false;
			printCompletionQuery(*it); std::cout << " -> ";
			std::cout << "MultiRadixTrie:  " << std::endl;
			printCompletionSet(dbWrapper, cset); std::cout << std::endl << std::endl;
			std::cout << "StaticRecTrie: " << std::endl;
			printCompletionSet(itemSet); std::cout << std::endl << std::endl;
		}

		if (!checkCompletionSetsEquality(dbWrapper,cset, itemSet)) {
			allOk = false;
			std::cout << "Static trie is broken" << std::endl;
			printCompletionQuery(*it); std::cout << " -> ";
			std::cout << "MultiRadixTrie:  " << std::endl;
			printCompletionSet(dbWrapper, cset); std::cout << std::endl << std::endl;
			std::cout << "StaticRecTrie: " << std::endl;
			printCompletionSet(itemSet); std::cout << std::endl << std::endl;
		}

		if (!allOk) {
			ItemIndex cset = mtrie.complete(*it, strMatchType);
			TestItemDataItemSet itemSet2(*it, scompleter, itemDataBase, SetOpTree::SOT_COMPLEX);
			itemSet2.execute();
			itemSet2.at(0);
			itemSet2.at(1);
			itemSet2.at(2);
			itemSet2.at(3);
			break;
		}
	}

// 	if (allOk) {
// 		scompleter.printStats(std::cout);
// 	}

	return allOk;
};

void testTrieSerializer() {

	std::set<uint8_t> levelsWithoutFullIndex;

	std::cout << "____Testing without subtries and direct Index______" << std::endl;
	if (createAndCheckTries(levelsWithoutFullIndex)) {
		std::cout << "OK!" << std::endl;
	}

	levelsWithoutFullIndex.insert(0);
	std::cout << "____Testing without subtries and direct Index with no fullindex in level=(0)______" << std::endl;
	if (createAndCheckTries(levelsWithoutFullIndex)) {
		std::cout << "OK!" << std::endl;
	}

	levelsWithoutFullIndex.clear();
	levelsWithoutFullIndex.insert(1);

	std::cout << "____Testing without subtries and direct Index with no fullindex in level=(1)______" << std::endl;
	if (createAndCheckTries(levelsWithoutFullIndex)) {
		std::cout << "OK!" << std::endl;
	}

	levelsWithoutFullIndex.insert(0);
	std::cout << "____Testing without subtries and direct Index with no fullindex in level=(0,1)______" << std::endl;
	if (createAndCheckTries(levelsWithoutFullIndex)) {
		std::cout << "OK!" << std::endl;
	}

	for(size_t i = 0; i < 20; i++) {
		levelsWithoutFullIndex.insert(i);
	}

	std::cout << "____Testing without subtries and direct Index with no fullindex at all______" << std::endl;
	if (createAndCheckTries(levelsWithoutFullIndex)) {
		std::cout << "OK!" << std::endl;
	}


}

int main() {
	std::cout << std::endl << "Test bottom-up serialization" << std::endl;
	testTrieSerializer();
	return 0;
}

