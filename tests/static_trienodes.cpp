#include <iostream>
#include <sstream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>

#include <sserialize/containers/GeneralizedTrie.h>
#include <sserialize/Static/TrieNodePrivates/TrieNodePrivates.h>

//uint8_t charWidth, bool withSubTries, const std::string & str, const std::deque<uint16_t> & childChars, bool fullIndex, uint32_t indexPtr, std::deque<uint8_t> & destination

using namespace sserialize;
using namespace sserialize::Static;


struct NodeConfig {
	NodeConfig() :
		charWidth(0), indexType(TrieNodePrivate::IT_NONE),
		childCount(0),
		exactIndexPtr(0), prefixIndexPtr(0), suffixIndexPtr(0), suffixPrefixIndexPtr(0),
		nodeSize(0), nodeOffSet(0) {}
	//Creation config
	uint8_t charWidth;
	sserialize::Static::TrieNodePrivate::IndexTypes indexType;


	//Filled during creation
	std::string nodeStr;
	uint16_t childCount;
	std::vector<uint32_t> childChars;
	std::vector<uint32_t> childOffSets;
	uint32_t exactIndexPtr;
	uint32_t prefixIndexPtr;
	uint32_t suffixIndexPtr;
	uint32_t suffixPrefixIndexPtr;

	//raw node data
	std::deque<uint8_t> rawNodeData;
	uint32_t nodeSize;
	uint32_t nodeOffSet;

	Static::TrieNodeCreationInfo creationNodeInfo() {
		TrieNodeCreationInfo info;
		info.charWidth = charWidth;
		info.childChars = childChars;
		info.childPtrs = childOffSets;
		info.exactIndexPtr = exactIndexPtr;
		info.indexTypes = indexType;
		info.nodeStr = nodeStr;
		info.prefixIndexPtr = prefixIndexPtr;
		info.suffixIndexPtr = suffixIndexPtr;
		info.suffixPrefixIndexPtr = suffixPrefixIndexPtr;
		return info;
	}

};

std::string baseStringChars("ABCDEFGHIJKLMOPQRSTUVWXYZabcdevghijklmnopqrstuvxyz1234567890!$%&/{}()[]+-*~<>");

template<class StaticTrieNodeT>
void createNodeBottomUp(NodeConfig & config) {

	uint8_t strLen = (double)rand()/RAND_MAX * 255;
	for(uint8_t i = 0; i < strLen; i++) {
		char c = baseStringChars.at((double)rand()/RAND_MAX * baseStringChars.size());
		config.nodeStr.push_back(c);
	}

	config.childCount = (static_cast<uint32_t>((double)rand()/RAND_MAX * 2048)) & 0x1FF;
	if (config.childCount) {
		config.childOffSets.push_back(0);
		for(uint16_t i = 1; i < config.childCount; i++) {
			uint32_t addOffSet = (double)rand()/RAND_MAX * 0x1FF;
			config.childOffSets.push_back(config.childOffSets.back()+addOffSet);
		}
		for(uint16_t i = 0; i < config.childCount; i++) {
			if (config.charWidth == 2) {
				config.childChars.push_back((double)rand()/RAND_MAX * 0xFFFF);
			}
			else if (config.charWidth == 1) {
				config.childChars.push_back((double)rand()/RAND_MAX * 0xFF);
			}
			else {
				std::cout << "Tester: Unsupported charWidth" << std::endl;
			}
		}
	}
	
	config.exactIndexPtr = rand() + rand();
	config.prefixIndexPtr = rand() + rand();
	config.suffixIndexPtr = rand() + rand();
	config.suffixPrefixIndexPtr = rand() + rand();
	config.nodeOffSet = rand() & 0xFFF;
	std::deque<uint8_t> nodeData(config.nodeOffSet, 0xFE);
	

	//uint8_t charWidth, const std::string& nodeString, const std::deque< uint16_t >& childChars, const std::deque< uint32_t >& childPtrs, bool fullIndex, uint32_t indexPtr, std::deque< uint8_t >& destination
	unsigned int err = StaticTrieNodeT::prependNewNode(config.creationNodeInfo(), nodeData);
	
	
	if (StaticTrieNodeT::isError(err)) {
		std::cout << "Error while creating node:" << StaticTrieNodeT::errorString(err) << std::endl;
	}


	config.nodeSize = nodeData.size()-config.nodeOffSet;
	prependToDeque(std::deque<uint8_t>(config.nodeOffSet, 0xFE), nodeData);

	config.rawNodeData.swap(nodeData);
}

template<typename A, typename B>
std::ostream&
printDebugShouldIs(std::string what, A a, B b, std::ostream & out = std::cout) {
	out << what << ", Should: " << a << "; is: " << b << std::endl;
	return out;
}

template<typename A, typename B>
bool checkAndDebugPrint(std::string what, A a, B b, std::ostream & out = std::cout) {
	if (a != b) {
		printDebugShouldIs(what, a, b);
		return false;
	}
	return true;
}

template<class StaticTrieNodeT>
bool testNode(NodeConfig & config, StaticTrieNodeT & node) {

	bool allOk = true;

	if (! checkAndDebugPrint("charWidth", config.charWidth, node.charWidth())) {
		allOk = false;
	}

	if (! checkAndDebugPrint("exactIndex", (bool) (config.indexType & TrieNodePrivate::IT_EXACT), node.hasExactIndex()) ) {
		allOk = false;
	}

	if (! checkAndDebugPrint("prefixIndex", (bool) (config.indexType & TrieNodePrivate::IT_PREFIX), node.hasPrefixIndex()) ) {
		allOk = false;
	}

	if (! checkAndDebugPrint("suffixIndex", (bool) (config.indexType & TrieNodePrivate::IT_SUFFIX), node.hasSuffixIndex()) ) {
		allOk = false;
	}

	if (! checkAndDebugPrint("suffixPrefixIndex", (bool) (config.indexType & TrieNodePrivate::IT_SUFFIX_PREFIX), node.hasSuffixPrefixIndex()) ) {
		allOk = false;
	}

	if (! checkAndDebugPrint("childCount", config.childCount, node.childCount())) {
		allOk = false;
	}

	if (! checkAndDebugPrint("storageSize", config.nodeSize, node.getStorageSize())) {
		allOk = false;
	}

	uint32_t cummulatedStorageSize = node.getHeaderStorageSize() + node.getNodeStringStorageSize();
	cummulatedStorageSize += node.getChildPtrStorageSize() + node.getChildCharStorageSize();
	cummulatedStorageSize += node.getIndexPtrStorageSize();

	if (! checkAndDebugPrint("cummulated Storage Size", config.nodeSize, cummulatedStorageSize) ) {
		allOk = false;
	}


	if (config.nodeStr.size() == 0 && node.strLen() != 0) {
		printDebugShouldIs("nodeStr", 0, node.strLen());
		allOk = false;
	}

	//This depends on single wide chars
	if (config.nodeStr.size() > 0 && (config.nodeStr.size()-1 != node.strLen())) {
		printDebugShouldIs("nodeStr", 0, node.strLen());
		allOk = false;
	}
	
	if ((config.indexType & TrieNodePrivate::IT_EXACT) && ! checkAndDebugPrint("exactIndexPtr", config.exactIndexPtr, node.getExactIndexPtr()) )
		allOk = false;

	if ((config.indexType & TrieNodePrivate::IT_PREFIX) && ! checkAndDebugPrint("prefixIndexPtr", config.prefixIndexPtr, node.getPrefixIndexPtr()) )
		allOk = false;

	if ((config.indexType & TrieNodePrivate::IT_SUFFIX) && ! checkAndDebugPrint("suffixIndexPtr", config.suffixIndexPtr, node.getSuffixIndexPtr()) )
		allOk = false;

	if ((config.indexType & TrieNodePrivate::IT_SUFFIX_PREFIX) && ! checkAndDebugPrint("suffixPrefixIndexPtr", config.suffixPrefixIndexPtr, node.getSuffixPrefixIndexPtr()) )
		allOk = false;

	std::string nodeStr = node.str();
	for(uint8_t i = 0; i < node.strLen(); i++) {
		std::stringstream ss;
		ss << "Single node char at position " << i;
		if (! checkAndDebugPrint(ss.str(), config.nodeStr.at(i+1), nodeStr.at(i)) ) {
			allOk = false;
		}
	}

	for(uint32_t i = 0; i < node.childCount(); i++) {
		std::stringstream ss;
		ss << "Single child char at position " << i;
		if (! checkAndDebugPrint(ss.str(), config.childChars.at(i), node.childCharAt(i)) ) {
			allOk = false;
			break;
		}
	}


	for(uint32_t i = 0; i < node.childCount(); i++) {
		std::stringstream ss;
		ss << "Single child ptr at position " << i;
		uint32_t realChildPtr = config.childOffSets.at(i);
		if (! checkAndDebugPrint(ss.str(), realChildPtr, node.getChildPtr(i)) ) {
			allOk = false;
			break;
		}
	}
	return allOk;
}

template<class StaticTrieCreationNodeT, class StaticTrieNodeT>
bool createAndTestNodePrivateBottomUp(NodeConfig & config) {
	createNodeBottomUp<StaticTrieCreationNodeT>(config);

	UByteArrayAdapter adap(&config.rawNodeData);
	StaticTrieNodeT node(adap + config.nodeOffSet);
	
	if (testNode<StaticTrieNodeT>(config, node)) {
		return true;
	}
	else {
		std::cout << "Node testing failed!" << std::endl;
		return false;
	}
}


template<class StaticTrieCreationNodeT, class StaticTrieNodeT>
bool testNodeImplementationPrivateBottumUp() {

	uint8_t charWidth[2] = { 1, 2};

	bool allOk = true;
	for(size_t cWIT = 0; cWIT < 2; cWIT++) {
		for(size_t indexTypeIt = 1; indexTypeIt <= 0xF; indexTypeIt++) {
			NodeConfig config;
			config.charWidth = charWidth[cWIT];
			config.indexType = (TrieNodePrivate::IndexTypes) indexTypeIt;
			if (!createAndTestNodePrivateBottomUp<StaticTrieCreationNodeT, StaticTrieNodeT>(config)) {
				allOk = false;
			}
		}
	}
	
	if (allOk) {
		std::cout << "All tests passed" << std::endl;
	}
	else {
		std::cout << "At least one test failed" << std::endl;
	}
	return allOk;
}


int main() {
	srand(0);

	std::cout << "Testing SimpleTrieNodePrivate" << std::endl;
	testNodeImplementationPrivateBottumUp<SimpleStaticTrieCreationNode, SimpleTrieNodePrivate>();

	std::cout << "Testing CompactTrieNodePrivate" << std::endl;
	testNodeImplementationPrivateBottumUp<CompactStaticTrieCreationNode, CompactTrieNodePrivate>();


	
	return 0;
}