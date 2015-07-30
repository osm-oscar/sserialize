#ifndef SSERIALIZE_GENERALIZED_TRIE_BASE_TRIE_H
#define SSERIALIZE_GENERALIZED_TRIE_BASE_TRIE_H
#include "Helpers.h"
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/containers/WindowedArray.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/Static/FlatGeneralizedTrie.h>
#include <sserialize/utility/debuggerfunctions.h>
#include <assert.h>
#include <sserialize/strings/DiacriticRemover.h>
#define GST_ITEMID_MAX 0xFFFFFFFF

namespace sserialize {
namespace GeneralizedTrie {

template<typename IndexStorageContainer = sserialize::WindowedArray<uint32_t> >
class BaseTrie {
public:
	typedef unsigned int ItemIdType;
	typedef MultiTrieNode<ItemIdType, IndexStorageContainer> Node;
	typedef typename Node::ChildNodeIterator ChildNodeIterator;
	typedef typename Node::ConstChildNodeIterator ConstChildNodeIterator;
	typedef typename Node::ItemSetIterator ItemSetIterator;
	typedef typename Node::ConstItemSetIterator ConstItemSetIterator;
	typedef typename Node::ItemSetContainer ItemSetContainer;
	typedef std::unordered_set<std::string> StringsContainer;
protected: //variables
	uint32_t m_count;
	uint32_t m_nodeCount;
	Node * m_root;
	bool m_caseSensitive;
	bool m_addTransDiacs;
	std::unordered_set<uint32_t> m_suffixDelimeters;


protected: //Insertion function
	Node * nextSuffixNode(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd);
	///moves to the next suffix entry and returns the beginning
	void nextSuffixString(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd);
	
protected: //completion functions
	void completionRecurse(Node * node, std::string::const_iterator strBegin, const std::string::const_iterator & strEnd, sserialize::StringCompleter::QuerryType qt, std::set<ItemIdType> & destination) const;

protected: //check functions
	/** Fixes Problems in the trie before serialization. Current Problems: Nodes with a string length > 255, calls swap on the IndexStorageContainer */
	void trieSerializationProblemFixer(Node * node);
	bool consistencyCheckRecurse(Node * node) const;

protected:
	template<typename T_ITEM_FACTORY>
	bool trieFromStringsFactory(const T_ITEM_FACTORY & stringsFactory, const uint32_t begin, const uint32_t end, std::unordered_map<std::string, std::unordered_set<Node*> > & strIdToSubStrNodes, std::unordered_map<std::string, std::unordered_set<Node*> > & strIdToExactNodes);

public: //Initalization and Destruction
	BaseTrie();
	BaseTrie(bool caseSensitive);
	virtual ~BaseTrie();
	void clear();
	void swap( BaseTrie & other);
	inline void setCaseSensitivity(bool c) { m_caseSensitive = c; }
	inline void setSuffixDelimeters(const std::unordered_set<uint32_t> & s) { m_suffixDelimeters = s; }
	
	/** If you set this to true, then transliterated versions of itemstrings are added as well
	  * This is only compatible with the Static::Trie. The FlatGST does not support this!
	  *
	  */
	inline void setAddTransliteratedDiacritics(bool c) { m_addTransDiacs = c; }
	
	inline bool isCaseSensitive() { return m_caseSensitive; }
	inline bool getSuffixDelimeters() { return m_suffixDelimeters; }

public: //Fill functions
	Node* at(const std::string::const_iterator & strBegin, const std::string::const_iterator & strEnd);
	Node* operator[](const std::string & str);

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
m_addTransDiacs(false)
{
}

template<typename IndexStorageContainer>
BaseTrie<IndexStorageContainer>::BaseTrie(bool caseSensitive) :
m_count(0),
m_nodeCount(0),
m_root(0),
m_caseSensitive(caseSensitive),
m_addTransDiacs(false)
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
	swap(m_caseSensitive, other.m_caseSensitive);
	swap(m_addTransDiacs, other.m_addTransDiacs);
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
	sserialize::nextSuffixString(strIt, strEnd, m_suffixDelimeters);
	return node;
}

template<typename IndexStorageContainer>
template<typename T_ITEM_FACTORY>
bool BaseTrie<IndexStorageContainer>::trieFromStringsFactory(const T_ITEM_FACTORY & stringsFactory, const uint32_t begin, const uint32_t end, std::unordered_map<std::string, std::unordered_set<Node*> > & strIdToSubStrNodes, std::unordered_map<std::string, std::unordered_set<Node*> > & strIdToExactNodes) {
	DiacriticRemover transLiterator;
	if (m_addTransDiacs) {
		UErrorCode status = transLiterator.init();
		if (U_FAILURE(status)) {
			std::cerr << "Failed to create translitorated on request: " << u_errorName(status) << std::endl;
			return false;
		}
	}
	StringsContainer itemPrefixStrings, itemSuffixStrings;

	clear();
	ProgressInfo progressInfo;

	
	//This is the first part (create the trie)
	progressInfo.begin(end-begin, "BaseTrie::trieFromStringsFactory: Creating Trie form strings");
	for(uint32_t i(0); i < end; ++i) {
		itemPrefixStrings.clear();
		itemSuffixStrings.clear();
		stringsFactory(i, itemPrefixStrings, itemSuffixStrings);
		for(std::string str : itemPrefixStrings) {
			if (!m_caseSensitive) {
				str = sserialize::unicode_to_lower(str);
			}
			at(str.cbegin(), str.cend());
			if (m_addTransDiacs) {
				transLiterator.transliterate(str);
				at(str.cbegin(), str.cend());
			}
		}
		
		for(std::string str : itemSuffixStrings) {
			if (!m_caseSensitive) {
				str = sserialize::unicode_to_lower(str);
			}
			std::string::const_iterator strIt(str.cbegin());
			std::string::const_iterator strEnd(str.cend());
			while (strIt != strEnd) {
				nextSuffixNode(strIt, strEnd);
			}
			if (m_addTransDiacs) {
				transLiterator.transliterate(str);
				std::string::const_iterator strIt(str.cbegin());
				std::string::const_iterator strEnd(str.cend());
				while (strIt != strEnd) {
					nextSuffixNode(strIt, strEnd);
				}
			}
		}

		progressInfo(i);
	}
	progressInfo.end();
	
	if (!consistencyCheck()) {
		std::cout << "Trie is broken after BaseTrie::trieFromStringsFactory" << std::endl;
		return false;
	}

	progressInfo.begin(end-begin, "BaseTrie::trieFromStringsFactory: Finding String->node mappings" );
	#pragma omp parallel for 
	for(uint32_t i = 0; i < end; ++i) {
		StringsContainer myItemPrefixStrings, myItemSuffixStrings;
		stringsFactory(i, myItemPrefixStrings, myItemSuffixStrings);
		for(std::string str : myItemPrefixStrings) {
			std::string itemStr = str;
			if (!m_caseSensitive) {
				str = sserialize::unicode_to_lower(str);
			}
			Node * n = at(str.cbegin(), str.cend());
			#pragma omp critical
			{
				strIdToExactNodes[itemStr].insert(n);
			}
			if (m_addTransDiacs) {
				transLiterator.transliterate(str);
				#pragma omp critical
				{
					strIdToExactNodes[itemStr].insert(n);
				}
			}
		}
		std::unordered_set<Node*> mySuffixNodes;
		for(std::string str : myItemSuffixStrings) {
			mySuffixNodes.clear();
			std::string itemStr = str;
			if (!m_caseSensitive) {
				str = sserialize::unicode_to_lower(str);
			}
			std::string::const_iterator strIt(str.cbegin());
			std::string::const_iterator strEnd(str.cend());
			while (strIt != strEnd) {
				mySuffixNodes.insert( nextSuffixNode(strIt, strEnd) );
			}
			if (m_addTransDiacs) {
				transLiterator.transliterate(str);
				std::string::const_iterator strIt(str.cbegin());
				std::string::const_iterator strEnd(str.cend());
				while (strIt != strEnd) {
					mySuffixNodes.insert( nextSuffixNode(strIt, strEnd) );
				}
			}
			#pragma omp critical
			{
				strIdToSubStrNodes[itemStr].insert(mySuffixNodes.cbegin(), mySuffixNodes.cend());
			}
		}
		progressInfo(i);
	}
	progressInfo.end();

	if (!consistencyCheck()) {
		std::cout << "Trie is broken after BaseTrie::trieFromStringsFactory::findStringNodes" << std::endl;
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