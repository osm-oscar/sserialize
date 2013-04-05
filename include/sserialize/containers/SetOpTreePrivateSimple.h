#ifndef SET_OP_TREE_PRIVATE_SIMPLE_H
#define SET_OP_TREE_PRIVATE_SIMPLE_H
#include "SetOpTreePrivate.h"


/** This class abstracts set operations. It is the base class for multiple Implementations.
  * 
 */
 
namespace sserialize {
 
class SetOpTreePrivateSimple: public SetOpTreePrivate {
private:

	struct QueryStringDescription {
		std::string m_qstr;
		uint8_t m_qt;
		RCPtrWrapper<SetOpTree::SelectableOpFilter> m_ef;
	
		QueryStringDescription() : m_qt(sserialize::StringCompleter::QT_NONE) {}
		QueryStringDescription(const std::string & qstr) : m_qstr(qstr), m_qt(sserialize::StringCompleter::QT_NONE) {}
		QueryStringDescription(const std::string & qstr, const RCPtrWrapper<SetOpTree::SelectableOpFilter> & ef) : m_qstr(qstr), m_ef(ef) {}
		QueryStringDescription(const std::string & qstr, uint8_t qt) : m_qstr(qstr), m_qt(qt) {}
		QueryStringDescription(const std::string::const_iterator & begin, const std::string::const_iterator & end, uint8_t qt) :
			m_qstr(begin, end), m_qt(qt) {}
		virtual ~QueryStringDescription() {}
		inline std::string & str() { return m_qstr;}
		inline const std::string & str() const { return m_qstr;}
		inline uint8_t & qt() { return m_qt; }
		inline const uint8_t & qt() const { return m_qt; }
		inline RCPtrWrapper<SetOpTree::SelectableOpFilter> & ef() { return m_ef; }
		inline const RCPtrWrapper<SetOpTree::SelectableOpFilter> & ef() const { return m_ef; }
		inline bool operator<(const QueryStringDescription & o) const {
			if (m_qt == o.m_qt) {
				if (m_ef.priv() == o.m_ef.priv()) {
					return m_qstr < o.m_qstr;
				}
				else {
					return m_ef.priv() < o.m_ef.priv();
				}
			}
			else
				return m_qt < o.m_qt;
		}
	};
	
	typedef enum { QSPS_START, QSPS_CP_NEXT, QSPS_READ_TOKEN } QueryStringParserStates;
	
private:
	StringCompleter m_strCompleter;
	std::vector<QueryStringDescription> m_intersectStrings;
	std::vector<QueryStringDescription> m_diffStrings;
	std::map< QueryStringDescription, ItemIndex> m_completions;
	std::map<std::string, RCPtrWrapper<SetOpTree::SelectableOpFilter> > m_ef;
	uint32_t m_maxResultSetSize;
	uint32_t m_minStrLen;
private:
	SetOpTreePrivateSimple(const SetOpTreePrivateSimple & other);
	void ragelParse(const std::string & qstr);
	void handParse(const std::string & qstr);
	
public:
	SetOpTreePrivateSimple();
	SetOpTreePrivateSimple(const sserialize::StringCompleter & strCompleter);
	virtual ~SetOpTreePrivateSimple();
	virtual SetOpTreePrivate * copy() const;
	
	virtual void setMaxResultSetSize(uint32_t size) { m_maxResultSetSize = size; }
	virtual void setMinStrLen(uint32_t size) { m_minStrLen = size; }

	virtual void buildTree(const std::string & qstr);
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