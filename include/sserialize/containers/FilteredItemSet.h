#ifndef SSERIALIZE_FILTERED_ITEM_SET_H
#define SSERIALIZE_FILTERED_ITEM_SET_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/storage/SerializationInfo.h>
#include <sserialize/containers/ItemSet.h>

namespace sserialize {

template<typename DataBaseItemType, typename DataBaseType>
class FilteredItemSet {
	ItemSet<DataBaseItemType, DataBaseType> m_itemSet;
	std::shared_ptr<ItemIndex::ItemFilter> m_filter; 
	sserialize::UByteArrayAdapter m_cache;
	uint32_t m_pos;
	uint32_t m_cachePos;//iterator one past our current item
public:
	FilteredItemSet() : m_filter(std::shared_ptr<ItemIndex::ItemFilter>(new ItemIndex::ItemFilterIdentity()) ), m_pos(0) {};
	FilteredItemSet(const std::string& queryString, const DataBaseType & dataBase, sserialize::SetOpTree::SotType setOpTreeType, const std::shared_ptr<ItemIndex::ItemFilter> & filter) :
	m_itemSet(queryString, dataBase, setOpTreeType),
	m_filter(filter),
	m_cache(sserialize::UByteArrayAdapter::createCache(4, sserialize::MM_PROGRAM_MEMORY)),
	m_pos(0),
	m_cachePos(0)
	{}
	FilteredItemSet(const std::string& queryString, const DataBaseType & dataBase, const SetOpTree & setOpTree, const std::shared_ptr<ItemIndex::ItemFilter> & filter) :
	m_itemSet(queryString, dataBase, setOpTree),
	m_filter(filter),
	m_cache(sserialize::UByteArrayAdapter::createCache(4, sserialize::MM_PROGRAM_MEMORY)),
	m_pos(0),
	m_cachePos(0)
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
		m_cachePos = 0;
		operator++();
	}
	std::set<uint16_t> getCharHints(uint32_t /*posInQuery*/) { return std::set<uint16_t>(); }
	inline uint32_t cacheSize() const { return m_cache.tellPutPtr()/4; }
	inline uint32_t maxSize() const { return m_itemSet.size(); }
	uint32_t size() {
		uint32_t tmp = m_cachePos;
		seekEnd();
		m_cachePos = tmp;
		return cacheSize();
	}
	bool valid() const { return m_pos < m_itemSet.size(); }
	DataBaseItemType operator*() const {
		if (m_cache.tellPutPtr() > 0 && m_cache.tellPutPtr() >= m_cachePos*4) {
			uint32_t id = m_cache.getUint32((m_cachePos-1)*sserialize::SerializationInfo<uint32_t>::length);
			return m_itemSet.db().at(id);
		}
		return DataBaseItemType();
	}
	FilteredItemSet& operator++() {
		return seek(1);
	}
	
	FilteredItemSet& populateCache(uint32_t count) {
		uint32_t cS = cacheSize();
		if (!m_cachePos || cS >= count) //this means the first operator++ failed to find an element => result is empty
			return *this;
		for(uint32_t s = m_itemSet.size(); cS < count && m_pos < s; ++m_pos) {
			uint32_t id = m_itemSet.index().at(m_pos);
			if (m_filter->operator()(id)) {
				m_cache.putUint32(id);
				++cS;
				break;
			}
		}
		return *this;
	}
	
	FilteredItemSet& seek(uint32_t count) {
		populateCache(m_cachePos+count);
		m_cachePos = std::min<uint32_t>(cacheSize(), m_cachePos+count);
		return *this;
	}
	
	FilteredItemSet & seekEnd() {
		return seek(maxSize());
	}
	
	DataBaseItemType at(uint32_t pos) {
		populateCache(pos);
		if (cacheSize() > pos)  {
			return m_itemSet.db().at( m_cache.getUint32( m_cache.tellPutPtr()/sserialize::SerializationInfo<uint32_t>::length - 1 ) );
		}
		return DataBaseItemType();
	}
	void printTreeStructure(std::ostream & out) const { m_itemSet.printTreeStructure(out); }

};

}//end namespace


#endif