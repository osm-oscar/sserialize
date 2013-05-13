#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <set>
#include <cassert>
#include "../include/sserialize/vendor/utf8.h"
#include <sserialize/utility/TimeMeasuerer.h>

typedef std::vector<std::string> StringDataBase;

inline bool utf8Smaller(std::string::const_iterator itA, std::string::const_iterator endA, std::string::const_iterator itB, std::string::const_iterator endB) {
		while (itA < endA && itB < endB) {
			uint32_t aCode = utf8::next(itA, endA);
			uint32_t bCode = utf8::next(itB, endB);
			if (aCode < bCode)
				return true;
			else if (bCode < aCode)
				return false;
		}
		return (itB < endB);
}

struct Utf8Compare {
	bool operator()(const std::string & a, const std::string & b) const {
		return utf8Smaller(a.begin(), a.end(), b.begin(), b.end());
	};
};

struct SubString {
	uint32_t stringId;
	uint16_t begin;
	uint16_t end;
	std::string toString(const StringDataBase & db) const {
		const std::string & base = db.at(stringId);
		return std::string(base.begin()+begin, base.begin()+end);
	}
	uint16_t len() const {
		return end-begin;
	}
	
	bool valid() const {
		return begin < end;
	}
	
	bool smallerThan(const SubString & other, const StringDataBase & db) {
		const std::string & myBase = db.at(stringId);
		const std::string & oBase = db.at(other.stringId);
		return utf8Smaller(myBase.begin()+begin, myBase.begin()+end, oBase.begin()+other.begin, oBase.begin()+other.end);
	}
};

struct SubStringCompare  {
	StringDataBase * db;
	bool operator()(const SubString & a, const SubString & b) {
		return a.smallerThan(b, *db);
	}
};

struct Node {
	struct StringInfo {
		StringInfo(const std::string::const_iterator & begin, const std::string::const_iterator & end) :
		begin(begin), end(end) {}
		std::string::const_iterator begin;
		std::string::const_iterator end;
	};
	Node(std::vector<StringInfo> & srcStrsIt) : srcStrsIt(srcStrsIt) {
		begin = 0;
		end = srcStrsIt.size();
	}
	Node(std::vector<StringInfo> & srcStrsIt, int32_t begin, int32_t end, const std::string & prefix) : 
	srcStrsIt(srcStrsIt),
	begin(begin),
	end(end),
	prefix(prefix)
	{}
	~Node() {}
	
	std::vector<StringInfo> & srcStrsIt;
	int32_t begin;
	int32_t end;
	bool hasEndIterator() {
		return (begin < end && srcStrsIt[begin].begin >= srcStrsIt[begin].end);
	}
	///@return token with token begin and token end

	typedef std::map<uint32_t, std::pair<int32_t, int32_t> > NextTokensType;
	NextTokensType nextTokens() {
		assert(valid());
		NextTokensType ret;
		int32_t tokenBegin = begin;
		uint32_t curRunToken = utf8::next(srcStrsIt[tokenBegin].begin, srcStrsIt[tokenBegin].end);
		for(int32_t i = tokenBegin+1; i < end; ++i) {
			uint32_t token = utf8::next(srcStrsIt[i].begin, srcStrsIt[i].end);
			if (token != curRunToken) {
				ret[curRunToken] = std::pair<int32_t, int32_t>(tokenBegin, i);
				tokenBegin = i;
				curRunToken = token;
			}
		}
		ret[curRunToken] = std::pair<int32_t, int32_t>(tokenBegin, end);
		return ret;
	}
	
	std::string prefix;
	
	bool valid() {
		return begin < end;
	}
	
	///Next child node if this is a inner/end-node
	Node childNode() const {
		return Node(srcStrsIt, begin+1, end, prefix);
	}
	
	Node childNode(uint32_t begin, uint32_t end, uint32_t addToPrefix) const {
		std::string p(prefix);
		utf8::append(addToPrefix, std::back_insert_iterator<std::string>(p));
		return Node(srcStrsIt, begin, end, p);
	}
};

void createFGST(Node node, std::vector<std::string> & dest) {
	if (node.valid()) {
		//First check if we have to add a inner node
		//This is an inner node if theres a string that ends here, then just append that to dest and we're done (it's a endnode aswell)
		//otherwise increase strIdent until either we find a inner node thats an endnode as well or there are at least two different
		//the (inner+end)node can only be the first one due to the sorted nature of the input
		if (node.hasEndIterator()) { //end node reached, append it and recurse with the rest
			dest.push_back(node.prefix);
			createFGST(node.childNode(), dest);
		}
		else {
			std::map<uint32_t, std::pair<int32_t, int32_t> > nextTokens(node.nextTokens());
			if (nextTokens.size() > 1) { //check if we need a inner node
				dest.push_back(node.prefix);
			}
			for(Node::NextTokensType::const_iterator it(nextTokens.begin()); it != nextTokens.end(); ++it) {
				createFGST( node.childNode(it->second.first, it->second.second, it->first), dest);
			}
		}
	}
}

int main(int argc, char ** argv) {
	std::string inFileName;
	if (argc > 1) {
		inFileName = std::string(argv[1]);
	}

	std::vector<std::string> itemStrings;
	
	if (inFileName.empty()) {
		std::string itemStr;
		while (std::getline(std::cin, itemStr)) {
			itemStrings.push_back(itemStr);
		}
	}
	else {
		std::ifstream inFile;
		inFile.open(inFileName);
		if (!inFile.is_open()) {
			std::cerr << "Failed to open file" << std::endl;
			return -1;
		}
		std::string itemStr;
		while (std::getline(inFile, itemStr)) {
			itemStrings.push_back(itemStr);
		}
	}
	std::cerr << "Processed " << itemStrings.size() << " item strings" << std::endl;
	sserialize::TimeMeasurer tm;
	tm.begin();
	std::sort(itemStrings.begin(), itemStrings.end(), Utf8Compare());
	{
		std::vector<std::string> tmp;
		tmp.reserve(itemStrings.size());
		std::unique_copy(itemStrings.begin(), itemStrings.end(), std::back_insert_iterator< std::vector<std::string> >(tmp));
		itemStrings = tmp;
	}
	tm.end();
	std::cerr << "Preprocessing took " << tm.elapsedMilliSeconds() << " ms" << std::endl;
	
	std::vector<std::string> gstStrings;
	gstStrings.reserve(itemStrings.size());
	
	
	{
		std::vector<Node::StringInfo> itemStrIts;
		itemStrIts.reserve(itemStrings.size());
		for(std::vector<std::string>::const_iterator it(itemStrings.begin());  it != itemStrings.end(); ++it) {
			itemStrIts.push_back(Node::StringInfo(it->cbegin(), it->cend()));
		}
		
		Node rootNode(itemStrIts);
		tm.begin();
		createFGST(rootNode, gstStrings);
		tm.end();
	}
	std::cerr << "Creating the fgst took " << tm.elapsedMilliSeconds() << " ms" << std::endl;
	
	for(std::vector<std::string>::const_iterator it(gstStrings.begin()); it != gstStrings.end(); ++it) {
		std::cout << *it << std::endl;
	}
	
	return 0;
}