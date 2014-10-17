#include <sserialize/containers/GeneralizedTrie/FlatTrie.h>

namespace sserialize {
namespace GeneralizedTrie {


FlatTrie::FlatTrie() {}
FlatTrie::FlatTrie(bool caseSensitive, bool suffixTrie) : MyBaseClass(caseSensitive, suffixTrie) {}
FlatTrie::~FlatTrie() {}

void FlatTrie::swap( MyBaseClass & baseTrie ) {
	MyBaseClass::swap(baseTrie);
}

void FlatTrie::createStaticFlatTrie(FlatGSTConfig & config) {
	if (m_addTransDiacs) {
		std::cout << "No support for transliterated strings!" << std::endl;
		return;
	}

	if (m_root)
		compactify(m_root);

	std::vector<std::string> trieStrings;
	
	if (m_caseSensitive) {
		trieStrings = m_strings;
	}
	else {
		for(std::vector<std::string>::const_iterator it = m_strings.begin(); it != m_strings.end(); ++it) {
			trieStrings.push_back( unicode_to_lower(*it) );
		}
	}
	{//normalize input
		std::sort(trieStrings.begin(), trieStrings.end());
		std::vector<std::string>::iterator last = std::unique(trieStrings.begin(), trieStrings.end());
		trieStrings.erase(last, trieStrings.end());
	}
	
	FlatTrieEntryConfig flatTrieConfig(config.indexFactory);
	
	
	uint32_t nodeHitCount = 0;
	ProgressInfo progressInfo;
	progressInfo.begin(m_nodeCount);
	for(std::vector<std::string>::const_iterator it(trieStrings.begin()), end(trieStrings.end()); it != end && nodeHitCount < m_nodeCount; it++) {
		flatTrieConfig.curStrId = flatTrieConfig.flatTrieStrings.size();
		flatTrieConfig.strIt = it->begin();
		flatTrieConfig.strBegin = it->begin();
		flatTrieConfig.strEnd = it->end();
		
		uint32_t curNodeHitCount = 0;
		do {
			curNodeHitCount += createFlatTrieEntry(flatTrieConfig);
			nextSuffixString(flatTrieConfig.strIt, flatTrieConfig.strEnd);
		} while (flatTrieConfig.strIt != flatTrieConfig.strEnd && m_isSuffixTrie);

		if (curNodeHitCount > 0) {
			flatTrieConfig.flatTrieStrings.push_back(*it);
			nodeHitCount += curNodeHitCount;
		}
		progressInfo(nodeHitCount, "BaseTrie::createStaticFlatTrie::createTrieArray");
	}
	progressInfo.end("BaseTrie::createStaticFlatTrie::createTrieArray");
	trieStrings.clear();
	
	uint8_t sq = StringCompleter::SQ_EP;
	if (m_caseSensitive)
		sq |= StringCompleter::SQ_CASE_SENSITIVE;
	else
		sq |= StringCompleter::SQ_CASE_INSENSITIVE;
		
	if (m_isSuffixTrie)
		sq |= StringCompleter::SQ_SSP;
	
	
	sserialize::UByteArrayAdapter header = config.destination;
	config.destination << sserialize::Static::StringCompleter::HeaderInfo(sserialize::Static::StringCompleter::T_FLAT_TRIE, 0);

	UByteArrayAdapter::OffsetType beginOffset = config.destination.tellPutPtr();
	config.destination << static_cast<uint8_t>(0); //version
	config.destination << sq;
	config.destination << flatTrieConfig.flatTrieStrings; //string table

	flatTrieConfig.flatTrieStrings = std::vector<std::string>(); //clear doesn't always free the memory


	{//put string ids
		uint32_t largestStrId = 0;
		uint32_t largestBegin = 0;
		uint32_t largestLen = 0;;
		for(std::unordered_map<MultiTrieNode<ItemIdType>*, StringEntry>::const_iterator it = flatTrieConfig.stringEntries.begin(); it != flatTrieConfig.stringEntries.end(); ++it) {
			largestStrId = std::max( (uint32_t) it->second.stringId, largestStrId);
			largestBegin = std::max((uint32_t) it->second.strBegin, largestBegin);
			largestLen = std::max((uint32_t) it->second.strLen, largestLen);
		}
		std::vector<uint8_t> bitConfig;
		bitConfig.push_back( CompactUintArray::minStorageBits(largestStrId) );
		bitConfig.push_back( CompactUintArray::minStorageBits(largestBegin) );
		bitConfig.push_back( CompactUintArray::minStorageBits(largestLen) );
		
		std::vector<Node*> nodesInDepthFirst; nodesInDepthFirst.reserve(m_nodeCount);
		m_root->addNodesSortedDepthFirst(nodesInDepthFirst);
		
		UByteArrayAdapter stringEntriesAdap(new std::vector<uint8_t>(), true);
		MultiVarBitArrayCreator stringEntriesCreator(bitConfig, stringEntriesAdap);
		stringEntriesCreator.reserve(flatTrieConfig.stringEntries.size() );
		for(size_t i = 0, pos = 0; i < nodesInDepthFirst.size() && pos < flatTrieConfig.stringEntries.size(); ++i, ++pos) {
			const StringEntry & se = flatTrieConfig.stringEntries.at( nodesInDepthFirst[i] );
			stringEntriesCreator.set(pos, 0, se.stringId);
			stringEntriesCreator.set(pos, 1, se.strBegin);
			stringEntriesCreator.set(pos, 2, se.strLen);
		}
		flatTrieConfig.stringEntries = std::unordered_map<MultiTrieNode<ItemIdType>*, StringEntry>(); //make sure memory is real freed
		nodesInDepthFirst = std::vector<Node*>();
		config.destination.put(stringEntriesCreator.flush());
	}
	
	fillFlatTrieIndexEntries(flatTrieConfig, config);
	
	sserialize::Static::ArrayCreator<IndexEntry> dc(config.destination);
	FlatGSTIndexEntryMapper indexEntryMapper(dc);
	m_root->mapDepthFirst(indexEntryMapper);
	dc.flush();

	header << sserialize::Static::StringCompleter::HeaderInfo(sserialize::Static::StringCompleter::T_FLAT_TRIE, config.destination.tellPutPtr() - beginOffset);
	
	if (config.deleteTrie)
		clear();

}

void FlatTrie::
fillFlatTrieIndexEntries(FlatTrieEntryConfig & flatTrieConfig, const FlatGSTConfig & config) {
	
	ProgressInfo progressInfo;
	
	std::vector< Node * > nodesInLevelOrder;
	m_root->addNodesSorted(nodesInLevelOrder);
	
	progressInfo.begin(m_nodeCount);
	uint32_t count = 0;
	while (nodesInLevelOrder.size()) {
		Node * curNode = nodesInLevelOrder.back(); nodesInLevelOrder.pop_back();
		FlatGST_TPNS * parentIndexStore = 0;
		if (curNode->parent) {
			if (!curNode->parent->temporalPrivateStorage) {
				curNode->parent->temporalPrivateStorage = new FlatGST_TPNS();
			}
			parentIndexStore = dynamic_cast<FlatGST_TPNS*>(curNode->parent->temporalPrivateStorage);
		}
		FlatGST_TPNS * curNodeIndexStore = dynamic_cast<FlatGST_TPNS*>(curNode->temporalPrivateStorage);
		
		IndexEntry fe;
		fe.mergeIndex = config.mergeIndex;
		
		
		fe.exactValues = flatTrieConfig.indexFactory.addIndex(curNode->exactValues);

		std::vector<ItemIdType> ps;
		if (curNodeIndexStore) {
			if (config.mergeIndex)
				diffSortedContainer(ps, curNodeIndexStore->prefixIndex(), curNode->exactValues);
			else
				mergeSortedContainer(ps, ps, curNodeIndexStore->prefixIndex());
		}
		if (!config.mergeIndex)
			mergeSortedContainer(ps, ps, curNode->exactValues);
		fe.prefixValues = flatTrieConfig.indexFactory.addIndex(ps);
		
		if (parentIndexStore) {
			parentIndexStore->prefixIndexInsert(curNode->exactValues);
			if (curNodeIndexStore)
				parentIndexStore->prefixIndexInsert(curNodeIndexStore->prefixIndex());
		}

		if (m_isSuffixTrie) {
			std::vector<ItemIdType> ss(curNode->subStrValues);
			if (!config.mergeIndex)
				mergeSortedContainer(ss, ss, curNode->exactValues);
			fe.suffixValues = flatTrieConfig.indexFactory.addIndex(ss);
			
			
			std::vector<ItemIdType> sps;
			if (curNodeIndexStore) {
				if (config.mergeIndex) {
					diffSortedContainer(sps, curNodeIndexStore->suffixPrefixIndex(), curNode->exactValues);
					inplaceDiffSortedContainer(sps, ps);
					inplaceDiffSortedContainer(sps, ss);
				}
				else {
					mergeSortedContainer(sps, sps, curNodeIndexStore->suffixPrefixIndex());
				}
			}
			if (!config.mergeIndex) {
				mergeSortedContainer(sps, sps, curNode->subStrValues);
				mergeSortedContainer(sps, sps, curNode->exactValues);
			}
			
			fe.suffixPrefixValues = flatTrieConfig.indexFactory.addIndex(sps);
			
			if (parentIndexStore) {
				parentIndexStore->suffixPrefixIndexInsert(curNode->exactValues);
				parentIndexStore->suffixPrefixIndexInsert(curNode->subStrValues);
				if (curNodeIndexStore)
					parentIndexStore->suffixPrefixIndexInsert(curNodeIndexStore->suffixPrefixIndex());
			}
		}
		
		curNode->deleteTemporalPrivateStorage();
		curNode->temporalPrivateStorage = new FlatGSTIndexEntry_TPNS(fe);
		
		count++;
		if (config.deleteTrie) {
			curNode->exactValues = Node::ItemSetContainer();
			curNode->subStrValues = Node::ItemSetContainer();
			curNode->c = std::string();
		}
		
		progressInfo(count,"BaseTrie::fillFlatTrieIndexEntries");
	}
	progressInfo.end("BaseTrie::fillFlatTrieIndexEntries");
}

/** @param flatTrieConfig.strIt: the string has to be in the trie. correctnes is not checked! */
uint32_t FlatTrie::createFlatTrieEntry(FlatTrieEntryConfig & flatTrieConfig) {
	std::string::const_iterator strIt = flatTrieConfig.strIt;
	
	uint32_t nodeHitCount = 0;
	Node * curNode = m_root;
	while(strIt != flatTrieConfig.strEnd) {
		strIt += curNode->c.size();
		if (flatTrieConfig.stringEntries.count(curNode) == 0) {
			StringEntry se;
			se.stringId = flatTrieConfig.curStrId;
			se.strBegin = flatTrieConfig.strIt - flatTrieConfig.strBegin;
			se.strLen = strIt - flatTrieConfig.strIt;
			flatTrieConfig.stringEntries[curNode] = se;
			nodeHitCount++;
		}
		
		if (strIt != flatTrieConfig.strEnd) {
			curNode = curNode->children.at( utf8::peek_next(strIt, flatTrieConfig.strEnd) );
		}
	}
	return nodeHitCount;
}

bool
FlatTrie::
checkFlatTrieEquality(Node * node, std::string prefix, uint32_t & posInFTrie, const sserialize::Static::FlatGST & trie, bool checkIndex) {
	if (node) {
		prefix += node->c;
		UByteArrayAdapter fgstString = trie.fgstStringAt(posInFTrie);
		if (fgstString.size() != prefix.size()) {
			std::cout << "FlatTrie broken (fgstString.size() != prefix.size()):" << posInFTrie << std::endl;
			node->dump();
			return false;
		}
			
		for(size_t i = 0; i < prefix.size(); i++) {
			if (fgstString.at(i) != static_cast<uint8_t>(prefix[i]) ) {
				std::cout << "FlatTrie broken (fgstString != prefix):" << posInFTrie << std::endl;
				node->dump();
				return false;
			}
		}
		
		//String ok, check index
		if (checkIndex) {
			if (node->exactValues != trie.indexFromPosition(posInFTrie, StringCompleter::QT_EXACT)) {
				std::cout << "FlatTrie broken (exactIndex):" << posInFTrie << std::endl;
				node->dump();
				return false;
			}
			
			if (m_isSuffixTrie) {
				std::set<ItemIdType> s;
				s.insert(node->exactValues.begin(), node->exactValues.end());
				s.insert(node->subStrValues.begin(), node->subStrValues.end());
				if (m_isSuffixTrie && s != trie.indexFromPosition(posInFTrie, StringCompleter::QT_SUFFIX)) {
					std::cout << "FlatTrie broken (exactIndex):" << posInFTrie << std::endl;
					node->dump();
					return false;
				}
			}
			
			{
				std::set<ItemIdType> s;
				node->insertExactValuesRecursive(s);
				if (s != trie.indexFromPosition(posInFTrie, StringCompleter::QT_PREFIX)) {
					std::cout << "FlatTrie broken (exactIndex):" << posInFTrie << std::endl;
					node->dump();
					return false;
				}
				if (m_isSuffixTrie) {
					node->insertAllValuesRecursive(s);
					if (s != trie.indexFromPosition(posInFTrie, StringCompleter::QT_SUBSTRING)) {
						std::cout << "FlatTrie broken (exactIndex):" << posInFTrie << std::endl;
						node->dump();
						return false;
					}
				}
			}
		}
		
		
		//recurse
		posInFTrie++; //move to next "node"
		for(Node::ChildNodeIterator childIt = node->children.begin(); childIt != node->children.end(); ++childIt) {
			if (! checkFlatTrieEquality(childIt->second, prefix, posInFTrie, trie, checkIndex) ) {
				std::cout << "FlatTrie broken (child):" << std::endl;
				node->dump();
				return false;
			}
		}
		
		return true;
	}
	else {
		return false;
	}
}


bool
FlatTrie::
checkFlatTrieEquality(const sserialize::Static::FlatGST & trie, bool checkIndex) {
	uint32_t posInFTrie = 0;
	return (trie.size() == m_nodeCount) && checkFlatTrieEquality(m_root, "", posInFTrie, trie, checkIndex);
}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const FlatTrie::StringEntry & source) {
	destination.putVlPackedUint32(source.stringId);
	destination.putVlPackedUint32(source.strBegin);
	destination.putVlPackedUint32(source.strLen);
	return destination;
}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const FlatTrie::IndexEntry & source) {
	uint8_t header = 0;
	std::deque<uint32_t> indexPtrs;
	if (source.exactValues) {
		header |= 0x1;
		indexPtrs.push_back(source.exactValues);
	}
	if (source.prefixValues) {
		header |= 0x2;
		indexPtrs.push_back(source.prefixValues);
	}
	if (source.suffixValues) {
		header |= 0x4;
		indexPtrs.push_back(source.suffixValues);
	}
	if (source.suffixPrefixValues) {
		header |= 0x8;
		indexPtrs.push_back(source.suffixPrefixValues);
	}
	
	if (source.itemIdIndex)
		header |= 0x10;
	else {
		indexPtrs.push_back(source.minId);
		indexPtrs.push_back(source.maxId);
	}

	if (source.mergeIndex)
		header |= 0x80;
		
	uint32_t largestPtr = std::max(std::max(source.exactValues, source.prefixValues), std::max(source.suffixValues, source.suffixPrefixValues) );
	if (!source.itemIdIndex)
		largestPtr = std::max(largestPtr, source.maxId);
	uint8_t bpn = sserialize::CompactUintArray::minStorageBitsFullBytes(largestPtr);
	header |= ((bpn/8-1) & 0x3) << 5;
	

	std::deque<uint8_t> sidxPtrs; 
	sserialize::CompactUintArray::createFromSet(indexPtrs, sidxPtrs, bpn);
	
	destination << header;
	destination.put(sidxPtrs);
	return destination;
}

}}//end namespace