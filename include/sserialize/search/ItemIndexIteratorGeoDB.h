#ifndef SSERILIZE_ITEM_INDEX_ITERATOR_GEODB_H
#define SSERILIZE_ITEM_INDEX_ITERATOR_GEODB_H
#include <sserialize/containers/ItemIndexIterator.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/spatial/GeoRect.h>

namespace sserialize {

/** This ItemIndexIterator is essentially an AND-Filter for the given "it".
  * It iterates through the elements in it and checks each Item if it is within the given rect.
  * If you want to just check all Items within a given range, use the ItemIndexIteratorPrivateFixedRange
  */

template<class DataBaseType>
class ItemIndexIteratorGeoDB: public ItemIndexIteratorPrivate {
private:
	DataBaseType m_db;
	ItemIndexIterator m_it;
	sserialize::spatial::GeoRect m_rect;
	bool m_valid;
	uint32_t m_val;
private:
	void moveToNext();
public:
	ItemIndexIteratorGeoDB() : m_valid(false), m_val(0) {}
	ItemIndexIteratorGeoDB(const DataBaseType & db, const spatial::GeoRect & rect, const ItemIndexIterator & it);
	virtual ~ItemIndexIteratorGeoDB() {}
	virtual uint32_t maxSize() const { return m_it.maxSize(); }
	virtual uint32_t operator*() const { return m_val; }
	virtual bool valid() const { return m_valid; }
	virtual void next();
	virtual void reset();
	
	virtual ItemIndexIteratorPrivate * copy() const;
};

template<class DataBaseType>
ItemIndexIteratorGeoDB<DataBaseType>::ItemIndexIteratorGeoDB(const DataBaseType & db, const spatial::GeoRect & rect, const ItemIndexIterator & it) : 
m_db(db),
m_it(it),
m_rect(rect),
m_valid(true),
m_val(0)
{
	next();
}

template<class DataBaseType>
void
ItemIndexIteratorGeoDB<DataBaseType>::next() {
	while (m_it.valid()) {
		if (m_db.match(*m_it, m_rect)) {
			m_val = *m_it;
			++m_it;
			m_valid = true;
			return;
		}
		else {
			++m_it;
		}
	}
	m_val = 0;
	m_valid = 0;
}

template<class DataBaseType>
void
ItemIndexIteratorGeoDB<DataBaseType>::reset() {
	m_it.reset();
	next();
}

template<class DataBaseType>
ItemIndexIteratorPrivate *
ItemIndexIteratorGeoDB<DataBaseType>::copy() const {
	ItemIndexIteratorGeoDB<DataBaseType> * newPriv = new ItemIndexIteratorGeoDB<DataBaseType>();
	newPriv->m_db = m_db;
	newPriv->m_it = m_it;
	newPriv->m_rect = m_rect;
	newPriv->m_valid = m_valid;
	newPriv->m_val = m_val;
	return newPriv;
}

}//end namespace

#endif