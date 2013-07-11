#include "printfunctions.h"
#include <iostream>

namespace sserialize {

void printBranchingFactorCount(std::map<uint16_t, uint32_t> m) {
	std::cout << "Branching factor counts: " << std::endl;
	for(std::map<uint16_t, uint32_t>::iterator i = m.begin(); i != m.end(); ++i) {
		std::cout << i->first << ": " << i->second << "; ";
	}
	std::cout << std::endl;
}

void printCompletionQuery(const std::deque<std::string> & q) {
	std::cout << "(";
	for(std::deque<std::string>::const_iterator it = q.begin(); it != q.end(); it++) {
		std::cout << *it << ",";
	}
	std::cout << ")";
}

void printCompletionSet(const sserialize::StringsItemDBWrapper< sserialize::TestItemData >& db, sserialize::ItemIndex trieSet) {
	for(size_t i = 0; i < trieSet.size(); i++) {
		const TestItemData & item = db.at( trieSet.at(i) );
		std::cout << "[(";
		for(std::deque<std::string>::const_iterator sit = item.strs.begin(); sit != item.strs.end(); sit++) {
			std::cout << *sit << ",";
		}
		std::cout << "):" << item.id << "]";
	}
}


void printCompletionSet(const  std::deque<TestItemData> & db, ItemIndex trieSet) {
	for(size_t i = 0; i < trieSet.size(); i++) {
		const TestItemData & item = db.at( trieSet.at(i) );
		std::cout << "[(";
		for(std::deque<std::string>::const_iterator sit = item.strs.begin(); sit != item.strs.end(); sit++) {
			std::cout << *sit << ",";
		}
		std::cout << "):" << item.id << "]";
	}
}

void printCompletionSet(const std::set<unsigned int> & set, const std::deque<TestItemData> & items) {
	for(size_t i = 0; i < items.size(); i++) {
		if (set.count(items.at(i).id) > 0) {
			std::cout << "[(";
			for(std::deque<std::string>::const_iterator sit = items.at(i).strs.begin(); sit != items.at(i).strs.end(); sit++) {
				std::cout << *sit << ",";
			}
			std::cout << "):" << items.at(i).id << std::cout << "]";
		}
	}
}


void printCompletionSet(std::deque< ItemIndex > & set, std::deque<TestItemData> & items) {
	for(std::deque<ItemIndex>::iterator setIt = set.begin(); setIt != set.end(); setIt++) {
		for(uint32_t i = 0; i < setIt->size() ; i++) {
			uint32_t itemId = setIt->at(i);
			TestItemData tdata = items.at(itemId);
			std::cout << "(";
			for(std::deque<std::string>::iterator sit = tdata.strs.begin(); sit != tdata.strs.end(); sit++) {
				std::cout << *sit << ",";
			}
			std::cout << ")";
		}
	}
}

void printCompletionSet(TestItemDataItemSet & set) {
	for(size_t i = 0; i < set.size(); i++) {
		TestItemDataDBItem item = set.at(i);
		std::cout << "[(";
		for(size_t j = 0; j < item.strCount(); j++) {
			std::cout << item.strAt(j) << ",";
		}
		std::cout << "):";
		std::cout << item.data().id;
		std::cout << "]";
	}
}


std::string toString(sserialize::StringCompleter::QuerryType qt) {
	std::string str = "(";
	if (qt & sserialize::StringCompleter::QT_EXACT) {
		str += "exact,";
	}
	if (qt & sserialize::StringCompleter::QT_PREFIX) {
		str += "prefix,";
	}
	if (qt & sserialize::StringCompleter::QT_SUFFIX) {
		str += "suffix,";
	}
	if (qt & sserialize::StringCompleter::QT_SUFFIX_PREFIX) {
		str += "suffix_prefix,";
	}
	if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
		str += "case-insensitive";
	}
	else {
		str += "case-sensitive";
	}
	str += ")";
	return str;
}

}//end namespace