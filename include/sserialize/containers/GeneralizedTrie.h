#ifndef COMPLETIONTREE_MULTIRADIXTRIE_H
#define COMPLETIONTREE_MULTIRADIXTRIE_H
#include "GeneralizedTrieHelpers.h"
#include <sserialize/containers/StringsItemDBWrapper.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/templated/StringsItemDBWrapperPrivateSIDB.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/Static/FlatGeneralizedTrie.h>
#include <sserialize/utility/debuggerfunctions.h>
#include <assert.h>
#include <sserialize/utility/DiacriticRemover.h>
#define GST_ITEMID_MAX 0xFFFFFFFF

//TODO:This is one piece of ugly code which needs ot be refactored (or better rewritten)
//BUG: if a child is pointed at by a codepoint larger than uint16_max then this will break the tree

namespace sserialize {

template<class ItemType>
class GeneralizedTrie {
public:
	typedef unsigned int ItemIdType;
	typedef MultiTrieNode<ItemIdType> Node;
private:
	class TemporalPrivateNodeStorage: public MultiTrieNode<unsigned int>::TemporalPrivateStorage {
	public:
		TemporalPrivateNodeStorage() : TemporalPrivateStorage(), childrenCompleted(0), prefixComparisonCount(0), suffixComparisonCount(0) {}
		virtual ~TemporalPrivateNodeStorage() {}
		uint32_t childrenCompleted;
		std::map<uint32_t, uint32_t> remappedPrefixIdxIds;
		std::map<uint32_t, uint32_t> remappedSuffixIdxIds;
		std::deque<uint32_t> staticChildPointers;
		Node::ItemSetContainer prefixIndex; //same as curNode->insertExactValuesRecursive()
		Node::ItemSetContainer suffixIndex; //same as curNode->insertSubStrValuesRecursive()
		uint64_t prefixComparisonCount; //number of comparissons to create prefixIndex
		uint64_t suffixComparisonCount; // number of comparissons to create suffixIndex
	};
	
	class FlatGST_TPNS: public MultiTrieNode<unsigned int>::TemporalPrivateStorage {
	public:
		typedef std::vector<ItemIdType> ItemIdContainerType;
	private:
		ItemIdContainerType m_prefixIndex;
		ItemIdContainerType m_suffixPrefixIndex;
	public:
		FlatGST_TPNS() {}
		virtual ~FlatGST_TPNS() {}
		template<class TSortedContainer>
		void prefixIndexInsert(const TSortedContainer & c) {
			mergeSortedContainer(m_prefixIndex, m_prefixIndex, c);
		}
		template<class TSortedContainer>
		void suffixPrefixIndexInsert(const TSortedContainer & c) {
			mergeSortedContainer(m_suffixPrefixIndex, m_suffixPrefixIndex, c);
		}
		
		const ItemIdContainerType & prefixIndex() const { return m_prefixIndex; }
		const ItemIdContainerType & suffixPrefixIndex() const { return m_suffixPrefixIndex; }
	};

	class FlatGSTIndexEntry_TPNS: public MultiTrieNode<unsigned int>::TemporalPrivateStorage {
	public:
		FlatGSTIndexEntry_TPNS(const IndexEntry & e) : e(e) {}
		virtual ~FlatGSTIndexEntry_TPNS() {}
		IndexEntry e;
	};
	
	class FlatGSTStrIds_TPNS: public MultiTrieNode<unsigned int>::TemporalPrivateStorage {
	public:
		FlatGSTStrIds_TPNS() : itemIdIndex(true), minId(GST_ITEMID_MAX), maxId(0) {}
		virtual ~FlatGSTStrIds_TPNS() {}
		bool itemIdIndex;
		ItemIdType minId;
		ItemIdType maxId;
		Node::ItemSetContainer exactIndex;
		Node::ItemSetContainer suffixIndex;
		template<class TSortedContainer>
		void prefixIndexInsert(const TSortedContainer & c) {
			mergeSortedContainer(prefixIndex, prefixIndex, c);
		}
		template<class TSortedContainer>
		void suffixPrefixIndexInsert(const TSortedContainer & c) {
			mergeSortedContainer(suffixPrefixIndex, suffixPrefixIndex, c);
		}
		std::vector<ItemIdType> prefixIndex;
		std::vector<ItemIdType> suffixPrefixIndex;
	};
	
	struct FlatGSTIndexEntryMapper {
		sserialize::Static::DequeCreator<IndexEntry> & dc;
		FlatGSTIndexEntryMapper(sserialize::Static::DequeCreator<IndexEntry> & dc) : dc(dc) {}
		void operator()(Node * node) {
			FlatGSTIndexEntry_TPNS * p = dynamic_cast<FlatGSTIndexEntry_TPNS*>(node->temporalPrivateStorage);
			dc.put(p->e);
			node->deleteTemporalPrivateStorage();
		}
	};

	struct NodeIndexSets {
		typedef std::vector<uint32_t> StorageType;
		NodeIndexSets() : mergeIndex(true), hasExactValues(false), hasPrefixValues(false), hasSuffixValues(false), hasSuffixPrefixValues(false) {}
		bool mergeIndex;
		bool hasExactValues;
		bool hasPrefixValues;
		bool hasSuffixValues;
		bool hasSuffixPrefixValues;
		std::vector<ItemIdType> exactValues;
		//same as prefixIndex from above minus exactValues
		std::vector<ItemIdType> prefixValues;
		//contains only the REAL suffix values
		std::vector<ItemIdType> suffixValues;
		//same as suffixIndex from above minus (suffixValues + exactValues);
		//BUT: if indirectIndex is set and node gets a full index, then this contains ALL, including suffixValue+prefixValues  
		std::vector<ItemIdType> suffixPrefixValues;
	};

private: //variables
	uint32_t m_count;
	uint32_t m_nodeCount;
	Node * m_root;
	StringsItemDBWrapper<ItemType> m_db;
	bool m_caseSensitive;
	bool m_addTransDiacs;
	bool m_isSuffixTrie;
	std::unordered_set<uint32_t> m_suffixDelimeters;


private: //Insertion function
	std::deque< std::string > createInsertStrings(const std::deque< std::string >& sourceStrs, std::set< std::string >& fullStrs, std::set< std::string >& suffixStrings);
	Node * nextSuffixNode(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd);
	///moves to the next suffix entry and returns the beginning
	void nextSuffixString(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd);
	
private: //completion functions
	void completionRecurse(Node * node, std::string::const_iterator strBegin, const std::string::const_iterator & strEnd, sserialize::StringCompleter::QuerryType qt, std::set<ItemIdType> & destination) const;

private: //static trie creation functions
	bool handleNodeIndices(Node* curNode, uint8_t curLevel, bool delStorage, sserialize::GeneralizedTrieCreatorConfig& config, Static::TrieNodeCreationInfo& nodeInfo);
	bool getItemIdsForNode(Node* curNode, uint8_t curLevel, const sserialize::GeneralizedTrieCreatorConfig& config, NodeIndexSets & nodeIndexSet);

	void mergeChildrenIndices(uint32_t start, uint32_t end, const std::deque<Node*>& nodes, Node::ItemSetContainer & prefixIndex, Node::ItemSetContainer& suffixIndex, uint64_t& prefixComparisonCount, uint64_t& suffixComparisonCount);
	void mergeChildrenIndices(Node * curNode, NodeIndexSets & idxSet, uint64_t & prefixComparisonCount, uint64_t & suffixComparisonCount);

	template<class StaticTrieNodeT>
	void serializeTrieBottomUp(GeneralizedTrieCreatorConfig & config);
	
private: //flat trie creation functions

	uint32_t createFlatTrieEntry(FlatTrieEntryConfig<ItemIdType> & flatTrieConfig);
	void fillFlatTrieIndexEntries(FlatTrieEntryConfig< ItemIdType >& flatTrieConfig, const sserialize::FlatGSTConfig& config);
	void fillFlatTrieIndexEntriesWithStrIds(FlatTrieEntryConfig< ItemIdType >& flatTrieConfig, const sserialize::FlatGSTConfig& config);
	void initFlatGSTStrIdNodes(Node* node, const FlatGSTConfig & config, uint32_t prefixLen, std::set< ItemIdType >* destSet);

private:
	void compactify(Node * node);
	/** Fixes Problems in the trie before serialization. Current Problems: Nodes with a string length > 255 */
	void trieSerializationProblemFixer(Node * node);

private:
	bool checkTrieEqualityRecurse(Node * curNode, Static::TrieNode curStaticNode, std::string firstChar);
	bool checkIndexEqualityRecurse(Node* curNode, sserialize::Static::TrieNode curStaticNode, sserialize::Static::GeneralizedTrie& staticTrie, StringCompleter::SupportedQuerries sqtype);
	bool checkFlatTrieEquality(Node* node, std::string prefix, uint32_t& posInFTrie, const sserialize::Static::FlatGST& trie, bool checkIndex);
	bool consistencyCheckRecurse(Node * node) const;


public: //Initalization and Destruction
	GeneralizedTrie();
	GeneralizedTrie(const StringsItemDBWrapper<ItemType> & db, bool caseSensitive=false, bool suffixTrie=false);
	~GeneralizedTrie();
	void clear();
	inline void setCaseSensitivity(bool c) { m_caseSensitive = c; }
	inline void setSuffixTrie(bool c) { m_isSuffixTrie = c; }
	inline void setSuffixDelimeters(const std::unordered_set<uint32_t> & s) { m_suffixDelimeters = s; }
	
	/** If you set this to true, then transliterated versions of itemstrings are added as well
	  * This is only compatible with the Static::Trie. The FlatGST does not support this!
	  *
	  */
	inline void setAddTransliteratedDiacritics(bool c) { m_addTransDiacs = c; }
	
	inline bool isCaseSensitive() { return m_caseSensitive; }
	inline bool isSuffixTrie() { return m_isSuffixTrie;}
	inline bool getSuffixDelimeters() { return m_suffixDelimeters; }

public: //Fill functions
	Node* at(const std::string::const_iterator & strBegin, const std::string::const_iterator & strEnd);
	Node* operator[](const std::string & str);
	/** This will set the DB from which the trie is created
	  * @param db The DB to create the trie from
	  * @param suffixDelimeters this is only useful if suffixTrie is set. This will change the way suffixstrings are created.
	  *                         If set, then the item strings will be scanned from left to right to find a code point in suffixDelimeters
	  *                         If it finds one, then this will be the next suffix string (to the end if the given string)
	  *
	  *
	  *
	  */
	bool setDB(const StringsItemDBWrapper< ItemType >& db);

public://Completion
	ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qt = sserialize::StringCompleter::QT_SUFFIX_PREFIX) const;
	ItemIndex complete(const std::deque< std::string > & strs, sserialize::StringCompleter::QuerryType qt = sserialize::StringCompleter::QT_SUFFIX_PREFIX) const;

public: //Access functions
	StringsItemDBWrapper<ItemType> & db() { return m_db;}
	Node * rootNode() { return m_root;}

public: //Serialization functions
	uint32_t getDepth() { return m_root->depth();}

	void createStaticTrie(sserialize::GeneralizedTrieCreatorConfig & config);
	void createStaticFlatTrie(FlatGSTConfig & config);

public: //Check functions
	bool consistencyCheck() const;
	bool checkTrieEquality(sserialize::GeneralizedTrieCreatorConfig config, sserialize::Static::GeneralizedTrie staticTrie);
	bool checkIndexEquality(GeneralizedTrieCreatorConfig config, Static::GeneralizedTrie staticTrie, StringCompleter::SupportedQuerries sqtype);
	bool checkFlatTrieEquality(const sserialize::Static::FlatGST& trie, bool checkIndex=true);

};


//------------------------Creation/Deletion functions-------------------------------->

template<class ItemType>
GeneralizedTrie<ItemType>::GeneralizedTrie() :
m_count(0),
m_nodeCount(0),
m_root(0),
m_db(StringsItemDBWrapper<ItemType>(  new StringsItemDBWrapperPrivateSIDB<ItemType>() )),
m_caseSensitive(false),
m_addTransDiacs(false),
m_isSuffixTrie(false)
{
}

template<class ItemType>
GeneralizedTrie<ItemType>::GeneralizedTrie(const StringsItemDBWrapper<ItemType> & db, bool caseSensitive, bool suffixTrie) :
m_count(0),
m_nodeCount(0),
m_root(0),
m_db(StringsItemDBWrapper<ItemType>( new StringsItemDBWrapperPrivateSIDB<ItemType>() )),
m_caseSensitive(caseSensitive),
m_addTransDiacs(false),
m_isSuffixTrie(suffixTrie)
{
	setDB(db);
}

template<class ItemType>
GeneralizedTrie<ItemType>::~GeneralizedTrie() {
	clear();
}

template<class ItemType>
void
GeneralizedTrie<ItemType>::clear() {
	delete m_root;
	m_root = 0;

	m_count = 0;
	m_nodeCount = 0;
}


//------------------------Insertion functions-------------------------------->

template<class ItemType>
std::deque<std::string>
GeneralizedTrie<ItemType>::
createInsertStrings(const std::deque<std::string> & sourceStrs, std::set<std::string> & fullStrs, std::set<std::string> & suffixStrings) {
	std::deque<std::string> itemStrs;
	for(std::deque<std::string>::const_iterator it = sourceStrs.begin(); it != sourceStrs.end(); ++it) {
		if(isValidString(*it)) {
			itemStrs.push_back(*it);
			if (m_caseSensitive) {
				fullStrs.insert(*it);
			}
			else {
				fullStrs.insert(unicode_to_lower(*it));
			}
		}
	}
	if (m_isSuffixTrie) {
		for(std::set<std::string>::iterator it = fullStrs.begin(); it != fullStrs.end(); ++it) {
			std::string str = *it;
			while (str.length() > 0) {
				suffixStrings.insert(str);
				std::string::iterator nextChar = str.begin();
				utf8::next(nextChar, str.end());
				str.erase(str.begin(), nextChar);
			}
		}
	}
	return itemStrs;
}

template<class ItemType>
typename GeneralizedTrie<ItemType>::Node *
GeneralizedTrie<ItemType>::operator[](const std::string & str) {
	return at(str.begin(), str.end());
}

template<class ItemType>
typename GeneralizedTrie<ItemType>::Node *
GeneralizedTrie<ItemType>::at(const std::string::const_iterator & strBegin, const std::string::const_iterator & strEnd) {
	std::string::const_iterator strIt = strBegin;
	uint32_t strItUCode;
	
	if (!m_root) {
		m_root = new Node();
		m_nodeCount++;
	}
	Node * current = m_root;
	Node * newNode = 0;

	strIt = strBegin;
	while(strIt != strEnd) {
		//Find the first different character
		std::string::iterator cIt = current->c.begin();
		uint32_t cItUCode;
		strItUCode = ((strBegin != strEnd) ? utf8::peek_next(strIt, strEnd) : 0);
		cItUCode = ((current->c.size() > 0) ? utf8::peek_next(cIt, current->c.end()) : 0);
		while (cIt != current->c.end() && strIt != strEnd) {
			strItUCode = utf8::peek_next(strIt, strEnd);
			cItUCode = utf8::peek_next(cIt, current->c.end());
			if (strItUCode == cItUCode) {
				utf8::next(strIt, strEnd);
				utf8::next(cIt, current->c.end());
			}
			else {
				break;
			}
		}
		//cIt=end or strIt=end or strIt != cIt
		if (cIt == current->c.end() && strIt != strEnd) { //node->c is real prefix, strIt points to new element
			strItUCode = utf8::peek_next(strIt, strEnd);
			if (current->children.count(strItUCode) > 0) {
				current = current->children.at(strItUCode);
			}
			else {
				newNode = new Node;
				m_nodeCount++;
				newNode->c = "";
				newNode->c.append(strIt, strEnd);
				(current->children)[strItUCode] = newNode;
				newNode->parent = current;
				current = newNode;
				break;
			}
		}
		else if (cIt == current->c.end() && strIt == strEnd) { //node->c is equal to str
			break;
		}
		else if (cIt != current->c.end() && strIt == strEnd) { //str is prefix of node->c
			cItUCode = utf8::peek_next(cIt, current->c.end());

			Node * oldStrNode = new Node;
			m_nodeCount++;
			oldStrNode->c = "";
			oldStrNode->c.append(cIt, current->c.end());
			oldStrNode->children.swap(current->children);
			oldStrNode->exactValues.swap(current->exactValues);
			oldStrNode->subStrValues.swap(current->subStrValues);

			oldStrNode->fixChildParentPtrRelation();
			
			//save char for children
			uint32_t c = cItUCode;
			
			//clear data, adjust to new c
			current->exactValues = Node::ItemSetContainer();
			current->subStrValues = Node::ItemSetContainer();
			current->children.clear();
			current->c.erase(cIt, current->c.end());
			
			//insert old node and add value
			(current->children)[c] = oldStrNode;
			oldStrNode->parent = current;
			break;
		}
		else if (cIt != current->c.end() && strIt != strEnd) { //possible common prefix, iterator at the different char

			strItUCode = utf8::peek_next(strIt, strEnd);
			cItUCode = utf8::peek_next(cIt, current->c.end());

			Node * newNode = new Node;
			m_nodeCount++;
			newNode->c = "";
			newNode->c.append(strIt, strEnd);
			
			Node * oldStrNode = new Node;
			m_nodeCount++;
			oldStrNode->c = "";
			oldStrNode->c.append(cIt, current->c.end());
			oldStrNode->children.swap(current->children);
			oldStrNode->exactValues.swap(current->exactValues);
			oldStrNode->subStrValues.swap(current->subStrValues);
			oldStrNode->fixChildParentPtrRelation();
			
			current->children.clear();
			current->exactValues = Node::ItemSetContainer();
			current->subStrValues = Node::ItemSetContainer();
			current->c.erase(cIt, current->c.end());

			//add pointer to node with the rest of the old node string
			(current->children)[strItUCode] = newNode;
			newNode->parent = current;
			//add pointer the node with the rest of the new string
			(current->children)[cItUCode] = oldStrNode;
			oldStrNode->parent = current;
			current = newNode;
			break;
		}
	} // end while
	return current;
};

///Inserts the string between strIt and strEnd. If suffixDelimeters is given, the string split between those code points
template<class ItemType>
typename GeneralizedTrie<ItemType>::Node * GeneralizedTrie<ItemType>::nextSuffixNode(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd) {
	Node * node = at(strIt, strEnd);
	nextSuffixString(strIt, strEnd);
	return node;
}

///Inserts the string between strIt and strEnd. If suffixDelimeters is given, the string split between those code points
template<class ItemType>
void GeneralizedTrie<ItemType>::nextSuffixString(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd) {
	if (m_suffixDelimeters.size()) {
		while (strIt != strEnd) {
			if (m_suffixDelimeters.count( utf8::next(strIt, strEnd) ) > 0)
				break;
		}
	}
	else {
		utf8::next(strIt, strEnd);
	}
}


/** This function sets the DB the trie uses and creates the trie out of the DB data
  * It does so in 3 Steps:
  * 1. Create trie nodes with all the strings in the db's string table
  * 2. Gather for every string the end node (multiple end nodes for suffix)
  * 3. Add the item ids to the nodes by iterating over them, the string ids will tell where to put the ids
  *
  **/
template<class ItemType>
bool GeneralizedTrie<ItemType>::setDB(const StringsItemDBWrapper< ItemType >& db) {
	clear();
	ProgressInfo progressInfo;
	
	DiacriticRemover transLiterator;
	if (m_addTransDiacs) {
		UErrorCode status = transLiterator.init();
		if (U_FAILURE(status)) {
			std::cerr << "Failed to create translitorated on request: " << u_errorName(status) << std::endl;
			return false;
		}
	}
		
	m_db = db;
	
	//This is the first part (create the trie)
	progressInfo.begin(db.strIdToStr().size());
	uint32_t count = 0;
	for(std::map<unsigned int, std::string>::const_iterator strsIt = db.strIdToStr().begin(); strsIt != db.strIdToStr().end(); ++strsIt) {
		std::vector<std::string> strs;
		if (m_caseSensitive)
			strs.push_back(strsIt->second);
		else
			strs.push_back( unicode_to_lower(strsIt->second) );
		
		if (m_addTransDiacs) {
			strs.push_back( strs.back() );
			transLiterator.transliterate(strs.back());
		}

		for(std::vector<std::string>::const_iterator it = strs.begin(); it != strs.end(); ++it) {
			at(it->begin(), it->end());
			if (m_isSuffixTrie) {
				std::string::const_iterator strIt = it->begin();
				std::string::const_iterator strEnd = it->end();
				while (strIt != strEnd) {
					nextSuffixNode(strIt, strEnd);
				}
			}
		}
		progressInfo(++count, "GeneralizedTrie::setDB::createTrie");
	}
	if (!consistencyCheck()) {
		std::cout << "Trie is broken after GeneralizedTrie::setDB::createTrie" << std::endl;
		return false;
	}

	//get all nodes for all strings
	std::unordered_map<unsigned int, std::set<Node*> > strIdToSubStrNodes; strIdToSubStrNodes.reserve(db.strIdToStr().size());
	std::unordered_map<unsigned int, std::set<Node*> > strIdToExactNodes; strIdToExactNodes.reserve(db.strIdToStr().size());
	
	std::vector<unsigned int> strIds;
	insertMapKeysIntoVector(db.strIdToStr(), strIds);
	
	progressInfo.begin(db.strIdToStr().size());
	count = 0;
	#pragma omp parallel for 
	for(size_t i = 0; i <  strIds.size(); ++i) { //we need to do it like that due to the parallelisation (map::iterator does not work here)
		std::map<unsigned int, std::string>::const_iterator dbStrIt = db.strIdToStr().find(strIds[i]);
		std::vector<std::string> strs;
		if (m_caseSensitive)
			strs.push_back(dbStrIt->second);
		else
			strs.push_back( unicode_to_lower(dbStrIt->second) );
		
		if (m_addTransDiacs) {
			strs.push_back( strs.back() );
			transLiterator.transliterate(strs.back());
		}

		
		for(std::vector<std::string>::const_iterator insStrIt = strs.begin(); insStrIt != strs.end(); ++insStrIt) {
			std::string::const_iterator strIt(insStrIt->begin());
			std::string::const_iterator strEnd(insStrIt->end());
			#pragma omp critical
			{strIdToExactNodes[dbStrIt->first].insert( at(strIt, strEnd) );}
			if (m_isSuffixTrie) {
				while (strIt != strEnd) {
					Node * node = nextSuffixNode(strIt, strEnd);
					#pragma omp critical
					{strIdToSubStrNodes[dbStrIt->first].insert( node );}
				}
			}
		}
		#pragma omp atomic
		count++;
		progressInfo(count, "GeneralizedTrie::setDB::findStringNodes");
	}
	progressInfo.end("GeneralizedTrie::setDB::findStringNodes");

	if (!consistencyCheck()) {
		std::cout << "Trie is broken after GeneralizedTrie::setDB::findStringNodes" << std::endl;
		return false;
	}

// 	assert( m_root->parent() == 0 );
	//Now add the items
	
	progressInfo.begin(db.size());
	count = 0;
	for(size_t i = 0; i < db.size(); i++) {
		std::vector<uint32_t> itemStrs( db.itemStringIDs(i) );
		std::set<Node*> exactNodes;
		std::set<Node*> suffixNodes;
		for(std::vector<uint32_t>::const_iterator itemStrsIt = itemStrs.begin(); itemStrsIt != itemStrs.end(); ++itemStrsIt) {
			if (strIdToExactNodes.count(*itemStrsIt)) {
				exactNodes.insert(strIdToExactNodes[*itemStrsIt].begin(), strIdToExactNodes[*itemStrsIt].end());
			}
			else {
				std::cout << "ERROR: No exact node for item string" << std::endl;
			}
			if (strIdToSubStrNodes.count(*itemStrsIt)) {
				suffixNodes.insert(strIdToSubStrNodes[*itemStrsIt].begin(), strIdToSubStrNodes[*itemStrsIt].end());
			}
		}
		
		for(std::set<Node*>::iterator esit = exactNodes.begin(); esit != exactNodes.end(); ++esit) {
			(*esit)->exactValues.insert((*esit)->exactValues.end(), i);
		}
		for(std::set<Node*>::iterator it = suffixNodes.begin(); it != suffixNodes.end(); ++it) {
			(*it)->subStrValues.insert((*it)->subStrValues.end(), i);
		}
		
		++count;
		progressInfo(count, "GeneralizedTrie::setDB::insertItems");
	}
	progressInfo.end("GeneralizedTrie::setDB::insertItems");
	std::cout << std::endl;
	if (!consistencyCheck()) {
		std::cout << "Trie is broken after GeneralizedTrie::setDB::insertItems" << std::endl;
		return false;
	}

	return true;
}


//------------------------Completion functions-------------------------------->


template<class ItemType>
void
GeneralizedTrie<ItemType>::
completionRecurse(Node * node, std::string::const_iterator strBegin, const std::string::const_iterator & strEnd, sserialize::StringCompleter::QuerryType qt, std::set<ItemIdType> & destination) const {
	//Find StartNode
	
	if (!node)
		return;

	Node * current = node;

	std::string::const_iterator strIt = strBegin;
	uint32_t strItUCode = ((strIt != strEnd) ? utf8::peek_next(strIt, strEnd) : 0);
	std::string prefix;
	std::string::iterator cIt = current->c.begin();
	uint32_t cItUCode = ((current->c.size() > 0) ? utf8::peek_next(cIt, current->c.end()) : 0);
	while (cIt != current->c.end() && strIt != strEnd) {
		strItUCode = utf8::peek_next(strIt, strEnd);
		cItUCode = utf8::peek_next(cIt, current->c.end());
		if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
			if (unicode32_to_lower(strItUCode) == unicode32_to_lower(cItUCode) or
				unicode32_to_upper(strItUCode) == unicode32_to_upper(cItUCode) ) {
				utf8::next(cIt, current->c.end());
				utf8::next(strIt, strEnd);
			}
			else { // no common prefix found
				return;
			}
		}
		else { //case sensitive
			if (strItUCode == cItUCode) {
				utf8::next(cIt, current->c.end());
				utf8::next(strIt, strEnd);
			}
			else {
				return;
			}
		}
	}
	//cIt=end or strIt=end or strIt != cIt
	if (cIt == current->c.end() && strIt != strEnd) { //node->c is real prefix, strIt points to new element
		strItUCode = utf8::peek_next(strIt, strEnd);
		if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
			uint32_t lowerChar = unicode32_to_lower(strItUCode);
			uint32_t upperChar = unicode32_to_upper(strItUCode);
			if (current->children.count(upperChar) > 0) {
				completionRecurse(current->children.at(upperChar), strIt, strEnd, qt, destination);
			}
			if (lowerChar != upperChar && current->children.count(lowerChar) > 0) {
				completionRecurse(current->children.at(lowerChar), strIt, strEnd, qt, destination);
			}
		}
		else {
			if (current->children.count(strItUCode) > 0) {
				completionRecurse(current->children.at(strItUCode), strIt, strEnd, qt, destination);
			}
		}
	}
	else if (current) { //cIt != end but strIt=end, otherwise node->c != str
		if (cIt == current->c.end()) {
			if (qt & sserialize::StringCompleter::QT_EXACT) {
				if (current->exactValues.size())
					destination.insert(current->exactValues.begin(), current->exactValues.end());
			}
			if (qt & sserialize::StringCompleter::QT_SUFFIX) {
				if (current->exactValues.size())
					destination.insert(current->exactValues.begin(), current->exactValues.end());
				if (current->subStrValues.size())
					destination.insert(current->subStrValues.begin(), current->subStrValues.end());
			}
		}
		if (qt & sserialize::StringCompleter::QT_PREFIX) {
			current->insertExactValuesRecursive(destination);
		}
		if (qt & sserialize::StringCompleter::QT_SUFFIX_PREFIX) {
			current->insertAllValuesRecursive(destination);
		}
	}
}

template<class ItemType>
ItemIndex
GeneralizedTrie<ItemType>::complete(const std::string& str, sserialize::StringCompleter::QuerryType qt) const {
	if (!m_caseSensitive) {
		qt = (sserialize::StringCompleter::QuerryType) (qt | sserialize::StringCompleter::QT_CASE_INSENSITIVE);
	}

	std::set<uint32_t> set;
	completionRecurse(m_root, str.begin(), str.end(), qt, set);
	std::vector<uint32_t> ds(set.begin(), set.end());
	return ItemIndex(ds);
}

template<class ItemType>
ItemIndex
GeneralizedTrie<ItemType>::complete(const std::deque< std::string >& strs, sserialize::StringCompleter::QuerryType qt) const {
	std::vector< ItemIndex > sets;
	for(size_t i = 0; i < strs.size(); i++) {
		sets.push_back(complete(strs[i], qt));
	}
	return ItemIndex::intersect(sets);
}

template<class ItemType>
void
GeneralizedTrie<ItemType>::createStaticTrie(sserialize::GeneralizedTrieCreatorConfig& config) {

	std::cout << "GeneralizedTrie<ItemType>::createStaticTrie: consistencyCheck...";
	if (consistencyCheck()) {
		std::cout << "OK" << std::endl;
	}
	else {
		std::cout << "Failed" << std::endl;
	}

	if (m_root) {
		std::cout << "Compactifying...";
		compactify(m_root);
		trieSerializationProblemFixer(m_root);
		std::cout << "done" << std::endl;
		if (!consistencyCheck()) {
			std::cout << "Compactifying broke trie!" << std::endl;
			return;
		}
	}

	if (config.nodeType == Static::TrieNode::T_SIMPLE) {
		serializeTrieBottomUp<Static::SimpleStaticTrieCreationNode>(config);
	}
	else if (config.nodeType == Static::TrieNode::T_COMPACT) {
		serializeTrieBottomUp<Static::CompactStaticTrieCreationNode>(config);
	}

	//Add the stats
	uint8_t trieType = 0;
	trieType |= Static::GeneralizedTrie::STO_NORMALIZED;
	if (m_caseSensitive)
		trieType |= Static::GeneralizedTrie::STO_CASE_SENSITIVE;
	if (m_isSuffixTrie)
		trieType |= Static::GeneralizedTrie::STO_SUFFIX;
		
	uint8_t nodeType = config.nodeType;
	uint16_t longestString = 0;
	uint16_t depth = getDepth();
	std::deque<uint8_t> header;
	uint8_t strCompleterType = sserialize::Static::StringCompleter::T_TRIE;
	
	Static::StringCompleter::addHeader(strCompleterType, header);
	
	Static::GeneralizedTrie::addHeader(trieType, nodeType, longestString, depth, header);
	prependToDeque(header, *(config.trieList));

	if (config.deleteRootTrie) {
		delete m_root;
		m_root = 0;
		m_count = 0;
		m_nodeCount = 0;
	}
}

template <class ItemType>
void GeneralizedTrie<ItemType>::createStaticFlatTrie(FlatGSTConfig & config) {
	if (m_addTransDiacs) {
		std::cout << "No support for transliterated strings!" << std::endl;
		return;
	}

	if (m_root)
		compactify(m_root);

	std::set<std::string> trieStrings;
	
	if (m_caseSensitive)
		insertMapValuesIntoSet<unsigned int, std::string>(m_db.strIdToStr(), trieStrings);
	else {
		std::map<unsigned int, std::string>::const_iterator itEnd = m_db.strIdToStr().end();
		for(std::map<unsigned int, std::string>::const_iterator it = m_db.strIdToStr().begin(); it != itEnd; ++it) {
			trieStrings.insert( unicode_to_lower(it->second) );
		}
	}
	
	FlatTrieEntryConfig<ItemIdType> flatTrieConfig(config.indexFactory);
	
	
	uint32_t nodeHitCount = 0;
	ProgressInfo progressInfo;
	progressInfo.begin(m_nodeCount);
	for(std::set<std::string>::const_iterator it = trieStrings.begin(); it != trieStrings.end() && nodeHitCount < m_nodeCount; it++) {
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
		progressInfo(nodeHitCount, "GeneralizedTrie::createStaticFlatTrie::createTrieArray");
	}
	progressInfo.end("GeneralizedTrie::createStaticFlatTrie::createTrieArray");
	trieStrings.clear();
	
	uint8_t sq = StringCompleter::SQ_EP;
	if (m_caseSensitive)
		sq |= StringCompleter::SQ_CASE_SENSITIVE;
	else
		sq |= StringCompleter::SQ_CASE_INSENSITIVE;
		
	if (m_isSuffixTrie)
		sq |= StringCompleter::SQ_SSP;
	
	std::deque<uint8_t> header;
	if (config.withStringIds)
		Static::StringCompleter::addHeader(Static::StringCompleter::T_FLAT_TRIE_STRIDS, header);
	else
		Static::StringCompleter::addHeader(Static::StringCompleter::T_FLAT_TRIE, header);
	config.destination << header[0];
	config.destination << header[1];
	config.destination << static_cast<uint8_t>(0); //version
	config.destination << sq;
	config.destination << flatTrieConfig.flatTrieStrings; //string table

	flatTrieConfig.flatTrieStrings = std::deque<std::string>(); //clear doesn't always free the memory


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
	
	if (config.withStringIds) {
		fillFlatTrieIndexEntriesWithStrIds(flatTrieConfig, config);
	}
	else {
		fillFlatTrieIndexEntries(flatTrieConfig, config);
	}
	sserialize::Static::DequeCreator<IndexEntry> dc(config.destination);
	FlatGSTIndexEntryMapper indexEntryMapper(dc);
	m_root->mapDepthFirst(indexEntryMapper);
	dc.flush();
	
	if (config.deleteTrie)
		clear();

}

template <class ItemType>
void GeneralizedTrie<ItemType>::
fillFlatTrieIndexEntries(FlatTrieEntryConfig<ItemIdType> & flatTrieConfig, const FlatGSTConfig & config) {
	
	ProgressInfo progressInfo;
	
	std::deque< MultiTrieNode<ItemIdType>* > nodesInLevelOrder;
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
		
		progressInfo(count,"GeneralizedTrie::fillFlatTrieIndexEntries");
	}
	progressInfo.end("GeneralizedTrie::fillFlatTrieIndexEntries");
}


template <class ItemType>
void GeneralizedTrie<ItemType>::
fillFlatTrieIndexEntriesWithStrIds(FlatTrieEntryConfig<ItemIdType> & flatTrieConfig, const FlatGSTConfig & config) {
	std::cout << "GeneralizedTrie::fillFlatTrieIndexEntriesWithStrIds: initFlatGSTStrIdNodes" << std::endl;
	initFlatGSTStrIdNodes(m_root, config, 0, 0);
	std::cout << "GeneralizedTrie::fillFlatTrieIndexEntriesWithStrIds: initFlatGSTStrIdNodes completed!" << std::endl;
	ProgressInfo progressInfo;
	std::map<unsigned int, std::string>::const_iterator itEnd = m_db.strIdToStr().end();
	progressInfo.begin(m_db.strIdToStr().size());
	uint32_t count = 0;
	for(std::map<unsigned int, std::string>::const_iterator it = m_db.strIdToStr().begin(); it != itEnd; ++it) {
		std::string str;
		if (m_caseSensitive)
			str = it->second;
		else
			str =  unicode_to_lower(it->second);
			
		
		{
			Node * curNode = m_root;
			std::string::const_iterator strIt = str.begin();
			std::string::const_iterator strEnd = str.end();
			while(strIt != strEnd) {
				strIt += curNode->c.size();
				if (strIt != strEnd)
					curNode = curNode->children.at( utf8::peek_next(strIt, strEnd) );
			}
			FlatGSTStrIds_TPNS * curNodeTPNS = dynamic_cast<FlatGSTStrIds_TPNS*>(curNode->temporalPrivateStorage);
			
			curNodeTPNS->exactIndex.push_back(it->first);
		
			if (m_isSuffixTrie) {
				for(std::string::const_iterator suffixIt = str.begin(); suffixIt != strEnd; nextSuffixString(suffixIt, strEnd)) {
					curNode = m_root;
					strIt = suffixIt;
					while(strIt != strEnd) {
						strIt += curNode->c.size();
						if (strIt != strEnd)
							curNode = curNode->children.at( utf8::peek_next(strIt, strEnd) );
					}
					FlatGSTStrIds_TPNS * curNodeTPNS = dynamic_cast<FlatGSTStrIds_TPNS*>(curNode->temporalPrivateStorage);
					if (curNodeTPNS->suffixIndex.size() == 0 || curNodeTPNS->suffixIndex.back() != it->first)
						curNodeTPNS->suffixIndex.push_back(it->first);
				}
			}
		}
		count++;
		progressInfo(count, "GeneralizedTrie::fillFlatTrieIndexEntriesWithStrIds::StrIdsIntoNodes");
	}
	progressInfo.end("GeneralizedTrie::fillFlatTrieIndexEntriesWithStrIds::StrIdsIntoNodes");
	
	//Let's serialize our set in bottom-up fashion
	std::deque<Node*> trieNodes;
	m_root->addNodesSorted(trieNodes);
	progressInfo.begin(trieNodes.size());
	count = 0;
	while(trieNodes.size() > 0) {
		Node * curNode = trieNodes.back();
		FlatGSTStrIds_TPNS * curNodeTPNS = dynamic_cast<FlatGSTStrIds_TPNS*>( curNode->temporalPrivateStorage );
		FlatGSTStrIds_TPNS * parentNodeTPNS = (curNode->parent ? dynamic_cast<FlatGSTStrIds_TPNS*>(curNode->parent->temporalPrivateStorage) : 0);
		
		IndexEntry fe;
		fe.itemIdIndex = curNodeTPNS->itemIdIndex;
		fe.minId = curNodeTPNS->minId;
		fe.maxId = curNodeTPNS->maxId;
		fe.mergeIndex = config.mergeIndex;

		if (fe.itemIdIndex)
			fe.exactValues = flatTrieConfig.indexFactory.addIndex( curNode->exactValues );
		else {
			fe.exactValues = flatTrieConfig.indexFactory.addIndex(curNodeTPNS->exactIndex);
		}
		
		if (m_isSuffixTrie) {
			if (fe.itemIdIndex) {
				std::vector<ItemIdType> s2;
				if (config.mergeIndex)
					diffSortedContainer(s2, curNode->subStrValues, curNode->exactValues);
				else {
					mergeSortedContainer(s2, s2, curNode->subStrValues);
					mergeSortedContainer(s2, s2, curNode->exactValues);
				}
				fe.suffixValues = flatTrieConfig.indexFactory.addIndex( s2 );
			}
			else {
				std::vector<ItemIdType> s;
				if (config.mergeIndex)
					diffSortedContainer(s, curNodeTPNS->suffixIndex, curNodeTPNS->exactIndex);
				else {
					mergeSortedContainer(s, s, curNodeTPNS->suffixIndex);
					mergeSortedContainer(s, s, curNodeTPNS->exactIndex);
				}
				fe.suffixValues = flatTrieConfig.indexFactory.addIndex(s);
			}
		}
		
		//put in the prefix/suffix values. we can reuse the sets here and delete children-sets afterwards
		if (curNode->children.size() > 0) {
			if (fe.itemIdIndex) {
				std::set<ItemIdType> pS, sS;
				curNode->insertExactValuesRecursive(pS);
				curNode->insertAllValuesRecursive(sS);
				if (config.mergeIndex) {
					inplaceDiffSortedContainer(sS, pS);
					inplaceDiffSortedContainer(sS, curNode->subStrValues);
					inplaceDiffSortedContainer(pS, curNode->exactValues);
				}
				fe.prefixValues = flatTrieConfig.indexFactory.addIndex(pS);
				fe.suffixPrefixValues = flatTrieConfig.indexFactory.addIndex(sS);
			}
			else {
				if (config.mergeIndex) {
					inplaceDiffSortedContainer(curNodeTPNS->prefixIndex, curNodeTPNS->exactIndex);
					inplaceDiffSortedContainer(curNodeTPNS->suffixPrefixIndex, curNodeTPNS->exactIndex);
					inplaceDiffSortedContainer(curNodeTPNS->suffixPrefixIndex, curNodeTPNS->suffixIndex);
					inplaceDiffSortedContainer(curNodeTPNS->suffixPrefixIndex, curNodeTPNS->prefixIndex);
				}
				else {
					curNodeTPNS->prefixIndexInsert(curNodeTPNS->exactIndex);
					curNodeTPNS->suffixPrefixIndexInsert(curNodeTPNS->exactIndex);
					curNodeTPNS->suffixPrefixIndexInsert(curNodeTPNS->suffixIndex);
					
				}
				fe.prefixValues = flatTrieConfig.indexFactory.addIndex(curNodeTPNS->prefixIndex);
				fe.suffixPrefixValues = flatTrieConfig.indexFactory.addIndex(curNodeTPNS->suffixPrefixIndex);
			}
			if (parentNodeTPNS) {
				parentNodeTPNS->prefixIndexInsert(curNodeTPNS->exactIndex);
				parentNodeTPNS->prefixIndexInsert(curNodeTPNS->prefixIndex);
				if (m_isSuffixTrie) {
					parentNodeTPNS->suffixPrefixIndexInsert(curNodeTPNS->exactIndex);
					parentNodeTPNS->suffixPrefixIndexInsert(curNodeTPNS->suffixIndex);
					parentNodeTPNS->suffixPrefixIndexInsert(curNodeTPNS->suffixPrefixIndex);
				}
			}
		}
		else {
			if (config.mergeIndex) {
				fe.prefixValues = flatTrieConfig.indexFactory.addIndex(Node::ItemSetContainer());
				fe.suffixPrefixValues = flatTrieConfig.indexFactory.addIndex(Node::ItemSetContainer());
			}
			else {
				fe.prefixValues = fe.exactValues;
				fe.suffixPrefixValues = fe.suffixValues;
			}
			
			if (parentNodeTPNS) {
				parentNodeTPNS->prefixIndexInsert(curNodeTPNS->prefixIndex);
				parentNodeTPNS->prefixIndexInsert(curNodeTPNS->exactIndex);
				if (m_isSuffixTrie) {
					parentNodeTPNS->suffixPrefixIndexInsert(curNodeTPNS->exactIndex);
					parentNodeTPNS->suffixPrefixIndexInsert(curNodeTPNS->suffixIndex);
				}
			}
		}
		
		curNode->deleteTemporalPrivateStorage();
		curNode->temporalPrivateStorage = new FlatGSTIndexEntry_TPNS(fe);
		trieNodes.pop_back();
		if (config.deleteTrie && !fe.itemIdIndex) {//TODO:Make this working.
			curNode->exactValues = Node::ItemSetContainer();
			curNode->subStrValues = Node::ItemSetContainer();
			curNode->c = std::string();
		}
		progressInfo(++count, "GeneralizedTrie::fillFlatTrieIndexEntriesWithStrIds::createIndexSet");
	}
	progressInfo.end("GeneralizedTrie::fillFlatTrieIndexEntriesWithStrIds::createIndexSet");
}

template <class ItemType>
void GeneralizedTrie<ItemType>::initFlatGSTStrIdNodes(Node* node, const sserialize::FlatGSTConfig& config, uint32_t prefixLen, std::set< ItemIdType >* destSet) {
	if (node) {
		if (node->c.size())
			prefixLen += utf8CharCount(node->c.begin(), node->c.end());
		FlatGSTStrIds_TPNS * nodeTPNS = new FlatGSTStrIds_TPNS();
		node->temporalPrivateStorage = nodeTPNS;
		if (node->exactValues.size()) {
			nodeTPNS->minId = std::min<ItemIdType>(GST_ITEMID_MAX, *(node->exactValues.begin()));
			nodeTPNS->maxId = std::max<ItemIdType>(0, *(node->exactValues.rbegin()));
		}
		if (node->subStrValues.size()) {
			nodeTPNS->minId = std::min<ItemIdType>(GST_ITEMID_MAX, *(node->subStrValues.begin()));
			nodeTPNS->maxId = std::max<ItemIdType>(0, *(node->subStrValues.rbegin()));
		}
		nodeTPNS->itemIdIndex = node->exactValues.size() < config.maxSizeForItemIdIndex && node->subStrValues.size() < config.maxSizeForItemIdIndex  && prefixLen >= config.minStrLenForItemIdIndex;
		std::set<ItemIdType> mySet;
		if (nodeTPNS->itemIdIndex) {
			mySet.insert(node->exactValues.begin(), node->exactValues.end());
			mySet.insert(node->subStrValues.begin(), node->subStrValues.end());
			nodeTPNS->itemIdIndex = mySet.size() < config.maxSizeForItemIdIndex && prefixLen >= config.minStrLenForItemIdIndex;
		}
		for(Node::ChildNodeIterator it = node->children.begin(); it != node->children.end(); ++it) {
			if (nodeTPNS->itemIdIndex) {
				initFlatGSTStrIdNodes(it->second, config, prefixLen, &mySet);
			}
			else {
				initFlatGSTStrIdNodes(it->second, config, prefixLen, &mySet);
			}
			FlatGSTStrIds_TPNS * childTPNS = dynamic_cast<FlatGSTStrIds_TPNS*>(it->second->temporalPrivateStorage);
			if (nodeTPNS->itemIdIndex)
				nodeTPNS->itemIdIndex =  nodeTPNS->itemIdIndex && childTPNS->itemIdIndex && mySet.size() < config.maxSizeForItemIdIndex  && prefixLen >= config.minStrLenForItemIdIndex;
			nodeTPNS->minId = std::min<ItemIdType>(childTPNS->minId, nodeTPNS->minId);
			nodeTPNS->maxId = std::max<ItemIdType>(childTPNS->maxId, nodeTPNS->maxId);
		}
		
		if (destSet && nodeTPNS->itemIdIndex) {
			destSet->insert(mySet.begin(), mySet.end());
		}
		
	}
}


/** @param flatTrieConfig.strIt: the string has to be in the trie. correctnes is not checked! */
template <class ItemType>
uint32_t GeneralizedTrie<ItemType>::
createFlatTrieEntry(FlatTrieEntryConfig<ItemIdType> & flatTrieConfig) {
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

//private functions

template <class ItemType>
void GeneralizedTrie<ItemType>::compactify(Node* node) {
	if (node) {
		inplaceDiffSortedContainer(node->subStrValues, node->exactValues);
		compactifyStlContainer(node->subStrValues);
		compactifyStlContainer(node->exactValues);
		for(Node::ChildNodeIterator i = node->children.begin(); i != node->children.end(); i++) {
			compactify(i->second);
		}
	}
}

template <class ItemType>
void GeneralizedTrie<ItemType>::trieSerializationProblemFixer(Node* node) {
	if (node) {
		if (node->c.size() > 0xFF) {
			std::string oldStr;
			oldStr.swap(node->c);
			Node * newNode = new Node();
			newNode->parent = node;
			newNode->children.swap(node->children);
			newNode->exactValues.swap(node->exactValues);
			newNode->subStrValues.swap(node->subStrValues);
			newNode->fixChildParentPtrRelation();
			
			std::string::iterator oldStrBegin = oldStr.begin();
			std::string::iterator oldStrIt = oldStr.begin();
			std::string::iterator oldStrEnd = oldStr.end();
			
			uint32_t c = 0;
			while(oldStrIt != oldStrEnd) {
				c = utf8::next(oldStrIt, oldStrEnd);
				if (oldStrIt-oldStrBegin < 0xFF) {
					utf8::append(c, std::back_insert_iterator<std::string>(node->c));
				}
				else {
					break;
				}
			}
			utf8::append(c, std::back_insert_iterator<std::string>(newNode->c));
			newNode->c.insert(newNode->c.end(), oldStrIt, oldStrEnd);
			node->children.clear();
			node->children[c] = newNode;
		}
		
		for(Node::ChildNodeIterator i = node->children.begin(); i != node->children.end(); i++) {
			trieSerializationProblemFixer(i->second);
		}
	}
}

//private static trie creation functions


template<class ItemType>
void
GeneralizedTrie<ItemType>::
mergeChildrenIndices(uint32_t start, uint32_t end, const std::deque< Node* >& nodes, Node::ItemSetContainer & prefixIndex, Node::ItemSetContainer& suffixIndex, uint64_t& prefixComparisonCount, uint64_t& suffixComparisonCount) {
	if (start == end) {
		TemporalPrivateNodeStorage * privateStorage = dynamic_cast<TemporalPrivateNodeStorage*>(nodes.at(start)->temporalPrivateStorage);
		prefixComparisonCount += privateStorage->prefixComparisonCount;
		suffixComparisonCount += privateStorage->suffixComparisonCount;
		prefixIndex.swap(privateStorage->prefixIndex);
		suffixIndex.swap(privateStorage->suffixIndex);
	}
	else {
		uint32_t mid = start + (end-start)/2;

		//do the recursive merge
		Node::ItemSetContainer tmpPIdx1, tmpSIdx1, tmpPIdx2, tmpSIdx2;
		mergeChildrenIndices(start, mid, nodes, tmpPIdx1, tmpSIdx1, prefixComparisonCount, suffixComparisonCount);
		mergeChildrenIndices(mid+1, end, nodes, tmpPIdx2, tmpSIdx2, prefixComparisonCount, suffixComparisonCount);

		mergeSortedContainer(prefixIndex, tmpPIdx1, tmpPIdx2, prefixComparisonCount);
		mergeSortedContainer(suffixIndex, tmpSIdx1, tmpSIdx2, suffixComparisonCount);
	}
}


/** @return returns the amount of comparissons to create this index 
 *  @param result sorted range of all children item ids
 *  
 *  This will delete all full indices in the children nodes as those are not needed anymore
 */
template<class ItemType>
void
GeneralizedTrie<ItemType>::
mergeChildrenIndices(Node * curNode, NodeIndexSets & idxSet, uint64_t & prefixComparisonCount, uint64_t & suffixComparisonCount) {
	std::deque< Node *> childPtrs = curNode->getSortedChildPtrs();
	if (childPtrs.size() > 0) {
		Node::ItemSetContainer pV, spV;
		mergeChildrenIndices(0, childPtrs.size()-1, childPtrs, pV, spV, prefixComparisonCount, suffixComparisonCount);
		mergeSortedContainer(idxSet.prefixValues, idxSet.prefixValues, pV);
		mergeSortedContainer(idxSet.suffixPrefixValues, idxSet.suffixPrefixValues, spV);
	}
	mergeSortedContainer(idxSet.prefixValues, idxSet.prefixValues, curNode->exactValues, prefixComparisonCount);
	mergeSortedContainer(idxSet.suffixPrefixValues, idxSet.suffixPrefixValues, curNode->exactValues, suffixComparisonCount);
	mergeSortedContainer(idxSet.suffixPrefixValues, idxSet.suffixPrefixValues, curNode->subStrValues, suffixComparisonCount);


	//Cache indices
	TemporalPrivateNodeStorage * privateStorage = dynamic_cast<TemporalPrivateNodeStorage*>(curNode->temporalPrivateStorage);
	if (!privateStorage) {
		privateStorage = new TemporalPrivateNodeStorage();
		curNode->temporalPrivateStorage = privateStorage;
	}
	privateStorage->prefixComparisonCount = prefixComparisonCount;
	privateStorage->suffixComparisonCount = suffixComparisonCount;
	privateStorage->prefixIndex = Node::ItemSetContainer(idxSet.prefixValues.begin(), idxSet.prefixValues.end());
	privateStorage->suffixIndex = Node::ItemSetContainer(idxSet.suffixPrefixValues.begin(), idxSet.suffixPrefixValues.end());

	for(size_t i = 0; i < childPtrs.size(); i++) {
		TemporalPrivateNodeStorage * privateStorage = dynamic_cast<TemporalPrivateNodeStorage*>(childPtrs[i]->temporalPrivateStorage);
		if (privateStorage) {
			privateStorage->prefixIndex = Node::ItemSetContainer();
			privateStorage->suffixIndex = Node::ItemSetContainer();
		}
	}
}

/** This function creates all necessary indices. */
template<class ItemType>
bool
GeneralizedTrie<ItemType>::getItemIdsForNode(
    Node* curNode, uint8_t curLevel, const sserialize::GeneralizedTrieCreatorConfig& config, NodeIndexSets & nodeIndexSet) {

	nodeIndexSet.hasExactValues = (curNode->exactValues.size() > 0);
	nodeIndexSet.hasSuffixValues = (m_isSuffixTrie && (curNode->subStrValues.size() > 0 || curNode->exactValues.size() > 0) );
	nodeIndexSet.exactValues = typename NodeIndexSets::StorageType(curNode->exactValues.begin(), curNode->exactValues.end());
	nodeIndexSet.suffixValues = typename NodeIndexSets::StorageType(curNode->subStrValues.begin(), curNode->subStrValues.end());
	if (!config.mergeIndex)
		mergeSortedContainer(nodeIndexSet.suffixValues, nodeIndexSet.suffixValues, curNode->exactValues);
	
	TemporalPrivateNodeStorage * curNodePrivateStorage = dynamic_cast<TemporalPrivateNodeStorage*>(curNode->temporalPrivateStorage);
	if (!curNodePrivateStorage) {
		curNodePrivateStorage = new TemporalPrivateNodeStorage();
		curNode->temporalPrivateStorage = curNodePrivateStorage;
	}
	uint64_t prefixComparisonCount = 0;
	uint64_t suffixComparisonCount = 0;
	mergeChildrenIndices(curNode, nodeIndexSet, prefixComparisonCount, suffixComparisonCount);

	bool clearPrefixIndexSet = (((int64_t)prefixComparisonCount < config.maxPrefixMergeCount) || config.levelsWithoutFullIndex.count(curLevel) > 0);
	if (!clearPrefixIndexSet) { //full prefixIndex
		nodeIndexSet.hasPrefixValues = true;
		curNodePrivateStorage->prefixComparisonCount = 0;
		prefixComparisonCount = 0;
		if (config.mergeIndex)
			inplaceDiffSortedContainer(nodeIndexSet.prefixValues, nodeIndexSet.exactValues);
	}
	int64_t totalSuffixComparisonCount = (int64_t)(suffixComparisonCount + (clearPrefixIndexSet ? prefixComparisonCount : nodeIndexSet.prefixValues.size()));
	if (totalSuffixComparisonCount < config.maxSuffixMergeCount || config.levelsWithoutFullIndex.count(curLevel) > 0) { // no full index
		nodeIndexSet.suffixPrefixValues = typename NodeIndexSets::StorageType();
	}
	else {
		nodeIndexSet.hasSuffixPrefixValues = true;
		curNodePrivateStorage->suffixComparisonCount = 0;
		if (config.mergeIndex) {
			if (!clearPrefixIndexSet)
				inplaceDiffSortedContainer(nodeIndexSet.suffixPrefixValues, nodeIndexSet.exactValues);
			
			inplaceDiffSortedContainer(nodeIndexSet.suffixPrefixValues, nodeIndexSet.prefixValues);
			inplaceDiffSortedContainer(nodeIndexSet.suffixPrefixValues, nodeIndexSet.suffixValues);
		}
	}

	if (clearPrefixIndexSet)
		nodeIndexSet.prefixValues = typename NodeIndexSets::StorageType();

	return true;
}

/** Creates all necessary item indices, put them into the ItemFactory, sets the index pointers in nodeInfo and sets the IndexTypes in nodeInfo */
template<class ItemType>
bool
GeneralizedTrie<ItemType>::handleNodeIndices(
    Node* curNode, uint8_t curLevel, bool delStorage, sserialize::GeneralizedTrieCreatorConfig& config, sserialize::Static::TrieNodeCreationInfo& nodeInfo) {
    
    nodeInfo.mergeIndex = config.mergeIndex;

	NodeIndexSets nodeIdcs;

	getItemIdsForNode(curNode, curLevel, config, nodeIdcs);

	bool ok;
	uint8_t indexTypes = 0;
	if (nodeIdcs.hasExactValues) {
		nodeInfo.exactIndexPtr = config.indexFactory.addIndex(nodeIdcs.exactValues, &ok);
		if (!ok) {
			std::cerr << "FATAL: Failed to add Index to index factory" << std::endl;
			nodeInfo.exactIndexPtr = 0xFFFFFFFF;
		}
		else
			indexTypes |= Static::TrieNodePrivate::IT_EXACT;
	}
	if (nodeIdcs.hasPrefixValues) {
		nodeInfo.prefixIndexPtr = config.indexFactory.addIndex(nodeIdcs.prefixValues, &ok);
		if (!ok) {
			std::cerr << "FATAL: Failed to add Index to index factory" << std::endl;
			nodeInfo.prefixIndexPtr = 0xFFFFFFFF;
		}
		else
			indexTypes |= Static::TrieNodePrivate::IT_PREFIX;
	}
	if (nodeIdcs.hasSuffixValues) {
		nodeInfo.suffixIndexPtr = config.indexFactory.addIndex(nodeIdcs.suffixValues, &ok);
		if (!ok) {
			std::cerr << "FATAL: Failed to add Index to index factory" << std::endl;
			nodeInfo.suffixIndexPtr = 0xFFFFFFFF;
		}
		else
			indexTypes |= Static::TrieNodePrivate::IT_SUFFIX;
	}
	if (nodeIdcs.hasSuffixPrefixValues) {
		nodeInfo.suffixPrefixIndexPtr = config.indexFactory.addIndex(nodeIdcs.suffixPrefixValues, &ok);
		if (!ok) {
			std::cerr << "FATAL: Failed to add Index to index factory" << std::endl;
			nodeInfo.suffixPrefixIndexPtr = 0xFFFFFFFF;
		}
		else
			indexTypes |= Static::TrieNodePrivate::IT_SUFFIX_PREFIX;
	}

	nodeInfo.indexTypes = (Static::TrieNodePrivate::IndexTypes) indexTypes;

	return true;

}



/**
 * This function serializes the trie in bottom-up fashion
 *
 */
template<class ItemType>
template<class StaticTrieNodeT>
void
GeneralizedTrie<ItemType>::
serializeTrieBottomUp(GeneralizedTrieCreatorConfig & config) {
	uint32_t depth = getDepth();
	if (depth == 0) {
		std::cout << "Trie is empty" << std::endl;
		return;
	}

	if (!consistencyCheck()) {
		std::cout << "Trie is broken!" << std::endl;
		return;
	}

	ProgressInfo progressInfo;
	progressInfo.begin(m_nodeCount);
	uint32_t finishedNodeCount = 0;
	std::cout << "Serializing Trie:" << std::endl;
	
	std::deque< std::deque< MultiTrieNode<unsigned int>* >  > trieNodeQue;
	if (m_nodeCount < 200 * 1000 * 1000)
		rootNode()->addNodesSorted(0, trieNodeQue);
	else
		trieNodeQue.resize(depth);
	for(int curLevel = depth-1; curLevel >= 0; curLevel--) {
		std::deque< MultiTrieNode<unsigned int>* > & nodeQue = trieNodeQue.at(curLevel);

		if (nodeQue.size() == 0)
			rootNode()->addNodesSortedInLevel(curLevel, 0, nodeQue);

		uint32_t curNodeCount = 0;
		for(std::deque< MultiTrieNode<unsigned int>* >::const_reverse_iterator it = nodeQue.rbegin(); it != nodeQue.rend(); ++it) {
			MultiTrieNode<unsigned int> * curNode = *it;
			
			Static::TrieNodeCreationInfo nodeInfo;

			nodeInfo.nodeStr = curNode->c;
			curNode->insertSortedChildChars(nodeInfo.childChars);
			nodeInfo.charWidth = 1;
			if (nodeInfo.childChars.size() > 0) {
				nodeInfo.charWidth = (nodeInfo.childChars.back() > 0xFF ? 2 : 1);
			}

			if (nodeInfo.childChars.size() != curNode->children.size()) {
				std::cout << "Error:  nodeInfo.childChars.size()=" << nodeInfo.childChars.size() << "!= curNode->children.size()=" << curNode->children.size() << std::endl;
			}
			
			//Get our child ptrs
			if (curNode->temporalPrivateStorage) {
				std::deque<uint32_t> tmp;
				TemporalPrivateNodeStorage * tmpStorage = dynamic_cast<TemporalPrivateNodeStorage*>(curNode->temporalPrivateStorage);
				tmpStorage->staticChildPointers.swap(tmp);

				//now do the remapping for the relative ptrs. stored ptrs offsets from the end of the list
				for(size_t i = 0; i < tmp.size(); i++) {
					nodeInfo.childPtrs.push_back(config.trieList->size() - tmp[i]);
				}
			}
			
			if (nodeInfo.childChars.size() != nodeInfo.childPtrs.size()) {
				std::cout << "Error: (childChars.size() != childPtrs.size()) = (" << nodeInfo.childChars.size() << " != " << nodeInfo.childPtrs.size() << ")" << std::endl;
				curNode->dump();
			}

			if (!handleNodeIndices(curNode, curLevel, false, config, nodeInfo)) {
				std::cout <<  "Failed to handle node Item index" << std::endl;
			}

			unsigned int err =  StaticTrieNodeT::prependNewNode(nodeInfo, *(config.trieList));
			if (StaticTrieNodeT::isError(err)) {
				std::cout << "ERROR: " << StaticTrieNodeT::errorString(err) << std::endl;
			}

			//Add ourself to the child ptrs of our parent
			if (curNode->parent) {
				TemporalPrivateNodeStorage * parentTempStorage = dynamic_cast<TemporalPrivateNodeStorage*>(curNode->parent->temporalPrivateStorage);
				if (!parentTempStorage) {
					parentTempStorage = new TemporalPrivateNodeStorage();
					curNode->parent->temporalPrivateStorage = parentTempStorage;
				}
				uint32_t nodeOffset = config.trieList->size();
				parentTempStorage->staticChildPointers.push_front(nodeOffset);
			}

			
			//delete children if config says so
			if (config.deleteRootTrie) {
				curNode->deleteChildren();
			}
			else {//Delete all child temporal node storages as those are not needed anymore
				for(Node::ChildNodeIterator it = curNode->children.begin(); it != curNode->children.end(); ++it) {
					it->second->deleteTemporalPrivateStorage();
				}
			}

			curNodeCount++;
			finishedNodeCount++;
			progressInfo(finishedNodeCount, "GeneralizedTrie::Bottom-up-Serialization");
		}
		trieNodeQue.pop_back();
	}
	progressInfo.end("GeneralizedTrie::Bottom-up-Serialization");
	rootNode()->deleteTemporalPrivateStorage();
}


//---------------------Check functions---------------------------------------->


template<class ItemType>
bool
GeneralizedTrie<ItemType>::consistencyCheck() const {
	if (!m_root) {
		std::cout << "GeneralizedTrie<ItemType>::consistencyCheck: No root node" << std::endl;
		return true;
	}
	if (m_root->parent) {
		std::cout << "GeneralizedTrie<ItemType>::consistencyCheck: root node has a parent" << std::endl;
		return false;
	}
	if (!m_root->checkParentChildRelations()) {
		std::cout << "GeneralizedTrie<ItemType>::consistencyCheck: parent->child relations are wrong" << std::endl;
		return false;
	}

	if (!consistencyCheckRecurse(m_root)) {
		std::cout << "GeneralizedTrie<ItemType>::consistencyCheck: item sets are not sorted" << std::endl;
		return false;
	}
	
	
	return true;
}

template<class ItemType>
bool
GeneralizedTrie<ItemType>::consistencyCheckRecurse(Node * node) const {
	if (node) {
		if (node->exactValues.size() > 2) {
			Node::ConstItemSetIterator front = node->exactValues.begin();
			Node::ConstItemSetIterator behind = front++;
			Node::ConstItemSetIterator end = node->exactValues.end();
			
			for(; front != end; ++front, ++behind) {
				if (*front < *behind)
					return false;
			}
		}
		if (node->subStrValues.size() > 2) {
			Node::ConstItemSetIterator front = node->subStrValues.begin();
			Node::ConstItemSetIterator behind = front++;
			Node::ConstItemSetIterator end = node->subStrValues.end();
			
			for(; front != end; ++front, ++behind) {
				if (*front < *behind)
					return false;
			}
		}
		
		for(Node::ConstChildNodeIterator it = node->children.begin(); it != node->children.end(); ++it) {
			if (!consistencyCheckRecurse(it->second))
				return false;
		}
	}
	return true;
}

template<class ItemType>
bool
GeneralizedTrie<ItemType>::
checkTrieEqualityRecurse(Node * curNode, Static::TrieNode curStaticNode, std::string firstChar) {
	std::string staticNodeStr = firstChar + curStaticNode.str();
	if (curNode->c != staticNodeStr) {
		std::cout << "FATAL: node.str() is wrong:"  << std::endl;
		curNode->dump();
		curStaticNode.dump();
		return false;
	}
	if (curNode->children.size() != curStaticNode.childCount()) {
		std::cout << "FATAL: node.children.size() is wrong:"  << std::endl;
		curNode->dump();
		curStaticNode.dump();
		return false;
	}

	for(size_t i = 0; i < curStaticNode.childCount(); i++) {
		uint16_t childChar = curStaticNode.childCharAt(i);
		std::string childCharStr = "";
		utf8::append(childChar, std::back_inserter(childCharStr));
		if (curNode->children.count(childChar) == 0) {
			std::cout << "FATAL: node.children[" << childChar << "] is wrong:"  << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
		if (!checkTrieEqualityRecurse(curNode->children.at(childChar), curStaticNode.childAt(i), childCharStr) ) {
			return false;
		}
	}
	return true;
}

template<class ItemType>
bool
GeneralizedTrie<ItemType>::
checkTrieEquality(GeneralizedTrieCreatorConfig config, Static::GeneralizedTrie staticTrie) {
	return checkTrieEqualityRecurse(rootNode(), staticTrie.getRootNode(), "");
}

template<class ItemType>
bool
GeneralizedTrie<ItemType>::
checkIndexEqualityRecurse(Node* curNode, sserialize::Static::TrieNode curStaticNode, sserialize::Static::GeneralizedTrie& staticTrie, StringCompleter::SupportedQuerries sqtype) {

	if (sqtype & sserialize::StringCompleter::SQ_EXACT) {
		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_EXACT));
		if (idx != curNode->exactValues) {
			std::cout << "FATAL: ExactIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
	}
	if (sqtype & sserialize::StringCompleter::SQ_PREFIX) {
		std::set<ItemIdType> destination;
		curNode->insertExactValuesRecursive(destination);
		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_PREFIX));
		if (idx != destination) {
			std::cout << "FATAL: PrefixIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
	}
	if (sqtype & sserialize::StringCompleter::SQ_SUFFIX) {
		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_SUFFIX));
		std::set<ItemIdType> s(curNode->subStrValues.begin(), curNode->subStrValues.end());
		s.insert(curNode->exactValues.begin(), curNode->exactValues.end());
		if (idx != s) {
			std::cout << "FATAL: SuffixIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
	}
	if (sqtype & sserialize::StringCompleter::SQ_SUFFIX_PREFIX) {
		std::set<ItemIdType> destination;
		curNode->insertAllValuesRecursive(destination);

		ItemIndex idx(staticTrie.getItemIndexFromNode(curStaticNode, sserialize::StringCompleter::QT_SUFFIX_PREFIX));
		if (idx != destination) {
			std::cout << "FATAL: SuffixPrefixIndex broken at:" << std::endl;
			curNode->dump();
			curStaticNode.dump();
			return false;
		}
	}

	for(size_t i = 0; i < curStaticNode.childCount(); i++) {
		uint16_t childChar = curStaticNode.childCharAt(i);
		if (curNode->children.count(childChar) == 0) {
			return false;
		}
		if (!checkIndexEqualityRecurse(curNode->children.at(childChar), curStaticNode.childAt(i), staticTrie, sqtype) ) {
			return false;
		}
	}
	return true;
}


template<class ItemType>
bool
GeneralizedTrie<ItemType>::
checkIndexEquality(sserialize::GeneralizedTrieCreatorConfig config, sserialize::Static::GeneralizedTrie staticTrie, sserialize::StringCompleter::SupportedQuerries sqtype) {
	Node * rootNode = m_root;
	return checkIndexEqualityRecurse(rootNode, staticTrie.getRootNode(), staticTrie, sqtype);
}

template<class ItemType>
bool
GeneralizedTrie<ItemType>::
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
					if (s != trie.indexFromPosition(posInFTrie, StringCompleter::QT_SUFFIX_PREFIX)) {
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


template<class ItemType>
bool
GeneralizedTrie<ItemType>::
checkFlatTrieEquality(const sserialize::Static::FlatGST & trie, bool checkIndex) {
	uint32_t posInFTrie = 0;
	return (trie.size() == m_nodeCount) && checkFlatTrieEquality(m_root, "", posInFTrie, trie, checkIndex);
}


}//end namespace

#endif
