#ifndef SSERIALIZE_GENERALIZED_TRIE_BASE_TRIE_H
#define SSERIALIZE_GENERALIZED_TRIE_BASE_TRIE_H
#include "Helpers.h"
#include <sserialize/containers/StringsItemDBWrapper.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/templated/StringsItemDBWrapperPrivateSIDB.h>
#include <sserialize/templated/WindowedArray.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/Static/FlatGeneralizedTrie.h>
#include <sserialize/utility/debuggerfunctions.h>
#include <assert.h>
#include <sserialize/utility/DiacriticRemover.h>
#define GST_ITEMID_MAX 0xFFFFFFFF

namespace sserialize {
namespace GeneralizedTrie {

template<typename IndexStorageContainer = std::vector<uint32_t> >
class BaseTrie {
public:
	typedef unsigned int ItemIdType;
	typedef MultiTrieNode<ItemIdType, IndexStorageContainer> Node;
	typedef typename Node::ChildNodeIterator ChildNodeIterator;
	typedef typename Node::ConstChildNodeIterator ConstChildNodeIterator;
	typedef typename Node::ItemSetIterator ItemSetIterator;
	typedef typename Node::ConstItemSetIterator ConstItemSetIterator;
	typedef typename Node::ItemSetContainer ItemSetContainer;
protected: //variables
	uint32_t m_count;
	uint32_t m_nodeCount;
	Node * m_root;
	std::vector<std::string> m_prefixStrings;
	bool m_caseSensitive;
	bool m_addTransDiacs;
	bool m_isSuffixTrie;
	std::unordered_set<uint32_t> m_suffixDelimeters;


protected: //Insertion function
	Node * nextSuffixNode(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd);
	///moves to the next suffix entry and returns the beginning
	void nextSuffixString(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd);
	
protected: //completion functions
	void completionRecurse(Node * node, std::string::const_iterator strBegin, const std::string::const_iterator & strEnd, sserialize::StringCompleter::QuerryType qt, std::set<ItemIdType> & destination) const;

protected: //check functions
	/** This compactifies stl-containers to decrease their size-overhead and removes all exactIndices from suffixIndices
		It calls at least swap, ItemSetContainer() and diffSortedContainer<ItemSetContainer>()
	*/
	void compactify(Node * node);
	/** Fixes Problems in the trie before serialization. Current Problems: Nodes with a string length > 255, calls swap on the IndexStorageContainer */
	void trieSerializationProblemFixer(Node * node);
	bool consistencyCheckRecurse(Node * node) const;

protected:
	template<typename T_ITEM_FACTORY, typename T_ITEM>
	bool trieFromStringsFactory(const T_ITEM_FACTORY & stringsFactory, std::unordered_map<std::string, std::unordered_set<Node*> > & strIdToSubStrNodes, std::unordered_map<std::string, std::unordered_set<Node*> > & strIdToExactNodes);

	
public: //Initalization and Destruction
	BaseTrie();
	BaseTrie(bool caseSensitive, bool suffixTrie);
	virtual ~BaseTrie();
	void clear();
	void swap( BaseTrie & other);
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
	template<typename ItemType>
	bool setDB(const StringsItemDBWrapper< ItemType >& db);
	
	/** This will create a trie out of a strings factory
	  * The Stringsfactory should provide begin(), end() iterators with their value_type having begin(), end() as well
	  * Dereferencing the StringFactory::const_iterator can yield a temporary value which should have a fast copy or move operation
	  */
	template<typename T_ITEM_FACTORY, typename T_ITEM>
	bool fromStringsFactory(const T_ITEM_FACTORY & stringsFactor);

public://Completion
	ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qt = sserialize::StringCompleter::QT_SUBSTRING) const;
	ItemIndex complete(const std::deque< std::string > & strs, sserialize::StringCompleter::QuerryType qt = sserialize::StringCompleter::QT_SUBSTRING) const;

public: //Access functions
	Node * rootNode() { return m_root;}

public: //Serialization functions
	uint32_t getDepth() { return m_root->depth();}
	
public: //Check functions
	bool consistencyCheck() const;
};


//------------------------Creation/Deletion functions-------------------------------->

template<typename IndexStorageContainer>
BaseTrie<IndexStorageContainer>::BaseTrie() :
m_count(0),
m_nodeCount(0),
m_root(0),
m_caseSensitive(false),
m_addTransDiacs(false),
m_isSuffixTrie(false)
{
}

template<typename IndexStorageContainer>
BaseTrie<IndexStorageContainer>::BaseTrie(bool caseSensitive, bool suffixTrie) :
m_count(0),
m_nodeCount(0),
m_root(0),
m_caseSensitive(caseSensitive),
m_addTransDiacs(false),
m_isSuffixTrie(suffixTrie)
{}

template<typename IndexStorageContainer>
BaseTrie<IndexStorageContainer>::~BaseTrie() {
	clear();
}

template<typename IndexStorageContainer>
void
BaseTrie<IndexStorageContainer>::clear() {
	delete m_root;
	m_root = 0;

	m_count = 0;
	m_nodeCount = 0;
}

template<typename IndexStorageContainer>
void
BaseTrie<IndexStorageContainer>::swap( BaseTrie & other) {
	using std::swap;
	swap(m_count, other.m_count);
	swap(m_nodeCount, other.m_nodeCount);
	swap(m_root, other.m_root);
	swap(m_prefixStrings, other.m_prefixStrings);
	swap(m_caseSensitive, other.m_caseSensitive);
	swap(m_addTransDiacs, other.m_addTransDiacs);
	swap(m_isSuffixTrie, other.m_isSuffixTrie);
	swap(m_suffixDelimeters, other.m_suffixDelimeters);
}

//------------------------Insertion functions-------------------------------->

template<typename IndexStorageContainer>
typename BaseTrie<IndexStorageContainer>::Node *
BaseTrie<IndexStorageContainer>::operator[](const std::string & str) {
	return at(str.begin(), str.end());
}

template<typename IndexStorageContainer>
typename BaseTrie<IndexStorageContainer>::Node *
BaseTrie<IndexStorageContainer>::at(const std::string::const_iterator & strBegin, const std::string::const_iterator & strEnd) {
	using std::swap;

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
			swap(oldStrNode->children, current->children);
			swap(oldStrNode->exactValues, current->exactValues);
			swap(oldStrNode->subStrValues, current->subStrValues);

			oldStrNode->fixChildParentPtrRelation();
			
			//save char for children
			uint32_t c = cItUCode;
			
			//clear data, adjust to new c
			current->exactValues = ItemSetContainer();
			current->subStrValues = ItemSetContainer();
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
			swap(oldStrNode->children, current->children);
			swap(oldStrNode->exactValues, current->exactValues);
			swap(oldStrNode->subStrValues, current->subStrValues);
			oldStrNode->fixChildParentPtrRelation();
			
			current->children.clear();
			current->exactValues = ItemSetContainer();
			current->subStrValues = ItemSetContainer();
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
template<typename IndexStorageContainer>
typename  BaseTrie<IndexStorageContainer>::Node *
BaseTrie<IndexStorageContainer>::nextSuffixNode(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd) {
	Node * node = at(strIt, strEnd);
	nextSuffixString(strIt, strEnd);
	return node;
}

///Inserts the string between strIt and strEnd. If suffixDelimeters is given, the string split between those code points
template<typename IndexStorageContainer>
void BaseTrie<IndexStorageContainer>::nextSuffixString(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd) {
	if (m_suffixDelimeters.size()) {
		while (strIt != strEnd) {
			if (m_suffixDelimeters.count( utf8::next(strIt, strEnd) ) > 0)
				break;
		}
	}
	else if (strIt != strEnd) {
		utf8::next(strIt, strEnd);
	}
}

template<typename IndexStorageContainer>
template<typename T_ITEM_FACTORY, typename T_ITEM>
bool BaseTrie<IndexStorageContainer>::trieFromStringsFactory(const T_ITEM_FACTORY & stringsFactory, std::unordered_map<std::string, std::unordered_set<Node*> > & strIdToSubStrNodes, std::unordered_map<std::string, std::unordered_set<Node*> > & strIdToExactNodes) {
	DiacriticRemover transLiterator;
	if (m_addTransDiacs) {
		UErrorCode status = transLiterator.init();
		if (U_FAILURE(status)) {
			std::cerr << "Failed to create translitorated on request: " << u_errorName(status) << std::endl;
			return false;
		}
	}

	clear();
	ProgressInfo progressInfo;

	{
		uint32_t count = 0;
		std::unordered_set<std::string> strings;
		progressInfo.begin(stringsFactory.end()-stringsFactory.begin(), "BaseTrie::trieFromStringsFactory: Gathering strings");
		for(typename T_ITEM_FACTORY::const_iterator itemsIt(stringsFactory.begin()), itemsEnd(stringsFactory.end()); itemsIt != itemsEnd; ++itemsIt) {
			T_ITEM item = *itemsIt;
			for(typename T_ITEM::const_iterator itemStrsIt(item.begin()), itemStrsEnd(item.end()); itemStrsIt != itemStrsEnd; ++itemStrsIt) {
				strings.insert(*itemStrsIt);
			}
			progressInfo(++count);
		}
		progressInfo.end();
		m_prefixStrings = std::vector<std::string>(strings.begin(), strings.end());
		std::sort(m_prefixStrings.begin(), m_prefixStrings.end());
	}
	
	//This is the first part (create the trie)
	progressInfo.begin(m_prefixStrings.size(), "BaseTrie::trieFromStringsFactory: Creating Trie form strings");
	uint32_t count = 0;
	for(std::vector<std::string>::const_iterator strsIt(m_prefixStrings.cbegin()); strsIt != m_prefixStrings.cend(); ++strsIt) {
		std::vector<std::string> strs;
		if (m_caseSensitive)
			strs.push_back(*strsIt);
		else
			strs.push_back( unicode_to_lower(*strsIt) );
		
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
		progressInfo(++count);
	}
	progressInfo.end();
	
	if (!consistencyCheck()) {
		std::cout << "Trie is broken after BaseTrie::trieFromStringsFactory" << std::endl;
		return false;
	}

	//get all nodes for all strings
	strIdToSubStrNodes.reserve(m_prefixStrings.size());
	strIdToExactNodes.reserve(m_prefixStrings.size());
	
	progressInfo.begin(m_prefixStrings.size(), "BaseTrie::trieFromStringsFactory: Finding String->node mappings" );
	count = 0;
	#pragma omp parallel for 
	for(size_t i = 0; i <  m_prefixStrings.size(); ++i) { //we need to do it like that due to the parallelisation (map::iterator does not work here)
		std::vector<std::string> strs;
		if (m_caseSensitive)
			strs.push_back(m_prefixStrings[i]);
		else
			strs.push_back( unicode_to_lower(m_prefixStrings[i]) );
		
		if (m_addTransDiacs) {
			strs.push_back( strs.back() );
			transLiterator.transliterate(strs.back());
		}

		
		for(std::vector<std::string>::const_iterator insStrIt = strs.begin(); insStrIt != strs.end(); ++insStrIt) {
			std::string::const_iterator strIt(insStrIt->begin());
			std::string::const_iterator strEnd(insStrIt->end());
			#pragma omp critical
			{strIdToExactNodes[m_prefixStrings[i]].insert( at(strIt, strEnd) );}
			if (m_isSuffixTrie) {
				while (strIt != strEnd) {
					Node * node = nextSuffixNode(strIt, strEnd);
					#pragma omp critical
					{strIdToSubStrNodes[m_prefixStrings[i]].insert( node );}
				}
			}
		}
		#pragma omp atomic
		++count;
		
		if (count % 100 == 0) {
			#pragma omp critical
			progressInfo(count);
		}
	}
	progressInfo.end();

	if (!consistencyCheck()) {
		std::cout << "Trie is broken after BaseTrie::trieFromStringsFactory::findStringNodes" << std::endl;
		return false;
	}
	return true;
}


template<typename IndexStorageContainer>
template<typename T_ITEM_FACTORY, typename T_ITEM>
bool BaseTrie<IndexStorageContainer>::fromStringsFactory(const T_ITEM_FACTORY & stringsFactory) {
	std::unordered_map<std::string, std::unordered_set<Node*> > strIdToSubStrNodes;
	std::unordered_map<std::string, std::unordered_set<Node*> > strIdToExactNodes;
	
	if (!trieFromStringsFactory<T_ITEM_FACTORY, T_ITEM>(stringsFactory, strIdToSubStrNodes, strIdToExactNodes)) {
		return false;
	}
	
	//Now add the items
	ProgressInfo progressInfo;
	progressInfo.begin(stringsFactory.end()-stringsFactory.begin(), "BaseTrie::fromStringsFactory::insertItems");
	uint32_t count = 0;
	for(typename T_ITEM_FACTORY::const_iterator itemIt(stringsFactory.begin()), itemEnd(stringsFactory.end()); itemIt != itemEnd; ++itemIt) {
		std::unordered_set<Node*> exactNodes;
		std::unordered_set<Node*> suffixNodes;
		T_ITEM item = *itemIt;
		for(typename T_ITEM::const_iterator itemStrsIt(item.begin()), itemStrsEnd(item.end()); itemStrsIt != itemStrsEnd; ++itemStrsIt) {
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
		
		for(typename std::unordered_set<Node*>::iterator esit = exactNodes.begin(); esit != exactNodes.end(); ++esit) {
			(*esit)->exactValues.insert((*esit)->exactValues.end(), count);
		}
		for(typename std::unordered_set<Node*>::iterator it = suffixNodes.begin(); it != suffixNodes.end(); ++it) {
			(*it)->subStrValues.insert((*it)->subStrValues.end(), count);
		}

		progressInfo(++count);
	}
	progressInfo.end();
	std::cout << std::endl;
	if (!consistencyCheck()) {
		std::cout << "Trie is broken after BaseTrie::fromStringsFactory::insertItems" << std::endl;
		return false;
	}

	return true;
}

	
/** This function sets the DB the trie uses and creates the trie out of the DB data
  * It does so in 3 Steps:
  * 1. Create trie nodes with all the strings in the db's string table
  * 2. Gather for every string the end node (multiple end nodes for suffix)
  * 3. Add the item ids to the nodes by iterating over them, the string ids will tell where to put the ids
  *
  **/
template<typename IndexStorageContainer>
template<typename ItemType>
bool BaseTrie<IndexStorageContainer>::setDB(const StringsItemDBWrapper< ItemType >& db) {
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
	{
		m_prefixStrings.insert(m_prefixStrings.end(), db.strIdToStr().cbegin(), db.strIdToStr().cend());
		std::sort(m_prefixStrings.begin(), m_prefixStrings.end());
	}
	
	//This is the first part (create the trie)
	progressInfo.begin(db.strIdToStr().size());
	uint32_t count = 0;
	for(std::vector<std::string>::const_iterator strsIt = db.strIdToStr().begin(); strsIt != db.strIdToStr().end(); ++strsIt) {
		std::vector<std::string> strs;
		if (m_caseSensitive)
			strs.push_back(*strsIt);
		else
			strs.push_back( unicode_to_lower(*strsIt) );
		
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
		progressInfo(++count, "BaseTrie::setDB::createTrie");
	}
	if (!consistencyCheck()) {
		std::cout << "Trie is broken after BaseTrie::setDB::createTrie" << std::endl;
		return false;
	}

	//get all nodes for all strings
	std::unordered_map<unsigned int, std::set<Node*> > strIdToSubStrNodes; strIdToSubStrNodes.reserve(db.strIdToStr().size());
	std::unordered_map<unsigned int, std::set<Node*> > strIdToExactNodes; strIdToExactNodes.reserve(db.strIdToStr().size());
	
	progressInfo.begin(db.strIdToStr().size());
	count = 0;
	#pragma omp parallel for 
	for(size_t i = 0; i <  db.strIdToStr().size(); ++i) { //we need to do it like that due to the parallelisation (map::iterator does not work here)
		const std::string & dbStr = db.strIdToStr().at(i);
		std::vector<std::string> strs;
		if (m_caseSensitive)
			strs.push_back(dbStr);
		else
			strs.push_back( unicode_to_lower(dbStr) );
		
		if (m_addTransDiacs) {
			strs.push_back( strs.back() );
			transLiterator.transliterate(strs.back());
		}

		
		for(std::vector<std::string>::const_iterator insStrIt = strs.begin(); insStrIt != strs.end(); ++insStrIt) {
			std::string::const_iterator strIt(insStrIt->begin());
			std::string::const_iterator strEnd(insStrIt->end());
			#pragma omp critical
			{strIdToExactNodes[i].insert( at(strIt, strEnd) );}
			if (m_isSuffixTrie) {
				while (strIt != strEnd) {
					Node * node = nextSuffixNode(strIt, strEnd);
					#pragma omp critical
					{strIdToSubStrNodes[i].insert( node );}
				}
			}
		}
		#pragma omp atomic
		count++;
		progressInfo(count, "BaseTrie::setDB::findStringNodes");
	}
	progressInfo.end("BaseTrie::setDB::findStringNodes");

	if (!consistencyCheck()) {
		std::cout << "Trie is broken after BaseTrie::setDB::findStringNodes" << std::endl;
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
		
		for(typename std::set<Node*>::iterator esit = exactNodes.begin(); esit != exactNodes.end(); ++esit) {
			(*esit)->exactValues.insert((*esit)->exactValues.end(), i);
		}
		for(typename std::set<Node*>::iterator it = suffixNodes.begin(); it != suffixNodes.end(); ++it) {
			(*it)->subStrValues.insert((*it)->subStrValues.end(), i);
		}
		
		++count;
		progressInfo(count, "BaseTrie::setDB::insertItems");
	}
	progressInfo.end("BaseTrie::setDB::insertItems");
	std::cout << std::endl;
	if (!consistencyCheck()) {
		std::cout << "Trie is broken after BaseTrie::setDB::insertItems" << std::endl;
		return false;
	}

	return true;
}


//------------------------Completion functions-------------------------------->


template<typename IndexStorageContainer>
void
BaseTrie<IndexStorageContainer>::
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
		if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
			current->insertAllValuesRecursive(destination);
		}
	}
}

template<typename IndexStorageContainer>
ItemIndex
BaseTrie<IndexStorageContainer>::complete(const std::string& str, sserialize::StringCompleter::QuerryType qt) const {
	if (!m_caseSensitive) {
		qt = (sserialize::StringCompleter::QuerryType) (qt | sserialize::StringCompleter::QT_CASE_INSENSITIVE);
	}

	std::set<uint32_t> set;
	completionRecurse(m_root, str.begin(), str.end(), qt, set);
	std::vector<uint32_t> ds(set.begin(), set.end());
	return ItemIndex(ds);
}

template<typename IndexStorageContainer>
ItemIndex
BaseTrie<IndexStorageContainer>::complete(const std::deque< std::string >& strs, sserialize::StringCompleter::QuerryType qt) const {
	std::vector< ItemIndex > sets;
	for(size_t i = 0; i < strs.size(); i++) {
		sets.push_back(complete(strs[i], qt));
	}
	return ItemIndex::intersect(sets);
}

//private functions

template <class IndexStorageContainer>
void BaseTrie<IndexStorageContainer>::compactify(Node* node) {
	if (node) {
		inplaceDiffSortedContainer(node->subStrValues, node->exactValues);
		compactifyStlContainer(node->subStrValues);
		compactifyStlContainer(node->exactValues);
		for(typename Node::ChildNodeIterator i = node->children.begin(); i != node->children.end(); i++) {
			compactify(i->second);
		}
	}
}

template <class IndexStorageContainer>
void BaseTrie<IndexStorageContainer>::trieSerializationProblemFixer(Node* node) {
	using std::swap;
	if (node) {
		if (node->c.size() > 0xFF) {
			std::string oldStr;
			oldStr.swap(node->c);
			Node * newNode = new Node();
			newNode->parent = node;
			newNode->children.swap(node->children);
			swap(newNode->exactValues, node->exactValues);
			swap(newNode->subStrValues, node->subStrValues);
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
		
		for(ChildNodeIterator i = node->children.begin(); i != node->children.end(); i++) {
			trieSerializationProblemFixer(i->second);
		}
	}
}

//---------------------Check functions---------------------------------------->


template<typename IndexStorageContainer>
bool
BaseTrie<IndexStorageContainer>::consistencyCheck() const {
	if (!m_root) {
		std::cout << "BaseTrie<IndexStorageContainer>::consistencyCheck: No root node" << std::endl;
		return true;
	}
	if (m_root->parent) {
		std::cout << "BaseTrie<IndexStorageContainer>::consistencyCheck: root node has a parent" << std::endl;
		return false;
	}
	if (!m_root->checkParentChildRelations()) {
		std::cout << "BaseTrie<IndexStorageContainer>::consistencyCheck: parent->child relations are wrong" << std::endl;
		return false;
	}

	if (!consistencyCheckRecurse(m_root)) {
		std::cout << "BaseTrie<IndexStorageContainer>::consistencyCheck: item sets are not sorted" << std::endl;
		return false;
	}
	
	
	return true;
}

template<typename IndexStorageContainer>
bool
BaseTrie<IndexStorageContainer>::consistencyCheckRecurse(Node * node) const {
	if (node) {
		if (node->exactValues.size() > 2) {
			ConstItemSetIterator front = node->exactValues.begin();
			ConstItemSetIterator behind = front++;
			ConstItemSetIterator end = node->exactValues.end();
			
			for(; front != end; ++front, ++behind) {
				if (*front < *behind)
					return false;
			}
		}
		if (node->subStrValues.size() > 2) {
			ConstItemSetIterator front = node->subStrValues.begin();
			ConstItemSetIterator behind = front++;
			ConstItemSetIterator end = node->subStrValues.end();
			
			for(; front != end; ++front, ++behind) {
				if (*front < *behind)
					return false;
			}
		}
		
		for(ConstChildNodeIterator it = node->children.begin(); it != node->children.end(); ++it) {
			if (!consistencyCheckRecurse(it->second))
				return false;
		}
	}
	return true;
}

}}//end namespace

#endif