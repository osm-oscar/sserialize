#include "trietestfuncs.h"
#include <stdlib.h>
#include <iostream>

namespace sserialize {

std::set<unsigned int> itemIdsFromTrieSet(const sserialize::StringsItemDBWrapper< sserialize::TestItemData >& db, sserialize::ItemIndex trieSet) {
	std::set<unsigned int> res;
	for(size_t i = 0; i < trieSet.size(); i++) {
		res.insert(db.at(trieSet.at(i)).id);
	}
	return res;
}

std::set<unsigned int> itemIdsFromTrieSet(TestItemDataItemSet & itemSet) {
	std::set<unsigned int> res;
	
	for(uint32_t i = 0; i < itemSet.size(); i++) {
		TestItemDataDBItem item = itemSet.at(i);
		res.insert(item.data().id);
	}
	return res;
}

bool checkCompletionSetsEquality(const sserialize::StringsItemDBWrapper< sserialize::TestItemData >& db, sserialize::ItemIndex trieSet, sserialize::TestItemDataItemSet& sset) {
	std::set<uint32_t> mset(itemIdsFromTrieSet(db, trieSet));
	if (mset.size() != sset.size())
		return false;
	uint32_t count = 0;
	for(uint32_t i = 0; i < sset.size(); i++) {
		if (mset.count(sset.at(i).data().id) > 0)
			count++;
	}
	if (count != mset.size())
		return false;
	else
		return true;
}



}//end namespace