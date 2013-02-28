#include <sserialize/containers/SetOpTree.h>
#include <string.h>
#include <sstream>
#include <istream>
#include <iterator>
#include <sserialize/containers/SetOpTreePrivateSimple.h>
#include <sserialize/containers/SetOpTreePrivateComplex.h>
#include <sserialize/containers/ItemIndexIteratorSetOp.h>

namespace sserialize {

ItemIndex SetOpTree::ExternalFunctoid::complete(const std::string& str ) {
	return (*this)(str);
}

ItemIndexIterator SetOpTree::ExternalFunctoid::partialComplete ( const std::string& str ) {
	return ItemIndexIterator( (*this)(str) );
}

ItemIndex SetOpTree::ExternalFunctoid::operator()(SupportedOps op, const std::string & str, const ItemIndex & partner) {
	switch (op) {
	case (OP_INTERSECT):
		return (*this)(str) / partner;
	case (OP_UNITE):
		return (*this)(str) + partner;
	case (OP_DIFF_FIRST):
		return (*this)(str) - partner;
	case (OP_DIFF_SECOND):
		return partner - (*this)(str);
	case (OP_XOR):
		return (*this)(str) xor partner;
	default:
		return ItemIndex();
	}
}

ItemIndexIterator SetOpTree::ExternalFunctoid::operator()(SupportedOps op, const std::string& str, const sserialize::ItemIndexIterator& partner) {
	switch (op) {
	case (OP_INTERSECT):
		return ItemIndexIterator( new ItemIndexIteratorSetOp( ItemIndexIterator((*this)(str)), partner, ItemIndexIteratorSetOp::OPT_INTERSECT) );
	case (OP_UNITE):
		return ItemIndexIterator( new ItemIndexIteratorSetOp( ItemIndexIterator((*this)(str)), partner, ItemIndexIteratorSetOp::OPT_UNITE) );
	case (OP_DIFF_FIRST):
		return ItemIndexIterator( new ItemIndexIteratorSetOp( ItemIndexIterator((*this)(str)), partner, ItemIndexIteratorSetOp::OPT_DIFF) );
	case (OP_DIFF_SECOND):
		return ItemIndexIterator( new ItemIndexIteratorSetOp(partner, ItemIndexIterator((*this)(str)), ItemIndexIteratorSetOp::OPT_DIFF) );
	case (OP_XOR):
		return ItemIndexIterator( new ItemIndexIteratorSetOp(partner, ItemIndexIterator((*this)(str)), ItemIndexIteratorSetOp::OPT_XOR) );
	default:
		return ItemIndexIterator();
	}
}


SetOpTree::SetOpTree(SotType type) : MyParentClass() {
	if (type == SOT_SIMPLE)
		setPrivate( new SetOpTreePrivateSimple() );
	else
		setPrivate( new SetOpTreePrivateComplex()); 
}

SetOpTree::SetOpTree(const SetOpTree & other) : RCWrapper< sserialize::SetOpTreePrivate >(other) {}

SetOpTree::SetOpTree(SotType type, const StringCompleter & stringCompleter) :  MyParentClass() {
	if (type == SOT_SIMPLE)
		setPrivate( new SetOpTreePrivateSimple(stringCompleter) );
	else
		setPrivate( new SetOpTreePrivateComplex(stringCompleter)); 
}

SetOpTree & SetOpTree::operator=(const SetOpTree & other ) {
	RCWrapper< sserialize::SetOpTreePrivate >::operator=(other);
	return *this;
}

SetOpTree::~SetOpTree() {}


void SetOpTree::copyPrivate() {
	if (privRc() > 1) {
		setPrivate( priv()->copy() );
	}
}

void SetOpTree::setMaxResultSetSize(uint32_t size) {
	if (privRc() > 1) {
		copyPrivate();
	}
	priv()->setMaxResultSetSize(size);
}

void SetOpTree::buildTree(const std::string & queryString) {
	if (privRc() > 1) {
		copyPrivate();
	}
	priv()->buildTree(queryString);
}

ItemIndexIterator SetOpTree::asItemIndexIterator() {
	return priv()->asItemIndexIterator();
}


ItemIndex SetOpTree::update(const std::string & queryString) {
	if (privRc() > 1) {
		copyPrivate();
	}
	return priv()->update(queryString);
}

void SetOpTree::doCompletions() {
	priv()->doCompletions();
}

ItemIndex SetOpTree::doSetOperations(bool /*cached*/) {
	return priv()->doSetOperations();
}

std::set<uint16_t> SetOpTree::getCharacterHint(uint32_t posInQueryString) const {
	return priv()->getCharacterHint(posInQueryString);
}

bool SetOpTree::registerExternalFunction(ExternalFunctoid * function) {
	if (privRc() > 1) {
		copyPrivate();
	}
	return priv()->registerExternalFunction(function);
}

bool SetOpTree::registerSelectableOpFilter(SelectableOpFilter * filter) {
	if (privRc() > 1) {
		copyPrivate();
	}
	return priv()->registerSelectableOpFilter(filter);
}


void SetOpTree::clear() {
	if (privRc() > 1) {
		copyPrivate();
	}
	priv()->clear();
}

std::ostream& SetOpTree::printStructure(std::ostream& out) const {
	return priv()->printStructure(out);
}


}//end namespace