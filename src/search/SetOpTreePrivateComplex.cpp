#include <sserialize/search/SetOpTreePrivateComplex.h>
#include <string.h>
#include <sstream>
#include <istream>
#include <iterator>
#include <iostream>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/log.h>
#include <sserialize/containers/ItemIndexIteratorSetOp.h>

namespace sserialize {

SetOpTreePrivateComplex::Node::~Node() {
	for(size_t i = 0; i < children.size(); i++) {
		delete children[i];
	}
}

SetOpTreePrivateComplex::Node * SetOpTreePrivateComplex::Node::getDeepCopy(Node * parent) {
	SetOpTreePrivateComplex::Node * retNode = new Node();
	retNode->type = type;
	retNode->cqtype = cqtype;
	retNode->parent = parent;
	retNode->completeString = completeString;
	retNode->cached = cached;
	retNode->index = index;
	retNode->externalFunc = externalFunc;
	retNode->srcBegin = srcBegin;
	retNode->srcEnd = srcEnd;

	for(size_t i = 0; i < children.size(); i++) {
		retNode->children[i] = children[i]->getDeepCopy(retNode);
	}
	return retNode;
}

SetOpTreePrivateComplex::Node* SetOpTreePrivateComplex::Node::getMostRight() {
	if (!children.size())
		return this;
	if (children[1])
		return children[1]->getMostRight();
	else
		return children[0]->getMostRight();
}

SetOpTreePrivateComplex::Node* SetOpTreePrivateComplex::Node::getNodeWithSrcCollision(uint16_t position) {
	std::cout << "charhints: pos=" << position << "; mbegin=" << this->srcBegin << "; mend=" << this->srcEnd << std::endl;
	if (position < srcBegin) {
		if (children.size()) {
			if (children[0])
				return children[0]->getNodeWithSrcCollision(position);
		}
	}
	else if (position > srcEnd) {
		if (children.size()) {
			if (children[1])
				return children[1]->getNodeWithSrcCollision(position);
		}
	}
	else {
		return this;
	}
	return 0;
}


bool SetOpTreePrivateComplex::Node::efSupport(SetOpTree::SelectableOpFilter::SupportedOps op) const {
	return (this->externalFunc && this->externalFunc->supportedOps() & op);
}


void SetOpTreePrivateComplex::Node::printStructure(std::ostream& out) const {
	out << "(";
	if (type == COMPLETE) {
		out << "\"" << completeString << "\"";
	}
	else if (type == DIFFERENCE) {
		children.at(0)->printStructure(out);
		out << " - ";
		children.at(1)->printStructure(out);
	}
	else if (type == INTERSECT) {
		children.at(0)->printStructure(out);
		out << " / ";
		children.at(1)->printStructure(out);
	}
	else if (type == SYMMETRIC_DIFFERENCE) {
		children.at(0)->printStructure(out);
		out << " ^ ";
		children.at(1)->printStructure(out);
	}
	else if (type == UNITE) {
		children.at(0)->printStructure(out);
		out << " + ";
		children.at(1)->printStructure(out);
	}
	else if (type == EXTERNAL) {
		out << "$" << externalFunc->cmdString() << "[" << completeString << "]";
	}
	out << ")";
}

void SetOpTreePrivateComplex::Node::printStructure() const {
	printStructure(std::cout);

}

SetOpTreePrivateComplex::TreeDiffTypes SetOpTreePrivateComplex::completionStringDifference(const std::string & newString, const std::string & oldString) {
	std::string::const_iterator newIt = newString.begin();
	std::string::const_iterator oldIt = oldString.begin();
	const std::string::const_iterator newItEnd (newString.end());
	const std::string::const_iterator oldItEnd(oldString.end());
	uint32_t newItCode, oldItCode;
	while (newIt != newItEnd && oldIt != oldItEnd) {
		newItCode = utf8::peek_next(newIt, newItEnd);
		oldItCode = utf8::peek_next(oldIt, oldItEnd);
		if (newItCode != oldItCode)
			return UDT_DIFFERENT;
		else {
			utf8::next(newIt, newItEnd);
			utf8::next(oldIt, oldItEnd);
		}
	}
	if (newIt != newItEnd)
		return UDT_SUBSET;
	else if (oldIt != oldItEnd)
		return UDT_SUPERSET;
	else
		return UDT_EQUAL;
}

bool SetOpTreePrivateComplex::parseQuery(const std::string& queryString, SetOpTreePrivateComplex::Node*& treeRootNode, std::map< std::pair<std::string, uint8_t>, ItemIndex > & cmpStrings) {
	std::map<char, uint32_t> opMap;
	opMap['+'] =  Node::UNITE;
	opMap['-'] =  Node::DIFFERENCE;
	opMap['/'] =  Node::INTERSECT;
	opMap['^'] =  Node::SYMMETRIC_DIFFERENCE;
	TreeBuilder builder(Node::INTERSECT, opMap, m_externalFunctoids);
	treeRootNode = builder.build(queryString, cmpStrings);
	return treeRootNode != 0;
}


ItemIndex SetOpTreePrivateComplex::doSetOperationsRecurse(SetOpTreePrivateComplex::Node* node) {
	if (!node)
		return ItemIndex();
	if (node->cached)
		return node->index;
	ItemIndex tmpIndex;
	switch (node->type) {
		case (Node::COMPLETE):
			tmpIndex = m_completions.at( std::pair<std::string, uint8_t>(node->completeString, node->cqtype));
			break;
		case (Node::DIFFERENCE):
			if (node->children[0]->efSupport( SetOpTree::SelectableOpFilter::OP_DIFF_FIRST ) ) {
				tmpIndex = (*(node->children[0]->externalFunc))(SetOpTree::SelectableOpFilter::OP_DIFF_FIRST, node->children[0]->completeString, doSetOperationsRecurse(node->children[1]));
			}
			else if (node->children[1]->efSupport( SetOpTree::SelectableOpFilter::OP_DIFF_SECOND) ) {
				tmpIndex = (*(node->children[1]->externalFunc))(SetOpTree::SelectableOpFilter::OP_DIFF_SECOND, node->children[1]->completeString, doSetOperationsRecurse(node->children[0]));
			}
			else {
				tmpIndex =  ItemIndex::difference(doSetOperationsRecurse(node->children[0]), doSetOperationsRecurse(node->children[1]));
			}
			break;
		case (Node::EXTERNAL):
			tmpIndex = node->externalFunc->complete(node->completeString);
			break;
		case (Node::INTERSECT):
			if (node->children[0]->efSupport( SetOpTree::SelectableOpFilter::OP_INTERSECT ) ) {
				tmpIndex = (*(node->children[0]->externalFunc))(SetOpTree::SelectableOpFilter::OP_INTERSECT, node->children[0]->completeString, doSetOperationsRecurse(node->children[1]));
			}
			else if (node->children[1]->efSupport( SetOpTree::SelectableOpFilter::OP_INTERSECT) ) {
				tmpIndex = (*(node->children[1]->externalFunc))(SetOpTree::SelectableOpFilter::OP_INTERSECT, node->children[1]->completeString, doSetOperationsRecurse(node->children[0]));
			}
			else {
				tmpIndex =  ItemIndex::intersect(doSetOperationsRecurse(node->children[0]), doSetOperationsRecurse(node->children[1]));
			}
			break;
		case (Node::SYMMETRIC_DIFFERENCE):
			if (node->children[0]->efSupport( SetOpTree::SelectableOpFilter::OP_XOR ) ) {
				tmpIndex = (*(node->children[0]->externalFunc))(SetOpTree::SelectableOpFilter::OP_XOR, node->children[0]->completeString, doSetOperationsRecurse(node->children[1]));
			}
			else if (node->children[1]->efSupport( SetOpTree::SelectableOpFilter::OP_XOR) ) {
				tmpIndex = (*(node->children[1]->externalFunc))(SetOpTree::SelectableOpFilter::OP_XOR, node->children[1]->completeString, doSetOperationsRecurse(node->children[0]));
			}
			else {
				tmpIndex =  ItemIndex::symmetricDifference(doSetOperationsRecurse(node->children[0]), doSetOperationsRecurse(node->children[1]));
			}
			break;
		case (Node::UNITE):
			if (node->children[0]->efSupport( SetOpTree::SelectableOpFilter::OP_UNITE) ) {
				tmpIndex = (*(node->children[0]->externalFunc))(SetOpTree::SelectableOpFilter::OP_UNITE, node->children[0]->completeString, doSetOperationsRecurse(node->children[1]));
			}
			else if (node->children[1]->efSupport( SetOpTree::SelectableOpFilter::OP_UNITE) ) {
				tmpIndex = (*(node->children[1]->externalFunc))(SetOpTree::SelectableOpFilter::OP_UNITE, node->children[1]->completeString, doSetOperationsRecurse(node->children[0]));
			}
			else {
				tmpIndex =  ItemIndex::unite(doSetOperationsRecurse(node->children[0]), doSetOperationsRecurse(node->children[1]));
			}
			break;
		default:
			return ItemIndex();
			break;
	}
	
	node->index = tmpIndex;
	node->cached = true;
	return tmpIndex;
}

/*
 * INTERSECT:
 * [sub,sub],[diff/sup,a],[a,diff/sup]->intersect
 * [sub, eq]1[eq, sub]1 -> intersect with refTree.index
 * [eq,eq]1 -> userrefTree.index
 *
 *
 */


ItemIndex SetOpTreePrivateComplex::doSetOperationsRecurse(SetOpTreePrivateComplex::Node* node, SetOpTreePrivateComplex::Node* refTree, TreeDiffTypes & diff) {
	diff = UDT_DIFFERENT;
	if (!node)
			return ItemIndex();
	if (node->cached)
		return node->index;
	ItemIndex tmpIndex;
	switch (node->type) {
		case (Node::COMPLETE):
			if (refTree && refTree->type == node->type && refTree->cqtype == node->cqtype) {
				diff = completionStringDifference(node->completeString, refTree->completeString);
			}
			tmpIndex = m_completions.at(std::pair<std::string, uint8_t>(node->completeString, node->cqtype));
			break;
		case (Node::DIFFERENCE):
			if (false && refTree && node->type == refTree->type) {
				;
			}
			else {
// 				TreeDiffTypes dummy;
				tmpIndex = doSetOperationsRecurse(node);
			}
			break;
		case (Node::EXTERNAL):
			if (refTree && node->type == refTree->type && node->completeString == refTree->completeString) {
				diff = UDT_EQUAL;
				tmpIndex = refTree->index;
			}
			else {
				tmpIndex = node->externalFunc->complete(node->completeString);
			}
			break;
		case (Node::INTERSECT):
			if (refTree && node->type == refTree->type) {
				TreeDiffTypes lDiff = UDT_DIFFERENT;
				TreeDiffTypes rDiff = UDT_DIFFERENT;
				ItemIndex lIdx, rIdx;
				
				if (refTree->children[0]->type == Node::EXTERNAL && node->children[0]->type == refTree->children[0]->type) {
					if (node->children[0]->completeString == refTree->children[0]->completeString) {
						lIdx = doSetOperationsRecurse(node->children[0], refTree->children[0], lDiff); //This will just copy the index and set lDiff to UDT_EQUAL
					}
				}
				else {
					lIdx = doSetOperationsRecurse(node->children[0], refTree->children[0], lDiff);
				}
				
				if (refTree->children[1]->type == Node::EXTERNAL && node->children[1]->type == refTree->children[1]->type) {
					if (node->children[1]->completeString == refTree->children[1]->completeString) {
						rIdx = doSetOperationsRecurse(node->children[1], refTree->children[1], rDiff); //This will just copy the index and set lDiff to UDT_EQUAL
					}
				}
				else {
					rIdx = doSetOperationsRecurse(node->children[1], refTree->children[1], rDiff);
				}
				
				
				if (lDiff == UDT_EQUAL && rDiff == UDT_EQUAL) {
					tmpIndex = refTree->index;
					break;
				}
				else if (lDiff == UDT_SUBSET && rDiff == UDT_EQUAL) {
					tmpIndex  = lIdx / refTree->index;
				}
				else if (lDiff == UDT_SUBSET && rDiff == UDT_EQUAL) {
					tmpIndex = refTree->index / rIdx;
				}
				else {
					if (node->children[0]->efSupport( SetOpTree::SelectableOpFilter::OP_INTERSECT ) ) {
						tmpIndex = (*(node->children[0]->externalFunc))(SetOpTree::SelectableOpFilter::OP_INTERSECT, node->children[0]->completeString, doSetOperationsRecurse(node->children[1]));
					}
					else if (node->children[1]->efSupport( SetOpTree::SelectableOpFilter::OP_INTERSECT) ) {
						tmpIndex = (*(node->children[1]->externalFunc))(SetOpTree::SelectableOpFilter::OP_INTERSECT, node->children[1]->completeString, doSetOperationsRecurse(node->children[0]));
					}
					else { //node external => lIdx and rIdx were set above
						tmpIndex =  ItemIndex::intersect(lIdx, rIdx);
					}
				}
			}
			else {
				tmpIndex = doSetOperationsRecurse(node);
			}
			break;
		case (Node::SYMMETRIC_DIFFERENCE):
			if (false && refTree && node->type == refTree->type) {
				;
			}
			else {
				tmpIndex =  doSetOperationsRecurse(node);
			}
			break;
		case (Node::UNITE):
			if (false && refTree && node->type == refTree->type) {
				;
			}
			else {
				tmpIndex = doSetOperationsRecurse(node);
			}
			break;
		default:
			sserialize::err("SetOpTreePrivate::doSetOperationsRecurse-Update", "Node-type is invalid");
			return ItemIndex();
			break;
	}
	
	node->index = tmpIndex;
	node->cached = true;
	return tmpIndex;
}

//trie has to be should be cached
bool SetOpTreePrivateComplex::charHintsCheckChanged(Node * node, Node * child, const ItemIndex & index) {
	ItemIndex tmpIndex = index;
	while (node) {
		if (! node->cached) {
			std::cout << "Charhints uncached tree!" << std::endl;
			return false;
		}
		switch (node->type) {
			case (Node::COMPLETE):
				return false;
				break;
			case (Node::DIFFERENCE):
				if (node->children[0] == child) {
					tmpIndex =  ItemIndex::difference(tmpIndex, doSetOperationsRecurse(node->children[1]));
				}
				else {
					tmpIndex =  ItemIndex::difference(doSetOperationsRecurse(node->children[0]), tmpIndex);
				}
				break;
			case (Node::EXTERNAL):
				return false;
				break;
			case (Node::INTERSECT):
				tmpIndex = tmpIndex / node->index;
				break;
			case (Node::SYMMETRIC_DIFFERENCE):
				if (node->children[0] == child) {
					tmpIndex =  ItemIndex::symmetricDifference(tmpIndex, doSetOperationsRecurse(node->children[1]));
				}
				else {
					tmpIndex =  ItemIndex::symmetricDifference(doSetOperationsRecurse(node->children[0]), tmpIndex);
				}
				break;
			case (Node::UNITE):
				if (node->children[0] == child) {
					tmpIndex = ItemIndex::unite(tmpIndex, doSetOperationsRecurse(node->children[1]));
				}
				else {
					tmpIndex =  ItemIndex::unite(doSetOperationsRecurse(node->children[0]), tmpIndex);
				}
				break;
			default:
				return false;
				break;
		}
		child = node;
		node = node->parent;
	}
	return (tmpIndex.size() > 0);
}

std::set<uint16_t> SetOpTreePrivateComplex::getCharHintsFromNode(Node * node) {
	if (!node || node->completeString.empty() || node->type != Node::COMPLETE)
		return std::set<uint16_t>();
	//First get the node:
	std::set<uint16_t> charSet;
	
	std::cout << "Getting CharHints->ItemIndex map from node with nodestring" << node->completeString << std::endl;
	std::map<uint16_t, ItemIndex> chars = m_strCompleter.getNextCharacters(node->completeString, node->cqtype, true);
	std::cout << "Got CharHints->ItemIndex map with " << chars.size() << " possible next chars" << std::endl;
	for(std::map<uint16_t, ItemIndex>::const_iterator it = chars.begin(); it != chars.end(); ++it) {
		if (charHintsCheckChanged(node->parent, node, it->second)) {
			charSet.insert(it->first);
		}
	}
	return charSet;
}

ItemIndexIterator SetOpTreePrivateComplex::createItemIndexIteratorTree(SetOpTreePrivateComplex::Node* node) {
	if (!node)
		return ItemIndexIterator();
	if (node->cached) {
		return ItemIndexIterator(node->index);
	}

	switch (node->type) {
		case (Node::COMPLETE):
			return m_strCompleter.partialComplete(node->completeString, node->cqtype);
		case (Node::EXTERNAL):
			return node->externalFunc->partialComplete(node->completeString);
		case (Node::DIFFERENCE):
			if (node->children[0]->efSupport(SetOpTree::SelectableOpFilter::OP_DIFF_FIRST) ) {
				return (*(node->children[0]->externalFunc))(SetOpTree::SelectableOpFilter::OP_DIFF_FIRST, node->children[0]->completeString, createItemIndexIteratorTree(node->children[1]));
			}
			else if (node->children[1]->efSupport(SetOpTree::SelectableOpFilter::OP_DIFF_SECOND) ) {
				return (*(node->children[1]->externalFunc))(SetOpTree::SelectableOpFilter::OP_DIFF_SECOND, node->children[1]->completeString, createItemIndexIteratorTree(node->children[0]));
			}
			else {
				return  createItemIndexIteratorTree(node->children[0]) - createItemIndexIteratorTree(node->children[1]);
			}
		case (Node::INTERSECT):
			if (node->children[0]->efSupport(SetOpTree::SelectableOpFilter::OP_INTERSECT) ) {
				return (*(node->children[0]->externalFunc))(SetOpTree::SelectableOpFilter::OP_INTERSECT, node->children[0]->completeString, createItemIndexIteratorTree(node->children[1]));
			}
			else if (node->children[1]->efSupport(SetOpTree::SelectableOpFilter::OP_INTERSECT) ) {
				return (*(node->children[1]->externalFunc))(SetOpTree::SelectableOpFilter::OP_INTERSECT, node->children[1]->completeString, createItemIndexIteratorTree(node->children[0]));
			}
			else {
				return  createItemIndexIteratorTree(node->children[0]) / createItemIndexIteratorTree(node->children[1]);
			}
		case (Node::SYMMETRIC_DIFFERENCE):
			if (node->children[0]->efSupport(SetOpTree::SelectableOpFilter::OP_XOR) ) {
				return (*(node->children[0]->externalFunc))(SetOpTree::SelectableOpFilter::OP_XOR, node->children[0]->completeString, createItemIndexIteratorTree(node->children[1]));
			}
			else if (node->children[1]->efSupport(SetOpTree::SelectableOpFilter::OP_XOR) ) {
				return (*(node->children[1]->externalFunc))(SetOpTree::SelectableOpFilter::OP_XOR, node->children[1]->completeString, createItemIndexIteratorTree(node->children[0]));
			}
			else {
				return  createItemIndexIteratorTree(node->children[0]) ^ createItemIndexIteratorTree(node->children[1]);
			}
		case (Node::UNITE):
			if (node->children[0]->efSupport(SetOpTree::SelectableOpFilter::OP_UNITE) ) {
				return (*(node->children[0]->externalFunc))(SetOpTree::SelectableOpFilter::OP_UNITE, node->children[0]->completeString, createItemIndexIteratorTree(node->children[1]));
			}
			else if (node->children[1]->efSupport(SetOpTree::SelectableOpFilter::OP_UNITE) ) {
				return (*(node->children[1]->externalFunc))(SetOpTree::SelectableOpFilter::OP_UNITE, node->children[1]->completeString, createItemIndexIteratorTree(node->children[0]));
			}
			else {
				return  createItemIndexIteratorTree(node->children[0]) + createItemIndexIteratorTree(node->children[1]);
			}
		default:
			return ItemIndexIterator();
	}
}

SetOpTreePrivateComplex::SetOpTreePrivateComplex() :
SetOpTreePrivate(),
m_rootNode(0)
{
}

SetOpTreePrivateComplex::SetOpTreePrivateComplex(const SetOpTreePrivateComplex & other) :
SetOpTreePrivate(),
m_rootNode(0)
{
	m_strCompleter = other.m_strCompleter;
	m_queryString = other.m_queryString;
	m_externalFunctoids = other.m_externalFunctoids;
	m_completions = other.m_completions;
	if (other.m_rootNode)
		m_rootNode = other.m_rootNode->getDeepCopy(0);
}

SetOpTreePrivate * SetOpTreePrivateComplex::copy() const {
	return new SetOpTreePrivateComplex(*this);
}

SetOpTreePrivateComplex::~SetOpTreePrivateComplex() {
	clear();
}

void SetOpTreePrivateComplex::buildTree(const std::string & queryString) {
	if (m_rootNode) {
		if (m_queryString == queryString) {
			return;
		}
		else {
			clear();
		}
	}
	parseQuery(queryString, m_rootNode, m_completions);
	m_queryString = queryString;
}

ItemIndexIterator SetOpTreePrivateComplex::asItemIndexIterator(){
	if (m_rootNode) {
		return createItemIndexIteratorTree(m_rootNode);
	}
	return ItemIndexIterator();
}


ItemIndex SetOpTreePrivateComplex::update(const std::string& queryString) {
	std::map< std::pair<std::string, uint8_t>, ItemIndex> newCompletions;
	Node* newRootNode = 0;
	parseQuery(queryString, newRootNode, newCompletions);
	for(std::map< std::pair<std::string, uint8_t>, ItemIndex>::iterator it = newCompletions.begin(); it != newCompletions.end(); ++it) {

		if (m_completions.count(it->first) > 0) {
			(*it).second = m_completions[it->first];
		}
		else {
			(*it).second = m_strCompleter.complete(it->first.first, (StringCompleter::QuerryType) it->first.second);
		}
	}
	m_completions.swap(newCompletions);
	Node* oldRootNode = m_rootNode;
	m_rootNode = newRootNode;
	TreeDiffTypes dummy;
	ItemIndex idx = doSetOperationsRecurse(m_rootNode, oldRootNode, dummy);
	delete oldRootNode;
	return idx;
}

void SetOpTreePrivateComplex::doCompletions() {
	for(std::map< std::pair<std::string, uint8_t>, ItemIndex >::iterator it = m_completions.begin(); it != m_completions.end(); ++it) {
		(*it).second = m_strCompleter.complete(it->first.first, (StringCompleter::QuerryType) it->first.second);
	}
}

ItemIndex SetOpTreePrivateComplex::doSetOperations() {
	if (!m_rootNode)
		return ItemIndex();
	else
		return doSetOperationsRecurse(m_rootNode);
}


std::set<uint16_t> SetOpTreePrivateComplex::getCharacterHint(uint32_t posInQueryString) {
	if (m_rootNode) {
		Node * node = m_rootNode->getNodeWithSrcCollision(posInQueryString);
		if (node && node->type == Node::COMPLETE) {
			std::cout << "Select Node with completion string: " << node->completeString << " for charHints" << std::endl;
			return getCharHintsFromNode(node);
		}
	}
	return std::set<uint16_t>();
}


bool SetOpTreePrivateComplex::registerStringCompleter(const sserialize::StringCompleter & stringCompleter) {
	m_strCompleter = stringCompleter;
	return true;
}

bool SetOpTreePrivateComplex::registerExternalFunction(SetOpTree::ExternalFunctoid * function) {
	if (!function)
		return false;
	m_externalFunctoids[function->cmdString()] = RCPtrWrapper<SetOpTree::SelectableOpFilter>(function);
	return true;
}

bool SetOpTreePrivateComplex::registerSelectableOpFilter(SetOpTree::SelectableOpFilter * filter) {
	if (!filter)
		return false;
	m_externalFunctoids[filter->cmdString()] = RCPtrWrapper<SetOpTree::SelectableOpFilter>(filter);
	return true;
}


void SetOpTreePrivateComplex::clearTree() {
	if (m_rootNode)
		delete m_rootNode;
	m_rootNode = 0;
}

void SetOpTreePrivateComplex::clear() {
	clearTree();
}


std::ostream& SetOpTreePrivateComplex::printStructure(std::ostream& out) const {
	if (m_rootNode)
		m_rootNode->printStructure(out);
	return out;
}

}