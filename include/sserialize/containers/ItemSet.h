#ifndef SSERIALIZE_ITEM_SET_H
#define SSERIALIZE_ITEM_SET_H
#include <sserialize/containers/SetOpTree.h>
#include <sserialize/search/StringCompleter.h>


//TODO:Sortierfunktion

namespace sserialize {

template<class DataBaseItemType, class DataBaseType>
class ItemSet {
public:
	typedef DataBaseItemType value_type;
	typedef value_type ItemType;
private:
	std::string m_queryString;
	DataBaseType m_dataBase;
	SetOpTree m_setOpTree;
	ItemIndex m_index;
public:
	ItemSet() {};
	ItemSet(const std::string& queryString, const DataBaseType & dataBase, const SetOpTree & setOpTree);
	ItemSet(const std::deque< std::string >& intersectStrings, const StringCompleter& completer, const DataBaseType & dataBase, SetOpTree::SotType type);
	ItemSet(const std::string& queryString, const StringCompleter & completer, const DataBaseType & dataBase, SetOpTree::SotType type);
	ItemSet(const DataBaseType & dataBase, const sserialize::ItemIndex & idx);
	~ItemSet() {};
	
	///Tries to set an upper limit to the result set size to speed-up set operations (soft constraint)
	void setMaxResultSetSize(uint32_t size) { m_setOpTree.setMaxResultSetSize(size); }
	void setMinStrLen(uint32_t size) { m_setOpTree.setMinStrLen(size); }
	
	/** increases the refcount by one */
	bool registerSelectableOpFilter(SetOpTree::SelectableOpFilter* functoid);
	bool registerStringCompleter(const sserialize::StringCompleter & strCompleter) { return m_setOpTree.registerStringCompleter(strCompleter); }
	void execute();
	void update(const std::string & queryString);
	std::set<uint16_t> getCharHints(uint32_t posInQuery);
	inline uint32_t size() const { return m_index.size(); }
	DataBaseItemType at(uint32_t pos) const;
	inline ItemIndex & getIndex() { return m_index; }
	inline const ItemIndex & index() const { return m_index; }
	inline ItemIndex & index() { return m_index; }
	inline const DataBaseType & db() const { return m_dataBase; }
	inline DataBaseType & db() { return m_dataBase; }

	void printTreeStructure(std::ostream & out) const { m_setOpTree.printStructure(out); }
};

template<class DataBaseItemType, class DataBaseType>
ItemSet<DataBaseItemType, DataBaseType>::ItemSet(const std::string& queryString, const DataBaseType & dataBase, const SetOpTree & setOpTree) :
m_queryString(queryString),
m_dataBase(dataBase),
m_setOpTree(setOpTree)
{}

template<class DataBaseItemType, class DataBaseType>
ItemSet<DataBaseItemType, DataBaseType>::ItemSet(const std::string& queryString, const StringCompleter& completer, const DataBaseType & dataBase, SetOpTree::SotType type) :
m_queryString(queryString),
m_dataBase(dataBase),
m_setOpTree(type)
{
	m_setOpTree.registerStringCompleter(completer);
}

template<class DataBaseItemType, class DataBaseType>
ItemSet<DataBaseItemType, DataBaseType>::ItemSet(const DataBaseType & dataBase, const sserialize::ItemIndex & idx) :
m_queryString(),
m_dataBase(dataBase),
m_setOpTree(sserialize::SetOpTree::SOT_NULL),
m_index(idx)
{}

template<class DataBaseItemType, class DataBaseType>
ItemSet<DataBaseItemType, DataBaseType>::ItemSet(const std::deque< std::string >& intersectStrings, const StringCompleter& completer, const DataBaseType & dataBase, SetOpTree::SotType type) :
m_dataBase(dataBase),
m_setOpTree(type)
{
	m_setOpTree.registerStringCompleter(completer);
	m_queryString = "";
	for(size_t i = 0; i < intersectStrings.size(); i++) {
		m_queryString += intersectStrings.at(i) + " ";
	}
}

template<class DataBaseItemType, class DataBaseType>
DataBaseItemType
ItemSet<DataBaseItemType, DataBaseType>::at(uint32_t pos) const {
	if (pos >= m_index.size())
		return DataBaseItemType();
	uint32_t itemId = m_index.at(pos);
// 	std::cout << "ItemSet::at("<< pos << ")=" << itemId << std::endl;
	return m_dataBase.at(itemId);
}

template<class DataBaseItemType, class DataBaseType>
void
ItemSet<DataBaseItemType, DataBaseType>::execute() {
	m_setOpTree.buildTree(m_queryString);
	m_setOpTree.doCompletions();
	m_index = m_setOpTree.doSetOperations(true);
}

template<class DataBaseItemType, class DataBaseType>
void
ItemSet<DataBaseItemType, DataBaseType>::update(const std::string & queryString) {
	m_queryString = queryString;
	m_index = m_setOpTree.update(queryString);
}

template<class DataBaseItemType, class DataBaseType>
bool
ItemSet<DataBaseItemType, DataBaseType>::registerSelectableOpFilter(SetOpTree::SelectableOpFilter * functoid) {
	return m_setOpTree.registerSelectableOpFilter(functoid);
}

template<class DataBaseItemType, class DataBaseType>
std::set< uint16_t >
ItemSet<DataBaseItemType, DataBaseType>::getCharHints(uint32_t posInQueryString) {
	return m_setOpTree.getCharacterHint(posInQueryString);
}

}//end namespace

#endif
