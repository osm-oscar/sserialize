#ifndef SSERIALIZE_TESTS_STRING_COMPLETER_H
#define SSERIALIZE_TESTS_STRING_COMPLETER_H
#include <sserialize/search/StringCompleter.h>
#include <sserialize/containers/ItemSet.h>
#include "TestItemData.h"

namespace sserialize {

enum StringCompleterTestErrors {
	SCTE_NONE = 0,
	SCTE_SQ=1,
	SCTE_EXACT=2,
	SCTE_EXACT_CI=4,
	SCTE_PREFIX=8,
	SCTE_PREFIX_CI=16,
	SCTE_SUFFIX=32,
	SCTE_SUFFIX_CI=64,
	SCTE_SUFFIX_PREFIX=128,
	SCTE_SUFFIX_PREFIX_CI=256,
	}; 

bool testStringCompleterQuerry(const std::deque< std::string >& testStrings, sserialize::StringCompleter & strCmp, const std::deque< sserialize::TestItemData >& data, unsigned int qt);
bool testPartialStringCompleterQuerry(const std::deque< std::string >& testStrings, sserialize::StringCompleter & strCmp, const std::deque< sserialize::TestItemData >& data, unsigned int qt);
StringCompleterTestErrors testStringCompleter(sserialize::StringCompleter & strCmp, const std::deque< sserialize::TestItemData >& data, sserialize::StringCompleter::SupportedQuerries sq);

}//end namespace

#endif