#ifndef SET_OP_TREE_H
#define SET_OP_TREE_H
#include <map>

#include "ItemIndex.h"
#include "ItemIndexIterator.h"
#include <sserialize/completers/StringCompleter.h>
#include <sserialize/utility/refcounting.h>


/** This clas abstracts set operations. You give it a query string and a trie and it will perform the necessary operations
 *
 * Supported set operations: EXCLUDE (A \ B), INTERSECT, UNITE
 * Special Strings/Character:
 *   / => INTERSECT
 *   + => UNITE
 *   - => DIFF
 *   ^ => XOR
 *   
 *   $EF[String]
 *   
 *   $EF can be registerd on runtime.
 *   it will get the String in the surrounding brackets 
 */

namespace sserialize {
 
class SetOpTreePrivate;


class SetOpTree: public RCWrapper<SetOpTreePrivate> {
private:
	typedef RCWrapper<SetOpTreePrivate> MyParentClass;
public:
	
	/** This class can be used to support special filtering. I.e.:
	  * If a filter supports a fast and operation, and its used within an and
	  * then it's fast filter op is used, otherwise the probably slower function will be used
	  */
	class SelectableOpFilter: public RefCountObject {
	public:
		typedef enum { OP_NONE=0, OP_INTERSECT=1, OP_UNITE=2, OP_DIFF_FIRST=4, OP_DIFF_SECOND=8, OP_XOR=16, OP_ALL=31} SupportedOps;
	public:
		SelectableOpFilter() : RefCountObject() {}
		virtual ~SelectableOpFilter() {}
		virtual SupportedOps supportedOps() const = 0;
		/** This should invoke the faster routine */
		virtual ItemIndex operator()(SupportedOps op, const std::string & str, const ItemIndex & partner) = 0;
		virtual ItemIndex complete(const std::string & str) = 0;
		virtual ItemIndexIterator operator()(SupportedOps op, const std::string & str, const ItemIndexIterator & partner) = 0;
		virtual ItemIndexIterator partialComplete(const std::string & str) = 0;
		virtual const std::string cmdString() const = 0;
		virtual std::string describe() const { return std::string("no description available"); }
// 		virtual ExternalFunctoid* copy() = 0;
	};

	class ExternalFunctoid: public SelectableOpFilter {
	private:
		SupportedOps supportedOps() const { return SelectableOpFilter::OP_NONE; }
		ItemIndex operator()(SupportedOps op, const std::string & str, const ItemIndex & partner);
		ItemIndex complete( const std::string& str );
		ItemIndexIterator operator()(SupportedOps op, const std::string& str, const sserialize::ItemIndexIterator& partner);
		ItemIndexIterator partialComplete(const std::string & str);
	public:
		ExternalFunctoid() : SelectableOpFilter() {};
		virtual ~ExternalFunctoid() {};
		virtual ItemIndex operator()(const std::string &)=0;
	};
	
	typedef enum { SOT_SIMPLE, SOT_COMPLEX, SOT_NULL} SotType;
	
private:
	void copyPrivate();
public:
	SetOpTree(SotType type = SOT_COMPLEX);
	SetOpTree(const sserialize::SetOpTree& other);
	SetOpTree(SetOpTreePrivate * data);
	SetOpTree & operator=(const SetOpTree & t );
	~SetOpTree();
	
	///Tries to set an upper limit to the result set size to speed-up set operations (soft constraint)
	void setMaxResultSetSize(uint32_t size);
	void setMinStrLen(uint32_t size);
	
	void buildTree(const std::string & queryString);
	ItemIndexIterator asItemIndexIterator();
	ItemIndex update(const std::string& queryString);
	void doCompletions();
	ItemIndex doSetOperations(bool cached=true);
	std::set<uint16_t> getCharacterHint(uint32_t posInQueryString) const;
	bool registerStringCompleter(const sserialize::StringCompleter & strCompleter);
	/** Register external functoid, increases refcount accordingly */
	bool registerExternalFunction(ExternalFunctoid * function);
	/** Register selectable op filter, increases refcount accordingly */
	bool registerSelectableOpFilter(SelectableOpFilter * filter);
	void clear();
	std::ostream & printStructure(std::ostream& out) const;
};

}//end namespace

#endif
