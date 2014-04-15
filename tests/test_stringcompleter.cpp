#include  "test_stringcompleter.h"
#include "trietestfuncs.h"

namespace sserialize {


bool testStringCompleterQuerry( const std::deque< std::string >& testStrings, sserialize::StringCompleter & strCmp, const std::deque< sserialize::TestItemData >& data, unsigned int qt ) {
	StringCompleter::QuerryType mqt = (StringCompleter::QuerryType) qt;
	for(size_t i = 0; i < testStrings.size(); i++) {
		ItemIndex strCmpIdx( strCmp.complete(testStrings[i], mqt) );
		std::set<uint32_t> strCmpSet( getItemIds(data, strCmpIdx) );
		std::set<uint32_t> realSet( getItemIdsWithString(testStrings[i], mqt, data) );
		if (strCmpSet != realSet) {
			strCmp.clearCache();
			ItemIndex strCmpIdx( strCmp.complete(testStrings[i], mqt) );
			std::set<uint32_t> strCmpSet( getItemIds(data, strCmpIdx) );
			std::set<uint32_t> realSet( getItemIdsWithString(testStrings[i], mqt, data) );
			return false;
		}
	}
	return true;
}

bool testPartialStringCompleterQuerry( const std::deque< std::string >& testStrings, sserialize::StringCompleter & strCmp, const std::deque< sserialize::TestItemData >& data, unsigned int qt ) {
	StringCompleter::QuerryType mqt = (StringCompleter::QuerryType) qt;
	for(size_t i = 0; i < testStrings.size(); i++) {
		ItemIndex strCmpIdx( strCmp.partialComplete(testStrings[i], mqt).toItemIndex() );
		std::set<uint32_t> strCmpSet( getItemIds(data, strCmpIdx) );
		std::set<uint32_t> realSet( getItemIdsWithString(testStrings[i], mqt, data) );
		if (strCmpSet != realSet) {
			strCmp.clearCache();
			ItemIndex strCmpIdx( strCmp.complete(testStrings[i], mqt) );
			std::set<uint32_t> strCmpSet( getItemIds(data, strCmpIdx) );
			std::set<uint32_t> realSet( getItemIdsWithString(testStrings[i], mqt, data) );
			return false;
		}
	}
	return true;
}


StringCompleterTestErrors testStringCompleter( sserialize::StringCompleter& strCmp, const std::deque< sserialize::TestItemData >& data, sserialize::StringCompleter::SupportedQuerries sq ) {
	unsigned int err = SCTE_NONE;
	if (strCmp.getSupportedQuerries() != sq) {
		err |= SCTE_SQ;
		sq = strCmp.getSupportedQuerries();
	}
	
	std::deque< std::string > testStrings( createSampleSingleCompletionStrings() );
	
	if (sq & StringCompleter::SQ_CASE_SENSITIVE) {
		if (sq & StringCompleter::SQ_EXACT && !testStringCompleterQuerry(testStrings, strCmp, data, StringCompleter::QT_EXACT))
			err |= SCTE_EXACT;
		if (sq & StringCompleter::SQ_PREFIX && !testStringCompleterQuerry(testStrings, strCmp, data, StringCompleter::QT_PREFIX))
			err |= SCTE_PREFIX;
		if (sq & StringCompleter::SQ_SUFFIX && !testStringCompleterQuerry(testStrings, strCmp, data, StringCompleter::QT_SUFFIX))
			err |= SCTE_SUFFIX;
		if (sq & StringCompleter::SQ_SUBSTRING && !testStringCompleterQuerry(testStrings, strCmp, data, StringCompleter::QT_SUBSTRING))
			err |= SCTE_SUFFIX_PREFIX;
	}
	if (sq & StringCompleter::SQ_CASE_INSENSITIVE) {
		if (sq & StringCompleter::SQ_EXACT && !testStringCompleterQuerry(testStrings, strCmp, data, StringCompleter::QT_EXACT | StringCompleter::QT_CASE_INSENSITIVE))
			err |= SCTE_EXACT_CI;
		if (sq & StringCompleter::SQ_PREFIX && !testStringCompleterQuerry(testStrings, strCmp, data, StringCompleter::QT_PREFIX | StringCompleter::QT_CASE_INSENSITIVE))
			err |= SCTE_PREFIX_CI;
		if (sq & StringCompleter::SQ_SUFFIX && !testStringCompleterQuerry(testStrings, strCmp, data, StringCompleter::QT_SUFFIX | StringCompleter::QT_CASE_INSENSITIVE))
			err |= SCTE_SUFFIX_CI;
		if (sq & StringCompleter::SQ_SUBSTRING && !testStringCompleterQuerry(testStrings, strCmp, data, StringCompleter::QT_SUBSTRING | StringCompleter::QT_CASE_INSENSITIVE))
			err |= SCTE_SUFFIX_PREFIX_CI;
	}
	return (StringCompleterTestErrors) err;
}



}//end namespace