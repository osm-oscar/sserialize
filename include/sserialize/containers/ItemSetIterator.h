#ifndef SSERIALIZE_ITEM_SET_ITERATOR_H
#define SSERIALIZE_ITEM_SET_ITERATOR_H
#include <sserialize/containers/SetOpTree.h>
#include <sserialize/completers/StringCompleter.h>
#include <sserialize/storage/UByteArrayAdapter.h>


//TODO:Sortierfunktion

namespace sserialize {

template<class DataBaseItemType, class DataBaseType>
class ItemSetIterator {
private:
	std::string m_queryString;
	StringCompleter m_completer;
	DataBaseType m_dataBase;
	SetOpTree m_setOpTree;
	ItemIndexIterator m_indexIt;
	UByteArrayAdapter m_cache;
	uint32_t m_cacheCount;
public:
	ItemSetIterator() {};
	ItemSetIterator(const std::string& queryString, const StringCompleter & completer, const DataBaseType & dataBase);
	~ItemSetIterator() {};
	/** Increases the refcount by one */
	bool registerSelectableOpFilter(SetOpTree::SelectableOpFilter* functoid);
	void execute();
	void update(const std::string & queryString);
	std::set<uint16_t> getCharHints(uint32_t posInQuery);
	inline uint32_t cacheSize() const { return m_cacheCount; }
	inline uint32_t maxSize() const { return m_indexIt.maxSize();}
	uint32_t size();
	bool valid() const { return m_indexIt.valid(); }
	DataBaseItemType operator*() const;
	ItemSetIterator& operator++();
	ItemSetIterator& seek(uint32_t count);
	ItemSetIterator& reset() { m_indexIt.reset(); return *this;}
	sserialize::ItemSetIterator<DataBaseItemType, DataBaseType>& seekEnd();
	DataBaseItemType at(uint32_t pos);
	void printTreeStructure(std::ostream & out) const { m_setOpTree.printStructure(out); }
};

template<class DataBaseItemType, class DataBaseType>
ItemSetIterator<DataBaseItemType, DataBaseType>::ItemSetIterator(const std::string& queryString, const StringCompleter& completer, const DataBaseType & dataBase) :
m_queryString(queryString),
m_completer(completer),
m_dataBase(dataBase),
m_setOpTree(sserialize::SetOpTree::SOT_COMPLEX),
m_cacheCount(0)
{
	m_setOpTree.registerStringCompleter(completer);
}

template<class DataBaseItemType, class DataBaseType>
uint32_t
ItemSetIterator<DataBaseItemType, DataBaseType>::size() {
	return m_cacheCount + m_indexIt.toItemIndex().size();
}


template<class DataBaseItemType, class DataBaseType>
DataBaseItemType
ItemSetIterator<DataBaseItemType, DataBaseType>::at(uint32_t pos) {
	if (pos < m_cacheCount)
		return m_dataBase.at(m_cache.getUint32(pos*4));
	while (m_indexIt.valid() && m_cacheCount <= pos) {
		m_cache.putUint32(m_cacheCount*4, *m_indexIt);
		++m_indexIt;
		m_cacheCount++;
	}
	if (m_indexIt.valid())
		return m_dataBase.at(m_cache.getUint32(pos*4));
	else
		return DataBaseItemType();
}

template<class DataBaseItemType, class DataBaseType>
DataBaseItemType
ItemSetIterator<DataBaseItemType, DataBaseType>::operator*() const {
	if (m_indexIt.valid()) {
		m_dataBase.at(*m_indexIt);
	}
	return DataBaseItemType();
}

template<class DataBaseItemType, class DataBaseType>
ItemSetIterator<DataBaseItemType, DataBaseType>&
ItemSetIterator<DataBaseItemType, DataBaseType>::operator++() {
	if (m_indexIt.valid()) {
		m_cache.putUint32(m_cacheCount*4, *m_indexIt);
		++m_indexIt;
		m_cacheCount++;
	}
	return *this;
}

template<class DataBaseItemType, class DataBaseType>
ItemSetIterator<DataBaseItemType, DataBaseType>&
ItemSetIterator<DataBaseItemType, DataBaseType>::seek(uint32_t count) {
	while (m_indexIt.valid() && m_cacheCount < count) {
		m_cache.putUint32(m_cacheCount*4, *m_indexIt);
		++m_indexIt;
		m_cacheCount++;
	}
	return *this;
}

template<class DataBaseItemType, class DataBaseType>
ItemSetIterator<DataBaseItemType, DataBaseType>&
ItemSetIterator<DataBaseItemType, DataBaseType>::seekEnd() {
	while (m_indexIt.valid()) {
		m_cache.putUint32(m_cacheCount*4, *m_indexIt);
		++m_indexIt;
		m_cacheCount++;
	}
	return *this;
}

template<class DataBaseItemType, class DataBaseType>
void
ItemSetIterator<DataBaseItemType, DataBaseType>::execute() {
	m_setOpTree.buildTree(m_queryString);
	m_indexIt = m_setOpTree.asItemIndexIterator();
	m_cache = UByteArrayAdapter::createCache(m_indexIt.maxSize()*4, sserialize::MM_PROGRAM_MEMORY);
}

template<class DataBaseItemType, class DataBaseType>
void
ItemSetIterator<DataBaseItemType, DataBaseType>::update(const std::string & queryString) {
	m_queryString = queryString;
	m_cacheCount = 0;
	m_setOpTree.buildTree(m_queryString);
	m_indexIt = m_setOpTree.asItemIndexIterator();
	m_cache = UByteArrayAdapter::createCache(m_indexIt.maxSize()*4, sserialize::MM_PROGRAM_MEMORY);
}

template<class DataBaseItemType, class DataBaseType>
bool
ItemSetIterator<DataBaseItemType, DataBaseType>::registerSelectableOpFilter(SetOpTree::SelectableOpFilter* functoid) {
	return m_setOpTree.registerSelectableOpFilter(functoid);
}

template<class DataBaseItemType, class DataBaseType>
std::set< uint16_t >
ItemSetIterator<DataBaseItemType, DataBaseType>::getCharHints(uint32_t /*posInQueryString*/) {
	return std::set< uint16_t >();
}

}//end namespace

#endif
