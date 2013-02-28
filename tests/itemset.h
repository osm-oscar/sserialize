#ifndef SSERIALIZE_TESTS_ITEM_SET_H
#define SSERIALIZE_TESTS_ITEM_SET_H
#include "stringcompleter.h"

namespace sserialize {

template<typename DataBaseItem, typename DataBaseType>
bool testItemSet(ItemSet<DataBaseItem, DataBaseType> itemSet, std::deque< sserialize::TestItemData >& data, sserialize::StringCompleter::SupportedQuerries sq) {

}

StringCompleterTestErrors testItemSet(StringCompleter strCmp, std::deque<TestItemData> & data, StringCompleter::SupportedQuerries sq);



}//end namespace


#endif