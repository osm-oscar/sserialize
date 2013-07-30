#ifndef SSERIALIZE_FILTERED_ITEM_SET_H
#define SSERIALIZE_FILTERED_ITEM_SET_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/SerializationInfo.h>
#include <sserialize/containers/ItemSet.h>

namespace sserialize {

template<typename DataBaseItemType, typename DataBaseType>
class FilteredItemSet {
	ItemSet<DataBaseItemType, DataBaseType> m_itemSet;
	std::shared_ptr<ItemIndex::ItemFilter> m_filter; 
	sserialize::UByteArrayAdapter m_cache;
	uint32_t m_pos;
public:
	FilteredItemSet() : m_filter(std::shared_ptr<ItemIndex::ItemFilter>(new ItemIndex::ItemFilterIdentity()) ), m_pos(0) {};
	FilteredItemSet(const std::string& queryString, const DataBaseType & dataBase, sserialize::SetOpTree::SotType setOpTreeType, const std::shared_ptr<ItemIndex::ItemFilter> & filter) :
	m_itemSet(queryString, dataBase, setOpTreeType),
	m_filter(filter),
	m_cache(sserialize::UByteArrayAdapter::createCache(4, 0)),
	m_pos(0)
	{}
	FilteredItemSet(const std::string& queryString, const DataBaseType & dataBase, const SetOpTree & setOpTree, const std::shared_ptr<ItemIndex::ItemFilter> & filter) :
	m_itemSet(queryString, dataBase, setOpTree),
	m_filter(filter),
	m_cache(sserialize::UByteArrayAdapter::createCache(4, 0)),
	m_pos(0)
	{}
	~FilteredItemSet() {};
	void setMinStrLen(uint32_t size) { m_itemSet.setMinStrLen(size); }

	/** Increases the refcount by one */
	bool registerSelectableOpFilter(SetOpTree::SelectableOpFilter* functoid) { return m_itemSet.registerSelectableOpFilter(functoid); }
	bool registerStringCompleter(const sserialize::StringCompleter & strCompleter) { return m_itemSet.registerStringCompleter(strCompleter); }
	void execute() {
		m_itemSet.execute();
		operator++();
	}
	void update(const std::string & queryString) {
		m_itemSet.update(queryString);
		m_cache.resetPutPtr();
		m_pos = 0;
		operator++();
	}
	std::set<uint16_t> getCharHints(uint32_t posInQuery) { return std::set<uint16_t>(); }
	inline uint32_t cacheSize() const { return m_cache.tellPutPtr()/4; }
	inline uint32_t maxSize() const { return m_itemSet.size(); }
	bool valid() const { return m_pos < m_itemSet.size(); }
	DataBaseItemType operator*() const {
		if (m_cache.tellPutPtr() > 0) {
			uint32_t id = m_cache.getUint32(m_cache.tellPutPtr()/sserialize::SerializationInfo<uint32_t>::length - 1);
			return m_itemSet.db().at(id);
		}
		return DataBaseItemType();
	}
	FilteredItemSet& operator++() {
		uint32_t s = m_itemSet.size();
		while (m_pos < s) {
			uint32_t id = m_itemSet.index().at(m_pos);
			if (m_filter->operator()(id)) {
				m_cache.putUint32(id);
				break;
			}
		}
		return *this;
	}
	FilteredItemSet& seek(uint32_t count) {
		for(uint32_t i = 0, s = m_itemSet.size(); i < count && m_pos < s; ++i, ++m_pos) {
			uint32_t id = m_itemSet.index().at(m_pos);
			if (m_filter->operator()(id)) {
				m_cache.putUint32(id);
			}
		}
		return *this;
	}
	FilteredItemSet & seekEnd() {
		return seek(maxSize());
	}
	DataBaseItemType at(uint32_t pos) {
		if (m_cache.tellPutPtr()/4 > pos)  {
			return m_itemSet.db().at( m_cache.getUint32( m_cache.tellPutPtr()/sserialize::SerializationInfo<uint32_t>::length - 1 ) );
		}
		return DataBaseItemType();
	}
	void printTreeStructure(std::ostream & out) const { m_itemSet.printTreeStructure(out); }

};

}//end namespace


#endif