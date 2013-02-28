#ifndef SET_OP_TREE_PRIVATE_SIMPLE_H
#define SET_OP_TREE_PRIVATE_SIMPLE_H
#include "SetOpTreePrivate.h"


/** This class abstracts set operations. It is the base class for multiple Implementations.
  * 
 */
 
namespace sserialize {
 
class SetOpTreePrivateSimple: public SetOpTreePrivate {
private:

	struct QueryStringDescription: std::pair<std::string, uint8_t> {
		QueryStringDescription() : std::pair<std::string, uint8_t>(std::string(), sserialize::StringCompleter::QT_NONE) {}
		QueryStringDescription(const std::string::const_iterator & begin, const std::string::const_iterator & end, uint8_t qt) :
			std::pair<std::string, uint8_t>(std::string(begin, end), qt) {}
		virtual ~QueryStringDescription() {}
		inline std::string & str() { return this->first;}
		inline const std::string & str() const { return this->first;}
		inline uint8_t & qt() { return this->second; }
		inline const uint8_t & qt() const { return this->second; }
	};
	
	typedef enum { QSPS_START, QSPS_CP_NEXT, QSPS_READ_TOKEN } QueryStringParserStates;
	
private:
	StringCompleter m_strCompleter;
	std::vector<QueryStringDescription> m_intersectStrings;
	std::vector<QueryStringDescription> m_diffStrings;
	std::map< std::pair<std::string, uint8_t>, ItemIndex> m_completions;
	uint32_t m_maxResultSetSize;
	
	SetOpTreePrivateSimple(const SetOpTreePrivateSimple & other);
	
public:
	SetOpTreePrivateSimple();
	SetOpTreePrivateSimple(const sserialize::StringCompleter & strCompleter);
	virtual ~SetOpTreePrivateSimple();
	virtual SetOpTreePrivate * copy() const;
	
	virtual void setMaxResultSetSize(uint32_t size) { m_maxResultSetSize = size; }

	virtual void buildTree(const std::string & queryString);
	virtual ItemIndexIterator asItemIndexIterator();
	virtual ItemIndex update(const std::string & queryString);
	virtual void doCompletions();
	virtual ItemIndex doSetOperations();
	virtual bool registerExternalFunction(SetOpTree::ExternalFunctoid * function);
	/** creates a copy of filter by calling copy() on filter which will be deleted if Tree is deleted  */
	virtual bool registerSelectableOpFilter(SetOpTree::SelectableOpFilter * filter);
	virtual void clear();
	virtual std::ostream & printStructure(std::ostream & out) const;
};

}

#endif