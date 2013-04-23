#ifndef SET_OP_TREE_PRIVATE_H
#define SET_OP_TREE_PRIVATE_H
#include <map>
#include <sserialize/completers/StringCompleter.h>
#include "SetOpTree.h"


/** This class abstracts set operations. It is the base class for multiple Implementations.
  * 
 */
 
namespace sserialize {
 
class SetOpTreePrivate: public RefCountObject {
private:
	SetOpTreePrivate & operator=(const SetOpTreePrivate & other);
	SetOpTreePrivate(const SetOpTreePrivate & other);
public:
	SetOpTreePrivate() {}
	virtual ~SetOpTreePrivate() {}
	virtual SetOpTreePrivate * copy() const = 0;

	//sets the maximum result set size, no need to impement this
	virtual void setMaxResultSetSize(uint32_t size) { }
	virtual void setMinStrLen(uint32_t size) {}

	virtual void buildTree(const std::string & queryString) = 0;
	virtual ItemIndexIterator asItemIndexIterator() = 0;
	virtual ItemIndex update(const std::string & queryString) = 0;
	virtual void doCompletions() = 0;
	virtual ItemIndex doSetOperations() = 0;
	/** This is actualy const for cow. It may alter things in StringCompleter */
	virtual std::set<uint16_t> getCharacterHint(uint32_t posInQueryString) { return std::set<uint16_t>(); }
	virtual bool registerExternalFunction(SetOpTree::ExternalFunctoid * function) { return false; }
	/** creates a copy of filter by calling copy() on filter which will be deleted if Tree is deleted  */
	virtual  bool registerSelectableOpFilter(SetOpTree::SelectableOpFilter * filter) { return false; }
	virtual void clear() = 0;
	virtual std::ostream & printStructure(std::ostream & out) const { return (out << "SetOpTreePrivate::printStructure: unsupported" << std::endl);}
};

}

#endif