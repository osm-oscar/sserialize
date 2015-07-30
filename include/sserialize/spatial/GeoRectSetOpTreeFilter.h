#ifndef SSERIALIZE_SPATIAL_GEO_RECT_SET_OP_TREE_FILTER_H
#define SSERIALIZE_SPATIAL_GEO_RECT_SET_OP_TREE_FILTER_H
#include <sserialize/search/SetOpTree.h>
#include <sserialize/search/ItemIndexIteratorGeoDB.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateSimple.h>
#include <sserialize/spatial/GeoRect.h>

namespace sserialize {
namespace spatial {

template<typename DataBaseType>
class GeoRectSetOpTreeFilter: public sserialize::SetOpTree::GlobalEndFilter {
private:
	GeoRect m_rect;
	DataBaseType m_db;
public:
	GeoRectSetOpTreeFilter() {}
	GeoRectSetOpTreeFilter(const GeoRect & rect, const DataBaseType & db) : m_rect(rect), m_db(db) {}
	virtual ~GeoRectSetOpTreeFilter() {}
    virtual ItemIndex operator()(const ItemIndex& index) {
		if (index.size() == 0)
			return index;
		
		UByteArrayAdapter data( UByteArrayAdapter::createCache(1) );
		ItemIndexPrivateSimpleCreator creator(index.first(), index.last(), index.size(), data);
    
		for(size_t i = 0; i < index.size(); i++) {
			uint32_t itemId = index.at(i);
			if (m_db.match(itemId, m_rect))
				creator.push_back(itemId);
		}
		return ItemIndex(data, ItemIndex::T_SIMPLE);
    }
};


}}//end namespace

#endif