#ifndef SSERIALIZE_TESTS_PRINT_FUNCTIONS_H
#define SSERIALIZE_TESTS_PRINT_FUNCTIONS_H
#include <sserialize/containers/GeneralizedTrie.h>
#include "TestItemData.h"

namespace sserialize {

void printBranchingFactorCount(std::map<uint16_t, uint32_t> m);
void printCompletionQuery(const std::deque<std::string> & q);

void printCompletionSet(const StringsItemDBWrapper< TestItemData >& db, ItemIndex trieSet);

void printCompletionSet(std::deque< ItemIndex > & set, std::deque<TestItemData> & items);

void printCompletionSet(TestItemDataItemSet & set);

void printCompletionSet(const std::set<unsigned int> & set, const std::deque<TestItemData> & items);

std::string toString(sserialize::StringCompleter::QuerryType qt);

}

#endif