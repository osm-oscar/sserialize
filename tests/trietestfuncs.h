#ifndef SSERIALIZE_TESTS_TRIE_TEST_FUNCS_H
#define SSERIALIZE_TESTS_TRIE_TEST_FUNCS_H
#include <deque>
#include <sserialize/Static/GeneralizedTrie.h>
#include "containers/StringsItemDBWrapper.h"
#include "TestItemData.h"

namespace sserialize {

bool checkCompletionSetsEquality(const StringsItemDBWrapper<TestItemData> & db, ItemIndex trieSet, TestItemDataItemSet & sset);

std::set<unsigned int> itemIdsFromTrieSet(const StringsItemDBWrapper<TestItemData> & db, ItemIndex trieSet);
std::set<unsigned int> itemIdsFromTrieSet(std::deque<TestItemData> & db, ItemIndex trieSet);
std::set<unsigned int> itemIdsFromTrieSet(TestItemDataItemSet & itemSet);

}//end namespace

#endif